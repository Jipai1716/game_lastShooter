#include "game/Hud.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "gfx/Assets.h"
#include "ui/Text.h"

namespace ls {

namespace {

std::wstring fmt(const wchar_t* format, int value)
{
    wchar_t buffer[64];
    std::swprintf(buffer, 64, format, value);
    return std::wstring(buffer);
}

void panel(Canvas& canvas, const Rect& rect, float radius = 10.0f)
{
    canvas.fillRoundRect(rect, radius, withAlpha(palette::kPanel, 0.80f));
    canvas.fillRoundRect({rect.x, rect.y, rect.w, 2.0f}, 1.0f, withAlpha(palette::kPanelEdge, 0.9f));
}

} // namespace

void Hud::drawHealth(Canvas& canvas, const World& world) const
{
    const Player& player = world.player();
    const Assets& a = assets();

    const Rect healthPanel{14.0f, 14.0f, 200.0f, 74.0f};
    panel(canvas, healthPanel);

    // 生命：满心 + 空心（间距自适应，避免心多了溢出面板）
    const int hearts = std::max(1, player.maxHealth);
    const float available = healthPanel.w - 40.0f;
    const float spacing = clampf(available / static_cast<float>(hearts), 16.0f, 24.0f);
    const float scale = clampf(spacing / 22.0f, 0.7f, 1.0f);
    for (int i = 0; i < hearts; ++i) {
        const Vec2 position{32.0f + static_cast<float>(i) * spacing, 40.0f};
        const bool filled = i < player.health;
        if (filled) {
            canvas.drawSprite(a.glowSoft, position, 0.20f * scale, palette::kHeal, 0.55f, true);
            canvas.drawSprite(a.heartIcon, position, scale, palette::kWhite, 1.0f, false);
        } else {
            canvas.drawSprite(a.heartIcon, position, scale, rgba(58, 72, 90), 0.95f, false);
        }
    }

    // 手雷
    for (int i = 0; i < cfg::kMaxGrenades; ++i) {
        const Vec2 position{32.0f + static_cast<float>(i) * 20.0f, 72.0f};
        const bool filled = i < player.grenades;
        canvas.drawSprite(a.grenadeIcon, position, 1.0f, filled ? palette::kWhite : rgba(56, 68, 86),
                          filled ? 1.0f : 0.85f, false);
    }
}

void Hud::drawScore(Canvas& canvas, const World& world) const
{
    const RunStats& stats = world.stats();
    panel(canvas, {610.0f, 14.0f, 176.0f, 62.0f});

    ui::Font small = ui::fontMono();
    small.size = 13;
    ui::draw(L"SCORE", {774.0f, 22.0f}, small, palette::kTextDim, ui::Align::Right, false);

    ui::Font big = ui::fontMonoBold();
    big.size = 30;
    ui::draw(fmt(L"%d", stats.score), {774.0f, 40.0f}, big, palette::kGold, ui::Align::Right, true);
}

void Hud::drawWave(Canvas& canvas, const World& world, float realTime) const
{
    (void)realTime;
    const WaveDirector& director = world.waveDirector();
    panel(canvas, {324.0f, 14.0f, 152.0f, 46.0f});

    ui::Font font = ui::fontHeading();
    font.size = 20;
    const std::wstring text = director.wave() > 0 ? fmt(L"第 %d 波", director.wave()) : L"准备";
    ui::draw(text, {400.0f, 20.0f}, font, palette::kAccent, ui::Align::Center, true);

    ui::Font small = ui::fontMono();
    small.size = 13;
    const std::wstring remain = fmt(L"剩余 %d", world.enemiesRemaining() + director.pendingCount());
    ui::draw(remain, {400.0f, 43.0f}, small, palette::kTextDim, ui::Align::Center, false);

    if (director.inIntermission()) {
        wchar_t buffer[64];
        std::swprintf(buffer, 64, L"下一波 %.1fs", director.intermissionRemaining());
        ui::Font hint = ui::fontBody();
        hint.size = 18;
        ui::draw(buffer, {400.0f, cfg::kArenaBottom - 34.0f}, hint, palette::kTextDim, ui::Align::Center, true);
    }
}

void Hud::drawAmmo(Canvas& canvas, const World& world) const
{
    const Player& player = world.player();
    panel(canvas, {14.0f, 716.0f, 216.0f, 68.0f});

    ui::Font font = ui::fontMonoBold();
    font.size = 24;
    const std::wstring ammoText = fmt(L"%d", player.magazine) + L" / " + fmt(L"%d", player.magazineSize);
    ui::draw(ammoText, {30.0f, 724.0f}, font, player.reloading ? palette::kGold : palette::kText, ui::Align::Left,
             true);

    // 弹匣格子
    const int count = std::max(1, player.magazineSize);
    const float gap = 4.0f;
    const float totalWidth = 190.0f;
    const float cellWidth = (totalWidth - gap * static_cast<float>(count - 1)) / static_cast<float>(count);
    for (int i = 0; i < count; ++i) {
        const float x = 30.0f + static_cast<float>(i) * (cellWidth + gap);
        const bool filled = i < player.magazine;
        canvas.fillRect({x, 762.0f, std::max(2.0f, cellWidth), 8.0f},
                        filled ? palette::kAccent : rgba(48, 60, 78));
    }

    if (player.reloading) {
        const float progress = 1.0f - saturate(player.reloadTimer / cfg::kReloadTime);
        canvas.fillRect({30.0f, 762.0f, 190.0f * progress, 8.0f}, palette::kGold);
        ui::Font small = ui::fontBody();
        small.size = 15;
        ui::draw(L"换弹中", {216.0f, 730.0f}, small, palette::kGold, ui::Align::Right, true);
    }
}

void Hud::drawCombo(Canvas& canvas, const World& world) const
{
    const int tier = world.comboTier();
    if (tier <= 1) return;

    const float ratio = world.comboRatio();
    const Vec2 center{400.0f, 730.0f};

    ui::Font font = ui::fontMonoBold();
    font.size = 28;
    ui::draw(L"x" + fmt(L"%d", tier), {center.x, center.y - 8.0f}, font, palette::kGold, ui::Align::Center, true);

    const float width = 130.0f;
    canvas.fillRect({center.x - width * 0.5f, center.y + 28.0f, width, 5.0f}, rgba(38, 48, 64, 220));
    canvas.fillRect({center.x - width * 0.5f, center.y + 28.0f, width * ratio, 5.0f}, palette::kGold);
}

void Hud::drawBuffs(Canvas& canvas, const World& world) const
{
    const Player& player = world.player();
    struct Buff
    {
        int stacks;
        Rgba color;
        const Sprite* icon;
    };
    const Buff buffs[3] = {
        {static_cast<int>(player.speedBonus), pickupColor(PickupKind::Speed), &assets().itemSpeed},
        {static_cast<int>(player.damageBonus), pickupColor(PickupKind::Damage), &assets().itemAttack},
        {static_cast<int>(player.rateBonus), pickupColor(PickupKind::FireRate), &assets().itemRate},
    };

    float x = 786.0f;
    bool any = false;
    for (int i = 2; i >= 0; --i) {
        const Buff& buff = buffs[i];
        if (buff.stacks <= 0) continue;
        any = true;
        const Vec2 position{x - 18.0f, 748.0f};
        canvas.drawSprite(*buff.icon, position, 0.6f, palette::kWhite, 1.0f, false);
        ui::Font font = ui::fontMonoBold();
        font.size = 14;
        ui::draw(L"x" + fmt(L"%d", buff.stacks), {position.x + 12.0f, position.y - 8.0f}, font, buff.color,
                 ui::Align::Left, true);
        x -= 56.0f;
    }
    if (any) {
        ui::Font hint = ui::fontBody();
        hint.size = 12;
        ui::draw(L"强化", {786.0f, 722.0f}, hint, palette::kTextDim, ui::Align::Right, false);
    }
}

void Hud::drawBossBar(Canvas& canvas, const World& world) const
{
    const Enemy* boss = nullptr;
    for (const Enemy& enemy : world.enemies()) {
        if (enemy.alive && enemy.isBoss) {
            boss = &enemy;
            break;
        }
    }
    if (boss == nullptr) return;

    const float width = 420.0f;
    const float x = 400.0f - width * 0.5f;
    const float y = 78.0f;
    const float ratio = saturate(boss->health / std::max(1.0f, boss->maxHealth));

    canvas.fillRect({x - 3.0f, y - 3.0f, width + 6.0f, 15.0f}, withAlpha(palette::kBlack, 0.8f));
    canvas.fillRect({x, y, width, 9.0f}, rgba(52, 18, 26));
    canvas.fillRect({x, y, width * ratio, 9.0f}, palette::kDanger);
    canvas.drawSprite(assets().glowSoft, {x + width * ratio, y + 4.0f}, 0.22f, palette::kDanger, 0.75f, true);
}

void Hud::drawBanner(Canvas& canvas, const World& world) const
{
    const WaveDirector& director = world.waveDirector();
    const float timer = director.bannerTimer();
    if (timer <= 0.0f || director.bannerText().empty()) return;

    const float fade = saturate(timer / 0.45f) * saturate((2.6f - timer) / 0.25f);
    const float rise = (1.0f - saturate(timer / 2.6f)) * -14.0f;

    ui::Font font = ui::fontTitle();
    font.size = 42;
    const Rgba color = withAlpha(director.bannerColor(), fade);
    ui::drawNeon(canvas, director.bannerText(), {400.0f, 236.0f + rise}, font, color, fade * 0.8f, 3.0f);
}

void Hud::drawCrosshair(Canvas& canvas, const World& world, const InputState& input) const
{
    const Player& player = world.player();
    if (!player.alive) return;

    const Vec2 position = input.mouse;
    const float spread = 18.0f + saturate(player.fireTimer / std::max(0.01f, player.fireInterval())) * 14.0f +
                         (player.reloading ? 12.0f : 0.0f);

    const Rgba color = player.reloading ? palette::kGold : palette::kAccent;
    for (int i = 0; i < 4; ++i) {
        const Vec2 dir = fromAngle(static_cast<float>(i) * kPi * 0.5f + kPi * 0.25f);
        canvas.line(position + dir * (spread * 0.55f), position + dir * spread, 2.0f, withAlpha(color, 0.75f));
    }
    canvas.strokeCircle(position, spread * 0.95f, 1.2f, withAlpha(color, 0.22f));
    canvas.blit(assets().crosshair, position, 0.95f);
    canvas.fillCircle(position, 1.6f, withAlpha(color, 0.9f));
}

void Hud::draw(Canvas& canvas, const World& world, const InputState& input, float realTime)
{
    drawHealth(canvas, world);
    drawScore(canvas, world);
    drawWave(canvas, world, realTime);
    drawAmmo(canvas, world);
    drawCombo(canvas, world);
    drawBuffs(canvas, world);
    drawBossBar(canvas, world);
    drawBanner(canvas, world);

    // 底部操作提示（放在战场与底部面板之间的窄条里）
    ui::Font hint = ui::fontBody();
    hint.size = 13;
    ui::draw(L"WASD 移动 · 左键射击 · Shift 冲刺 · 右键/Q 手雷 · R 换弹 · Esc 暂停", {400.0f, 696.0f}, hint,
             withAlpha(palette::kTextDim, 0.8f), ui::Align::Center, false);

    drawCrosshair(canvas, world, input);
}

} // namespace ls
