#include "game/Wave.h"

#include <algorithm>
#include <cstdio>

#include "core/Random.h"
#include "game/Config.h"
#include "game/World.h"

namespace ls {

namespace {

// 造价表（与 EnemyStats::scoreWeight 对应，这里只用于组牌）
float costOf(EnemyKind kind) { return enemyStats(kind).scoreWeight; }

EnemyKind pickWeighted(const std::vector<EnemyKind>& candidates)
{
    float total = 0.0f;
    for (EnemyKind kind : candidates) {
        switch (kind) {
        case EnemyKind::Zombie: total += 3.0f; break;
        case EnemyKind::Runner: total += 2.0f; break;
        case EnemyKind::Slime: total += 2.0f; break;
        case EnemyKind::Spitter: total += 1.4f; break;
        case EnemyKind::Brute: total += 1.0f; break;
        default: total += 1.0f; break;
        }
    }
    if (total <= 0.0f) return EnemyKind::Zombie;
    float roll = rng().range(0.0f, total);
    for (EnemyKind kind : candidates) {
        float weight = 1.0f;
        switch (kind) {
        case EnemyKind::Zombie: weight = 3.0f; break;
        case EnemyKind::Runner: weight = 2.0f; break;
        case EnemyKind::Slime: weight = 2.0f; break;
        case EnemyKind::Spitter: weight = 1.4f; break;
        case EnemyKind::Brute: weight = 1.0f; break;
        default: break;
        }
        roll -= weight;
        if (roll <= 0.0f) return kind;
    }
    return candidates.back();
}

} // namespace

int waveBudget(int wave)
{
    return 4 + wave * 2;
}

std::vector<EnemyKind> composeWave(int wave)
{
    std::vector<EnemyKind> result;

    // Boss 波：一只 Boss + 一半预算的小怪
    if (wave % cfg::kBossEveryNWaves == 0) {
        result.push_back(EnemyKind::Boss);
        float budget = static_cast<float>(waveBudget(wave)) * 0.5f;
        while (budget >= 1.0f) {
            std::vector<EnemyKind> candidates;
            if (budget >= costOf(EnemyKind::Runner)) candidates.push_back(EnemyKind::Runner);
            if (budget >= costOf(EnemyKind::Spitter)) candidates.push_back(EnemyKind::Spitter);
            candidates.push_back(EnemyKind::Zombie);
            const EnemyKind kind = pickWeighted(candidates);
            result.push_back(kind);
            budget -= costOf(kind);
        }
        return result;
    }

    float budget = static_cast<float>(waveBudget(wave));
    const EnemyKind pool[] = {EnemyKind::Zombie, EnemyKind::Runner, EnemyKind::Slime, EnemyKind::Spitter,
                              EnemyKind::Brute};
    while (budget >= 1.0f) {
        std::vector<EnemyKind> candidates;
        for (EnemyKind kind : pool) {
            if (!isKindUnlocked(kind, wave)) continue;
            if (costOf(kind) > budget) continue;
            candidates.push_back(kind);
        }
        if (candidates.empty()) break;
        const EnemyKind kind = pickWeighted(candidates);
        result.push_back(kind);
        budget -= costOf(kind);
    }
    if (result.empty()) result.push_back(EnemyKind::Zombie);
    return result;
}

void WaveDirector::reset()
{
    state_ = State::FirstDelay;
    wave_ = 0;
    stateTimer_ = cfg::kFirstWaveDelay;
    spawnTimer_ = 0.0f;
    intermissionTimer_ = 0.0f;
    bannerTimer_ = 0.0f;
    bannerText_.clear();
    queue_.clear();
}

bool WaveDirector::bossWave() const
{
    return wave_ > 0 && wave_ % cfg::kBossEveryNWaves == 0;
}

void WaveDirector::showBanner(std::wstring text, Rgba color, float seconds)
{
    bannerText_ = std::move(text);
    bannerColor_ = color;
    bannerTimer_ = seconds;
}

void WaveDirector::buildQueue(int wave)
{
    queue_ = composeWave(wave);
    // 打乱出场顺序，但 Boss 放在最后一批（先用小怪消耗玩家）
    std::vector<EnemyKind> bosses;
    std::vector<EnemyKind> others;
    for (EnemyKind kind : queue_) {
        if (kind == EnemyKind::Boss) bosses.push_back(kind);
        else others.push_back(kind);
    }
    for (size_t i = others.size(); i > 1; --i) {
        const size_t j = static_cast<size_t>(rng().rangeInt(0, static_cast<int>(i) - 1));
        std::swap(others[i - 1], others[j]);
    }
    queue_ = others;
    for (EnemyKind kind : bosses) queue_.push_back(kind);
}

void WaveDirector::beginWave(World& world)
{
    ++wave_;
    buildQueue(wave_);
    state_ = State::Spawning;
    spawnTimer_ = 0.25f;

    wchar_t buffer[64];
    if (bossWave()) {
        std::swprintf(buffer, 64, L"第 %d 波  ·  首领来袭", wave_);
        showBanner(buffer, palette::kDanger, 2.6f);
        world.playSound(Sfx::BossRoar, 0.9f);
        world.addShake(0.65f);
    } else {
        std::swprintf(buffer, 64, L"第 %d 波", wave_);
        showBanner(buffer, palette::kAccent, 2.0f);
        world.playSound(Sfx::WaveStart, 0.75f);
    }
    world.stats().wave = wave_;
}

void WaveDirector::update(World& world, float dt)
{
    bannerTimer_ = std::max(0.0f, bannerTimer_ - dt);

    switch (state_) {
    case State::FirstDelay:
        stateTimer_ -= dt;
        if (stateTimer_ <= 0.0f) beginWave(world);
        break;

    case State::Spawning: {
        spawnTimer_ -= dt;
        const float interval = std::max(0.19f, 0.58f - 0.030f * static_cast<float>(wave_));
        while (spawnTimer_ <= 0.0f && !queue_.empty() &&
               world.enemiesRemaining() < cfg::kMaxAliveEnemies) {
            const EnemyKind kind = queue_.back();
            queue_.pop_back();
            world.spawnEnemyAtEdge(kind);
            spawnTimer_ += interval;
        }
        if (queue_.empty()) {
            state_ = State::Clearing;
        }
        break;
    }

    case State::Clearing:
        if (world.enemiesRemaining() == 0) {
            const int bonus = 15 * wave_;
            world.addScore(bonus);
            wchar_t buffer[64];
            std::swprintf(buffer, 64, L"清空 +%d", bonus);
            showBanner(buffer, palette::kGold, 1.6f);
            state_ = State::Intermission;
            intermissionTimer_ = cfg::kIntermissionTime;
            world.playSound(Sfx::Pickup, 0.8f);
        }
        break;

    case State::Intermission:
        intermissionTimer_ -= dt;
        if (intermissionTimer_ <= 0.0f) beginWave(world);
        break;
    }
}

} // namespace ls
