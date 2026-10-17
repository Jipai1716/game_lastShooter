// =============================================================================
//  game/World.h  —  一局游戏的全部模拟状态
//  说明：模拟与表现分离——World 只管「发生了什么」，
//        渲染在 World::draw / Hud / Game 里完成，因此 World 可以被无头自测
//        （--selftest）直接跑几千帧而不需要窗口。
// =============================================================================
#pragma once

#include <string>
#include <vector>

#include "audio/Audio.h"
#include "core/Input.h"
#include "core/Math.h"
#include "gfx/Canvas.h"
#include "gfx/Particles.h"
#include "game/Config.h"
#include "game/Bullet.h"
#include "game/Enemy.h"
#include "game/Grenade.h"
#include "game/Pickup.h"
#include "game/Player.h"
#include "game/Wave.h"

namespace ls {

struct FloatingText
{
    Vec2 position;
    Vec2 velocity;
    float life = 0.0f;
    float maxLife = 1.0f;
    float scale = 1.0f;
    Rgba color = palette::kWhite;
    std::wstring text;
};

struct RunStats
{
    int score = 0;
    int kills = 0;
    int wave = 0;
    int shotsFired = 0;
    int hitsTaken = 0;
    float elapsed = 0.0f;
};

// 爆炸的视觉表现（伤害是瞬时结算的，这里只负责「看起来」）
struct ExplosionVisual
{
    Vec2 position;
    float radius = 0.0f;
    float life = 0.0f;
    float maxLife = 0.4f;
};

class World
{
public:
    World();

    void reset(unsigned int seed);
    void update(float dt, const InputState& input);
    void draw(Canvas& canvas);

    // -------------------------------------------------- 实体可用的服务 ----
    void spawnPlayerBullet(Vec2 position, Vec2 direction, int damage);
    void spawnEnemyBullet(Vec2 position, Vec2 velocity, int damage);
    void spawnEnemy(EnemyKind kind, Vec2 position);
    void spawnEnemyAtEdge(EnemyKind kind);
    void spawnPickup(PickupKind kind, Vec2 position);
    void throwGrenade(Vec2 position, Vec2 velocity);
    void spawnFloatingText(const std::wstring& text, Vec2 position, Rgba color, float scale = 1.0f);
    void explodeAt(Vec2 position, float radius, int damage, bool fromPlayer);
    void addShake(float amount);
    void requestHitstop(float seconds);
    void playSound(Sfx sfx, float volume = 1.0f);
    void addScore(int amount);

    ParticleSystem& particles() { return particles_; }
    std::vector<Enemy>& enemies() { return enemies_; }
    const std::vector<Enemy>& enemies() const { return enemies_; }
    const std::vector<Pickup>& pickups() const { return pickups_; }
    Player& player() { return player_; }
    const Player& player() const { return player_; }
    Vec2 playerPosition() const { return player_.position; }
    Rect arena() const
    {
        return {cfg::kArenaLeft, cfg::kArenaTop, cfg::kArenaWidth, cfg::kArenaHeight};
    }
    int wave() const { return wave_.wave(); }
    const WaveDirector& waveDirector() const { return wave_; }
    RunStats& stats() { return stats_; }
    const RunStats& stats() const { return stats_; }
    bool playerDead() const { return !player_.alive; }
    float comboRatio() const;   // 0..1 连击剩余时间
    int comboTier() const;      // 倍率（1 表示无加成）

    // 统计给结算界面用
    int enemiesRemaining() const;

    // 仅供截图/调试脚本使用：无视无敌帧把玩家打死
    void debugKillPlayer();

private:
    void updateEnemies(float dt);
    void updateBullets(float dt);
    void updateGrenades(float dt);
    void updatePickups(float dt);
    void updateFloatingTexts(float dt);
    void updateExplosions(float dt);
    void updateCombo(float dt);
    void updateShake(float dt);
    void resolveCollisions();

    void drawEnemies(Canvas& canvas);
    void drawPlayer(Canvas& canvas);
    void drawBullets(Canvas& canvas);
    void drawPickups(Canvas& canvas);
    void drawGrenades(Canvas& canvas);
    void drawExplosions(Canvas& canvas);
    void drawFloatingTexts(Canvas& canvas);
    void drawArenaFrame(Canvas& canvas);

    void onEnemyDeath(const Enemy& enemy);
    void damagePlayer(int amount, Vec2 sourcePosition);
    void rollDrop(const Enemy& enemy);

    // ------------------------------------------------------------ 状态 ----
    Player player_;
    std::vector<Enemy> enemies_;
    BulletPool bullets_;       // 玩家子弹
    BulletPool enemyBullets_;
    std::vector<Grenade> grenades_;
    std::vector<Pickup> pickups_;
    std::vector<FloatingText> floatingTexts_;
    std::vector<ExplosionVisual> explosions_;
    ParticleSystem particles_;
    WaveDirector wave_;
    RunStats stats_;

    Vec2 shakeOffset_;
    float shakeTrauma_ = 0.0f;
    float hitstopTimer_ = 0.0f;
    float timeScale_ = 1.0f;
    float spawnProtection_ = 0.0f;

    int combo_ = 0;
    float comboTimer_ = 0.0f;

    float runnerTimer_ = 0.0f;   // 敌人整体节奏用的共享计时器
};

} // namespace ls
