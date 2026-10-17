// =============================================================================
//  game/Wave.h  —  波次导演
//  说明：原项目用 nozoombies() + N+=WAVE/2 的写法，怪物数量会指数爆炸。
//        这里改成「预算制」：每波给定预算，按敌人造价搭配阵容，随波次成长。
// =============================================================================
#pragma once

#include <vector>

#include "game/Enemy.h"

namespace ls {

class World;

class WaveDirector
{
public:
    void reset();
    void update(World& world, float dt);

    int wave() const { return wave_; }
    bool bossWave() const;
    bool inIntermission() const { return state_ == State::Intermission; }
    float intermissionRemaining() const { return intermissionTimer_; }
    float bannerTimer() const { return bannerTimer_; }
    const std::wstring& bannerText() const { return bannerText_; }
    Rgba bannerColor() const { return bannerColor_; }

    // 已生成但还没清完的敌人数量（HUD 用）
    int pendingCount() const { return static_cast<int>(queue_.size()); }

private:
    enum class State
    {
        FirstDelay,
        Spawning,
        Clearing,
        Intermission
    };

    void beginWave(World& world);
    void buildQueue(int wave);
    void showBanner(std::wstring text, Rgba color, float seconds);

    State state_ = State::FirstDelay;
    int wave_ = 0;
    float stateTimer_ = 0.0f;
    float spawnTimer_ = 0.0f;
    float intermissionTimer_ = 0.0f;
    float bannerTimer_ = 0.0f;
    std::wstring bannerText_;
    Rgba bannerColor_ = palette::kAccent;
    std::vector<EnemyKind> queue_;
};

// 供导演/调试使用：一波的敌人预算
int waveBudget(int wave);
std::vector<EnemyKind> composeWave(int wave);

} // namespace ls
