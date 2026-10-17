#include "game/Game.h"

#include <easyx.h>
#include <windows.h>
#include <mmsystem.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "core/Log.h"
#include "core/Math.h"
#include "core/Random.h"
#include "gfx/Assets.h"
#include "ui/Text.h"

namespace ls {

namespace {

std::wstring wfmtInt(const wchar_t* format, int value)
{
    wchar_t buffer[64];
    std::swprintf(buffer, 64, format, value);
    return std::wstring(buffer);
}

std::wstring wfmtFloat(const wchar_t* format, float value)
{
    wchar_t buffer[64];
    std::swprintf(buffer, 64, format, value);
    return std::wstring(buffer);
}

void drawMenuPanel(Canvas& canvas, const Rect& rect, Rgba edge)
{
    canvas.fillRoundRect(rect, 14.0f, withAlpha(palette::kPanel, 0.88f));
    canvas.fillRoundRect({rect.x, rect.y, rect.w, 2.0f}, 1.0f, withAlpha(edge, 0.85f));
    canvas.fillRoundRect({rect.x, rect.bottom() - 2.0f, rect.w, 2.0f}, 1.0f, withAlpha(edge, 0.35f));
}

} // namespace

std::string executableDirectory()
{
    wchar_t path[MAX_PATH * 2]{};
    const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH * 2);
    if (length == 0) return std::string();
    std::wstring wpath(path, length);
    const size_t pos = wpath.find_last_of(L"\\/");
    if (pos == std::wstring::npos) return std::string();
    wpath = wpath.substr(0, pos + 1);

    const int size = WideCharToMultiByte(CP_UTF8, 0, wpath.c_str(), static_cast<int>(wpath.size()), nullptr, 0,
                                         nullptr, nullptr);
    std::string result(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wpath.c_str(), static_cast<int>(wpath.size()), &result[0], size, nullptr, nullptr);
    return result;
}

// ------------------------------------------------------------------ 入口 ----
int Game::run(const AppOptions& options)
{
    options_ = options;
    exeDirectory_ = executableDirectory();

    if (!assets().load(exeDirectory_)) {
        MessageBoxW(nullptr, L"找不到 assets/image 资源目录。\n请确认 assets 与 exe 在同一层或上层目录。",
                    L"LAST SHOOTER", MB_ICONERROR | MB_OK);
        return 1;
    }
    if (!options_.mute && options_.selftestFrames <= 0) audio().init();

    if (options_.selftestFrames > 0) return runHeadless(options_.selftestFrames);

    if (!canvas_.create(cfg::kScreenWidth, cfg::kScreenHeight, L"LAST SHOOTER · 最后的枪手")) {
        return 1;
    }

    record_ = loadRecord(exeDirectory_);
    world_.reset(static_cast<unsigned int>(GetTickCount64() & 0x7FFFFFFF));
    state_ = State::Title;

    if (!options_.captureDir.empty()) return runCapture(options_.captureDir);

    timeBeginPeriod(1);
    LARGE_INTEGER frequency{};
    LARGE_INTEGER previous{};
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&previous);

    while (!quit_) {
        LARGE_INTEGER now{};
        QueryPerformanceCounter(&now);
        float frameTime = static_cast<float>(now.QuadPart - previous.QuadPart) / static_cast<float>(frequency.QuadPart);
        previous = now;
        frameTime = clampf(frameTime, 0.0f, 0.25f);

        input_.poll(canvas_.center());
        if (input_.state().quit) break;

        if (options_.dumpInput) {
            static int dumpFrame = 0;
            ++dumpFrame;
            if (dumpFrame % 30 == 0) {
                FILE* fp = std::fopen("inputdump.txt", "a");
                if (fp != nullptr) {
                    const InputState& s = input_.state();
                    std::fprintf(fp, "frame=%d time=%.2f focused=%d state=%d down:", dumpFrame,
                                 static_cast<double>(realTime_), s.focused ? 1 : 0, static_cast<int>(state_));
                    for (int i = 0; i < kKeyCount; ++i) {
                        if (s.down[i]) std::fprintf(fp, "%d,", i);
                    }
                    std::fprintf(fp, " pressed:");
                    for (int i = 0; i < kKeyCount; ++i) {
                        if (s.pressed[i]) std::fprintf(fp, "%d,", i);
                    }
                    std::fprintf(fp, "\n");
                    std::fclose(fp);
                }
            }
        }

        InputState frameInput = options_.autoplay ? makeBotInput(frameTime) : input_.state();
        accumulator_ += frameTime;
        int steps = 0;
        while (accumulator_ >= cfg::kFixedStep && steps < cfg::kMaxUpdatesPerFrame) {
            updateFrame(cfg::kFixedStep, frameInput);
            frameInput.clearEdges();
            accumulator_ -= cfg::kFixedStep;
            ++steps;
        }
        if (steps == cfg::kMaxUpdatesPerFrame) accumulator_ = 0.0f;

        realTime_ += frameTime;
        renderFrame();

        // 限制渲染帧率，别把 CPU 跑满
        const float frameBudget = 1.0f / static_cast<float>(cfg::kTargetFps);
        if (frameTime < frameBudget) {
            Sleep(static_cast<DWORD>((frameBudget - frameTime) * 1000.0f));
        }
    }

    shutdown();
    timeEndPeriod(1);
    return 0;
}

void Game::shutdown()
{
    audio().shutdown();
    canvas_.destroy();
}

// -------------------------------------------------------------- 状态推进 ----
void Game::updateFrame(float dt, InputState input)
{
    stateTime_ += dt;

    switch (state_) {
    case State::Title:
        updateTitle(dt, input);
        break;
    case State::Playing:
        updatePlaying(dt, input);
        break;
    case State::Paused:
        updatePaused(dt, input);
        break;
    case State::GameOver:
        updateGameOver(dt, input);
        break;
    }
}

void Game::updateTitle(float dt, const InputState& input)
{
    // 标题界面也跑粒子，让画面有呼吸感
    ambience_.update(dt);
    if (rng().chance(dt * 3.0f)) {
        Particle spark;
        spark.position = {rng().range(0.0f, static_cast<float>(cfg::kScreenWidth)), static_cast<float>(cfg::kScreenHeight) + 10.0f};
        spark.velocity = {rng().range(-14.0f, 14.0f), rng().range(-46.0f, -22.0f)};
        spark.maxLife = rng().range(2.4f, 5.0f);
        spark.life = spark.maxLife;
        spark.size = rng().range(6.0f, 16.0f);
        spark.sizeEnd = 1.0f;
        spark.color = rng().chance(0.25f) ? palette::kGold : palette::kAccent;
        spark.drag = 0.4f;
        ambience_.spawn(spark);
    }

    if (input.isPressed(Key::Confirm) || input.isPressed(Key::MouseLeft)) {
        startRun();
    } else if (input.isPressed(Key::Pause)) {
        quit_ = true;
    }
}

void Game::updatePlaying(float dt, const InputState& input)
{
    if (input.isPressed(Key::Pause)) {
        state_ = State::Paused;
        stateTime_ = 0.0f;
        audio().play(Sfx::Empty, 0.4f);
        return;
    }

    // 切走窗口自动暂停（只对真人操作生效，机器人始终"专注"）
    if (!options_.autoplay && !input.focused) {
        unfocusedTimer_ += dt;
        if (unfocusedTimer_ > 0.35f) {
            unfocusedTimer_ = 0.0f;
            state_ = State::Paused;
            stateTime_ = 0.0f;
            return;
        }
    } else {
        unfocusedTimer_ = 0.0f;
    }

    const float scale = world_.playerDead() ? cfg::kDeathSlowmo : 1.0f;
    world_.update(dt * scale, input);
    bestComboTier_ = std::max(bestComboTier_, world_.comboTier());

    if (world_.playerDead()) {
        deathTimer_ += dt;
        if (deathTimer_ > 1.5f) endRun();
    }
}

void Game::updatePaused(float dt, const InputState& input)
{
    (void)dt;
    if (input.isPressed(Key::Pause) || input.isPressed(Key::Confirm) || input.isPressed(Key::MouseLeft)) {
        state_ = State::Playing;
        stateTime_ = 0.0f;
    } else if (input.isPressed(Key::Reload)) {
        // R 键在暂停界面直接重开
        startRun();
    }
}

void Game::updateGameOver(float dt, const InputState& input)
{
    (void)dt;
    // 让背景的残局继续慢慢动，别像一张死图
    world_.update(cfg::kFixedStep * 0.35f, neutralInput());

    if (stateTime_ < 0.6f) return;
    if (input.isPressed(Key::Confirm) || input.isPressed(Key::MouseLeft)) {
        startRun();
    } else if (input.isPressed(Key::Pause)) {
        state_ = State::Title;
        stateTime_ = 0.0f;
    }
}

void Game::startRun()
{
    world_.reset(static_cast<unsigned int>(GetTickCount64() & 0x7FFFFFFF));
    state_ = State::Playing;
    stateTime_ = 0.0f;
    deathTimer_ = 0.0f;
    newRecord_ = false;
    bestComboTier_ = 1;
    ambience_.clear();
    accumulator_ = 0.0f;
    record_.runs += 1;
}

void Game::endRun()
{
    // 结算界面用的是"死亡那一瞬间"的快照：
    // 死亡后 World 还在后台慢慢跑，不能让它的击杀继续改分数。
    finalStats_ = world_.stats();
    const RunStats& stats = finalStats_;
    if (stats.score > record_.bestScore) {
        record_.bestScore = stats.score;
        newRecord_ = true;
    }
    record_.bestWave = std::max(record_.bestWave, stats.wave);
    record_.bestKills = std::max(record_.bestKills, stats.kills);
    saveRecord(exeDirectory_, record_);

    state_ = State::GameOver;
    stateTime_ = 0.0f;
}

// ------------------------------------------------------------------ 渲染 ----
void Game::renderFrame()
{
    canvas_.beginFrame(palette::kInk);
    switch (state_) {
    case State::Title: renderTitle(); break;
    case State::Playing: renderPlaying(); break;
    case State::Paused:
        renderPlaying();
        renderPaused();
        break;
    case State::GameOver:
        renderPlaying();
        renderGameOver();
        break;
    }
    canvas_.endFrame();
}

void Game::renderPlaying()
{
    world_.draw(canvas_);
    hud_.draw(canvas_, world_, options_.autoplay ? makeBotInput(cfg::kFixedStep) : input_.state(), realTime_);
}

void Game::renderTitle()
{
    // 背景：把游戏场地压得很暗，突出标题
    canvas_.blitAt(assets().background, 0, 0);
    canvas_.darken(0.62f);
    canvas_.fillScreen(withAlpha(palette::kInk, 0.35f));

    // 缓慢移动的霓虹光斑
    const float t = realTime_;
    canvas_.drawSprite(assets().glowSoft, {400.0f + std::sin(t * 0.31f) * 130.0f, 250.0f + std::cos(t * 0.23f) * 60.0f},
                       5.0f, palette::kAccent, 0.20f, true);
    canvas_.drawSprite(assets().glowSoft, {400.0f + std::cos(t * 0.19f) * 180.0f, 520.0f + std::sin(t * 0.27f) * 70.0f},
                       4.0f, palette::kDanger, 0.14f, true);

    ambience_.draw(canvas_);

    // 装饰角色（放在面板与文字之外的左右两侧，避免和文字打架）
    const float bob = std::sin(t * 2.2f) * 5.0f;
    canvas_.drawSprite(assets().hero, {400.0f, 630.0f + bob}, 1.25f, palette::kWhite, 1.0f, false);
    canvas_.drawSprite(assets().zombie, {214.0f, 648.0f + std::sin(t * 1.7f) * 4.0f}, 1.0f, palette::kWhite, 0.75f,
                       false);
    canvas_.drawSprite(assets().slime, {596.0f, 652.0f + std::sin(t * 2.1f + 1.0f) * 4.0f}, 0.9f, palette::kWhite,
                       0.75f, false);

    // 标题
    ui::Font titleFont = ui::fontTitle();
    titleFont.size = 58;
    ui::drawNeon(canvas_, L"LAST SHOOTER", {400.0f, 118.0f}, titleFont, palette::kAccent, 1.2f, 5.0f);

    ui::Font subFont = ui::fontHeading();
    subFont.size = 21;
    ui::drawNeon(canvas_, L"最 后 的 枪 手", {400.0f, 186.0f}, subFont, palette::kGold, 0.7f, 2.0f);

    // 操作面板
    renderControlsPanel(226.0f, 1.0f);

    // 开始提示（呼吸闪烁）
    const float blink = 0.55f + 0.45f * std::sin(t * 4.0f);
    ui::Font prompt = ui::fontHeading();
    prompt.size = 24;
    ui::drawOutlined(L"按 ENTER 开始游戏", {400.0f, 470.0f}, prompt, withAlpha(palette::kText, blink));

    ui::Font small = ui::fontBody();
    small.size = 15;
    ui::draw(L"或点击鼠标左键", {400.0f, 504.0f}, small, withAlpha(palette::kTextDim, 0.9f), ui::Align::Center);

    // 最高纪录
    ui::Font recordFont = ui::fontMonoBold();
    recordFont.size = 18;
    const std::wstring recordText = L"最高分 " + wfmtInt(L"%d", record_.bestScore) + L"   最高波次 " +
                                    wfmtInt(L"%d", record_.bestWave) + L"   总场次 " + wfmtInt(L"%d", record_.runs);
    ui::draw(recordText, {400.0f, 706.0f}, recordFont, palette::kGold, ui::Align::Center, true);

    ui::Font version = ui::fontBody();
    version.size = 13;
    ui::draw(L"v1.0 · EasyX / C++17 重构版", {400.0f, 776.0f}, version, withAlpha(palette::kTextDim, 0.7f),
             ui::Align::Center);
}

void Game::renderControlsPanel(float y, float alpha)
{
    const Rect rect{170.0f, y, 460.0f, 176.0f};
    drawMenuPanel(canvas_, rect, palette::kPanelEdge);

    ui::Font head = ui::fontHeading();
    head.size = 16;
    ui::draw(L"操作说明", {400.0f, y + 14.0f}, head, withAlpha(palette::kAccent, alpha), ui::Align::Center);

    struct Line
    {
        const wchar_t* key;
        const wchar_t* action;
    };
    const Line lines[] = {
        {L"WASD", L"移动"},
        {L"鼠标左键", L"射击（按住连发）"},
        {L"Shift", L"冲刺闪避（短暂无敌）"},
        {L"右键 / Q", L"投掷手雷"},
        {L"R", L"换弹"},
        {L"Esc", L"暂停"},
    };

    ui::Font keyFont = ui::fontMonoBold();
    keyFont.size = 15;
    ui::Font bodyFont = ui::fontBody();
    bodyFont.size = 15;

    float lineY = y + 44.0f;
    for (const Line& line : lines) {
        canvas_.fillRoundRect({196.0f, lineY - 3.0f, 132.0f, 24.0f}, 6.0f, withAlpha(palette::kAccentDim, 0.28f));
        ui::draw(line.key, {262.0f, lineY}, keyFont, withAlpha(palette::kAccent, alpha), ui::Align::Center, false);
        ui::draw(line.action, {344.0f, lineY}, bodyFont, withAlpha(palette::kText, alpha * 0.92f), ui::Align::Left,
                 false);
        lineY += 22.0f;
    }
}

void Game::renderPaused()
{
    canvas_.desaturate(0.55f);
    canvas_.fillScreen(withAlpha(palette::kInk, 0.62f));

    ui::Font title = ui::fontTitle();
    title.size = 54;
    ui::drawNeon(canvas_, L"已 暂 停", {400.0f, 300.0f}, title, palette::kAccent, 1.0f, 4.0f);

    ui::Font hint = ui::fontHeading();
    hint.size = 20;
    ui::drawOutlined(L"ESC / Enter 继续 · R 重新开始", {400.0f, 372.0f}, hint, palette::kText);

    ui::Font small = ui::fontBody();
    small.size = 15;
    ui::draw(L"当前得分 " + wfmtInt(L"%d", world_.stats().score) + L"  ·  第 " + wfmtInt(L"%d", world_.wave()) + L" 波",
             {400.0f, 412.0f}, small, palette::kTextDim, ui::Align::Center);
}

void Game::renderGameOver()
{
    canvas_.desaturate(0.35f);
    canvas_.fillScreen(withAlpha(palette::kInk, 0.72f));

    const RunStats& stats = finalStats_;

    ui::Font title = ui::fontTitle();
    title.size = 58;
    ui::drawNeon(canvas_, L"GAME OVER", {400.0f, 168.0f}, title, palette::kDanger, 1.1f, 5.0f);

    if (newRecord_) {
        const float blink = 0.6f + 0.4f * std::sin(realTime_ * 6.0f);
        ui::Font badge = ui::fontHeading();
        badge.size = 22;
        ui::drawNeon(canvas_, L"★ 新纪录 ★", {400.0f, 224.0f}, badge, palette::kGold, 0.9f, 2.0f - blink * 0.0f);
    }

    // 成绩单
    const Rect panelRect{220.0f, 268.0f, 360.0f, 244.0f};
    drawMenuPanel(canvas_, panelRect, palette::kDanger);

    struct Row
    {
        const wchar_t* label;
        std::wstring value;
    };
    const Row rows[] = {
        {L"最终得分", wfmtInt(L"%d", stats.score)},
        {L"到达波次", wfmtInt(L"%d", stats.wave)},
        {L"击杀数", wfmtInt(L"%d", stats.kills)},
        {L"最高连击", L"x" + wfmtInt(L"%d", bestComboTier_)},
        {L"存活时间", wfmtFloat(L"%.1f s", stats.elapsed)},
        {L"最高分", wfmtInt(L"%d", record_.bestScore)},
    };

    ui::Font labelFont = ui::fontBody();
    labelFont.size = 17;
    ui::Font valueFont = ui::fontMonoBold();
    valueFont.size = 20;

    float rowY = panelRect.y + 20.0f;
    for (const Row& row : rows) {
        ui::draw(row.label, {panelRect.x + 28.0f, rowY}, labelFont, palette::kTextDim, ui::Align::Left, false);
        ui::draw(row.value, {panelRect.right() - 28.0f, rowY}, valueFont, palette::kText, ui::Align::Right, false);
        rowY += 36.0f;
    }

    const float blink = 0.5f + 0.5f * std::sin(realTime_ * 4.0f);
    ui::Font prompt = ui::fontHeading();
    prompt.size = 22;
    ui::drawOutlined(L"ENTER 再来一局 · ESC 返回标题", {400.0f, 560.0f}, prompt,
                     withAlpha(palette::kText, 0.35f + 0.65f * blink));

    ui::Font small = ui::fontBody();
    small.size = 14;
    ui::draw(L"总场次 " + wfmtInt(L"%d", record_.runs), {400.0f, 596.0f}, small, palette::kTextDim, ui::Align::Center,
             false);
}

// -------------------------------------------------------------- 机器人 AI ----
InputState Game::neutralInput() const
{
    InputState state;
    state.focused = true;
    state.mouse = canvas_.center();
    return state;
}

InputState Game::makeBotInput(float dt) const
{
    InputState state;
    state.focused = true;
    state.mouse = world_.player().position + fromAngle(world_.player().aim) * 200.0f;

    if (state_ != State::Playing || world_.playerDead()) return state;

    const Player& player = world_.player();
    const Vec2 playerPos = player.position;

    // 最近的敌人作为目标
    const Enemy* target = nullptr;
    float targetDistance = 1e9f;
    for (const Enemy& enemy : world_.enemies()) {
        if (!enemy.alive) continue;
        const float distance = ls::distance(enemy.position, playerPos);
        if (distance < targetDistance) {
            targetDistance = distance;
            target = &enemy;
        }
    }

    if (target != nullptr) {
        // 预判一点提前量
        const Vec2 aim = target->position + target->velocity * 0.08f;
        state.mouse = aim;
        state.down[static_cast<int>(Key::MouseLeft)] = true;
        state.pressed[static_cast<int>(Key::MouseLeft)] = true;
    }

    // 移动：远离最近的威胁 + 远离墙壁 + 轻微向场地中心靠拢
    Vec2 move{0.0f, 0.0f};
    for (const Enemy& enemy : world_.enemies()) {
        if (!enemy.alive) continue;
        const Vec2 delta = playerPos - enemy.position;
        const float distance = std::max(1.0f, delta.length());
        const float threat = clampf(200.0f / distance, 0.0f, 3.0f);
        move += delta / distance * threat;
    }

    const Rect arenaRect = world_.arena();
    const Vec2 arenaCenter = arenaRect.center();

    // 墙壁压力：越靠近边界推力越强（这是原版机器人被挤在墙角的原因）
    const float wallMargin = 170.0f;
    if (playerPos.x < arenaRect.left() + wallMargin) {
        move.x += (arenaRect.left() + wallMargin - playerPos.x) / wallMargin * 3.2f;
    }
    if (playerPos.x > arenaRect.right() - wallMargin) {
        move.x -= (playerPos.x - (arenaRect.right() - wallMargin)) / wallMargin * 3.2f;
    }
    if (playerPos.y < arenaRect.top() + wallMargin) {
        move.y += (arenaRect.top() + wallMargin - playerPos.y) / wallMargin * 3.2f;
    }
    if (playerPos.y > arenaRect.bottom() - wallMargin) {
        move.y -= (playerPos.y - (arenaRect.bottom() - wallMargin)) / wallMargin * 3.2f;
    }

    // 顺手去捡道具
    const Pickup* targetPickup = nullptr;
    float pickupDistance = 1e9f;
    for (const Pickup& pickup : world_.pickups()) {
        if (!pickup.alive) continue;
        const float distance = ls::distance(pickup.position, playerPos);
        if (distance < pickupDistance) {
            pickupDistance = distance;
            targetPickup = &pickup;
        }
    }
    if (targetPickup != nullptr && pickupDistance < 320.0f && targetDistance > 90.0f) {
        move += (targetPickup->position - playerPos).normalized() * 1.4f;
    } else if (targetDistance > 320.0f) {
        // 没有敌人靠近时向中心巡场
        move += -Vec2{playerPos - arenaCenter}.normalized() * 0.5f;
    }

    if (move.lengthSq() > 0.01f) {
        move = move.normalized();
        if (move.x < -0.35f) state.down[static_cast<int>(Key::A)] = true;
        if (move.x > 0.35f) state.down[static_cast<int>(Key::D)] = true;
        if (move.y < -0.35f) state.down[static_cast<int>(Key::W)] = true;
        if (move.y > 0.35f) state.down[static_cast<int>(Key::S)] = true;
    }

    // 冲刺：被逼近时闪避
    botDashTimer_ -= dt;
    if (targetDistance < 120.0f && botDashTimer_ <= 0.0f && player.dashCooldown <= 0.0f) {
        state.pressed[static_cast<int>(Key::Shift)] = true;
        state.down[static_cast<int>(Key::Shift)] = true;
        botDashTimer_ = 1.4f;
    }

    // 手雷：一群敌人聚在附近时丢
    botGrenadeTimer_ -= dt;
    if (botGrenadeTimer_ <= 0.0f && player.grenades > 0) {
        int nearby = 0;
        for (const Enemy& enemy : world_.enemies()) {
            if (enemy.alive && ls::distance(enemy.position, playerPos) < 190.0f) ++nearby;
        }
        if (nearby >= 3) {
            state.pressed[static_cast<int>(Key::Grenade)] = true;
            state.down[static_cast<int>(Key::Grenade)] = true;
            botGrenadeTimer_ = 3.2f;
        }
    }

    return state;
}

// -------------------------------------------------------------- 无头自测 ----
int Game::runHeadless(int frames)
{
    startRun();
    world_.reset(20240815u);
    int failures = 0;
    int maxEnemies = 0;
    int waveReached = 0;

    for (int frame = 0; frame < frames; ++frame) {
        const InputState input = makeBotInput(cfg::kFixedStep);
        world_.update(cfg::kFixedStep, input);

        maxEnemies = std::max(maxEnemies, world_.enemiesRemaining());
        waveReached = std::max(waveReached, world_.wave());

        // 不变式检查
        const Player& player = world_.player();
        if (!std::isfinite(player.position.x) || !std::isfinite(player.position.y)) {
            std::printf("[selftest] FAIL: player position NaN at frame %d\n", frame);
            ++failures;
            break;
        }
        const Rect arenaRect = world_.arena();
        if (player.position.x < arenaRect.left() - 24.0f || player.position.x > arenaRect.right() + 24.0f ||
            player.position.y < arenaRect.top() - 24.0f || player.position.y > arenaRect.bottom() + 24.0f) {
            std::printf("[selftest] FAIL: player escaped arena at frame %d (%.1f, %.1f)\n", frame, player.position.x,
                        player.position.y);
            ++failures;
            break;
        }
        if (player.health < 0 || player.health > player.maxHealth) {
            std::printf("[selftest] FAIL: health out of range (%d/%d) at frame %d\n", player.health, player.maxHealth,
                        frame);
            ++failures;
            break;
        }
        if (world_.stats().score < 0) {
            std::printf("[selftest] FAIL: negative score at frame %d\n", frame);
            ++failures;
            break;
        }
        if (world_.enemiesRemaining() > cfg::kMaxAliveEnemies + 8) {
            std::printf("[selftest] FAIL: too many enemies (%d) at frame %d\n", world_.enemiesRemaining(), frame);
            ++failures;
            break;
        }
        if (world_.playerDead() && frame > 60) {
            // 机器人被打死是允许的，直接结束这一轮
            std::printf("[selftest] player died at frame %d (wave %d)\n", frame, world_.wave());
            break;
        }
    }

    const RunStats& stats = world_.stats();
    std::printf("----------------------------------------------------------\n");
    std::printf("[selftest] frames      : %d\n", frames);
    std::printf("[selftest] wave        : %d\n", waveReached);
    std::printf("[selftest] kills       : %d\n", stats.kills);
    std::printf("[selftest] score       : %d\n", stats.score);
    std::printf("[selftest] shots fired : %d\n", stats.shotsFired);
    std::printf("[selftest] hits taken  : %d\n", stats.hitsTaken);
    std::printf("[selftest] max enemies : %d\n", maxEnemies);
    std::printf("[selftest] survivors   : %d\n", world_.enemiesRemaining());
    std::printf("----------------------------------------------------------\n");
    std::printf("%s\n", failures == 0 ? "SELFTEST OK" : "SELFTEST FAILED");
    return failures == 0 ? 0 : 2;
}

// ---------------------------------------------------------------- 截图模式 ----
int Game::runCapture(const std::string& directory)
{
    // 开发自测：按脚本跑一遍，把关键画面存成 PNG，方便检查视觉
    CreateDirectoryW(ui::wide(directory).c_str(), nullptr);

    auto saveFrame = [this, &directory](const std::string& name) {
        IMAGE shot(cfg::kScreenWidth, cfg::kScreenHeight);
        getimage(&shot, 0, 0, cfg::kScreenWidth, cfg::kScreenHeight);
        const std::string path = directory + "\\" + name + ".png";
        saveimage(ui::wide(path).c_str(), &shot);
        std::printf("[capture] %s\n", path.c_str());
    };

    options_.autoplay = true;

    auto stepFrames = [this](int count, bool render = true) {
        for (int i = 0; i < count; ++i) {
            const InputState input = makeBotInput(cfg::kFixedStep);
            updateFrame(cfg::kFixedStep, input);
            realTime_ += cfg::kFixedStep;
            if (render) renderFrame();
        }
    };

    // 1) 标题界面
    for (int i = 0; i < 40; ++i) {
        realTime_ += cfg::kFixedStep;
        updateFrame(cfg::kFixedStep, neutralInput());
        renderFrame();
    }
    saveFrame("01_title");

    // 2) 打到有 4 只以上敌人的热闹场面
    startRun();
    stepFrames(200);
    for (int i = 0; i < 1600 && state_ == State::Playing; ++i) {
        if (world_.enemiesRemaining() >= 4 && i > 30) break;
        stepFrames(1);
    }
    // 摆一排道具，检查道具与光效表现
    {
        const Rect arenaRect = world_.arena();
        const float baseY = arenaRect.bottom() - 90.0f;
        const float baseX = arenaRect.left() + 150.0f;
        for (int i = 0; i < 5; ++i) {
            world_.spawnPickup(static_cast<PickupKind>(i), {baseX + static_cast<float>(i) * 88.0f, baseY});
        }
        stepFrames(40);
    }
    saveFrame("02_combat");

    // 3) Boss 波（手动放一只 Boss 到玩家面前，验证血条与弹幕）
    if (state_ == State::Playing) {
        const Vec2 playerPos = world_.player().position;
        world_.spawnEnemy(EnemyKind::Boss, {playerPos.x + 150.0f, playerPos.y - 60.0f});
        stepFrames(150);
        saveFrame("03_boss");
    }

    // 4) 暂停界面
    if (state_ == State::Playing) {
        InputState pause;
        pause.focused = true;
        pause.mouse = canvas_.center();
        pause.press(Key::Pause);
        updateFrame(cfg::kFixedStep, pause);
        renderFrame();
        saveFrame("04_paused");
        InputState resume;
        resume.focused = true;
        resume.mouse = canvas_.center();
        resume.press(Key::Pause);
        updateFrame(cfg::kFixedStep, resume);
    }

    // 5) 强制死亡 -> 结算界面
    if (state_ == State::Playing) {
        world_.debugKillPlayer();
        for (int i = 0; i < 160; ++i) {
            updateFrame(cfg::kFixedStep, neutralInput());
            realTime_ += cfg::kFixedStep;
            renderFrame();
        }
        saveFrame("05_gameover");
    }

    shutdown();
    return 0;
}

} // namespace ls
