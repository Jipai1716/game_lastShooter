#include "game/Enemy.h"

#include <algorithm>
#include <cmath>

#include "core/Random.h"
#include "gfx/Assets.h"
#include "game/Config.h"
#include "game/World.h"

namespace ls {

namespace {

// 敌人属性表：血量 / 速度 / 半径 / 接触伤害 / 分数 / 主色 / 波次预算 / 是否会分裂
const EnemyStats kStats[] = {
    /* Zombie   */ {"僵尸", 3.0f, 66.0f, 15.0f, 1, 10, palette::kZombie, 1.0f, false},
    /* Runner   */ {"疾行者", 2.0f, 138.0f, 13.0f, 1, 16, palette::kRunner, 1.6f, false},
    /* Slime    */ {"史莱姆", 5.0f, 52.0f, 20.0f, 1, 14, palette::kSlime, 1.5f, true},
    /* SlimeMini*/ {"小史莱姆", 2.0f, 104.0f, 11.0f, 1, 6, palette::kSlime, 0.6f, false},
    /* Spitter  */ {"喷吐者", 4.0f, 74.0f, 15.0f, 1, 26, palette::kSpitter, 2.6f, false},
    /* Brute    */ {"巨兽", 16.0f, 44.0f, 26.0f, 2, 44, palette::kBrute, 3.8f, false},
    /* Boss     */ {"首领", 130.0f, 58.0f, 44.0f, 2, 400, palette::kBoss, 0.0f, false},
};

constexpr int kStatsCount = static_cast<int>(sizeof(kStats) / sizeof(kStats[0]));

} // namespace

const EnemyStats& enemyStats(EnemyKind kind)
{
    const int index = clampi(static_cast<int>(kind), 0, kStatsCount - 1);
    return kStats[index];
}

const Sprite& enemySprite(EnemyKind kind)
{
    const Assets& a = assets();
    switch (kind) {
    case EnemyKind::Zombie: return a.zombie;
    case EnemyKind::Runner: return a.runner;
    case EnemyKind::Slime: return a.slime;
    case EnemyKind::SlimeMini: return a.slimeMini;
    case EnemyKind::Spitter: return a.spitter;
    case EnemyKind::Brute: return a.brute;
    case EnemyKind::Boss: return a.boss;
    default: return a.zombie;
    }
}

bool isKindUnlocked(EnemyKind kind, int wave)
{
    switch (kind) {
    case EnemyKind::Zombie: return true;
    case EnemyKind::Runner: return wave >= 2;
    case EnemyKind::Slime: return wave >= 3;
    case EnemyKind::SlimeMini: return wave >= 3;
    case EnemyKind::Spitter: return wave >= 4;
    case EnemyKind::Brute: return wave >= 5;
    case EnemyKind::Boss: return wave >= cfg::kBossEveryNWaves && wave % cfg::kBossEveryNWaves == 0;
    default: return false;
    }
}

Enemy makeEnemy(EnemyKind kind, Vec2 position, int wave)
{
    const EnemyStats& stats = enemyStats(kind);
    Enemy e;
    e.kind = kind;
    e.position = position;
    e.radius = stats.radius;
    e.maxHealth = stats.health * (1.0f + cfg::kHealthGrowthPerWave * static_cast<float>(wave - 1));
    if (kind == EnemyKind::Boss) {
        // Boss 血量额外随波次上涨
        e.maxHealth = stats.health + static_cast<float>(wave) * 26.0f;
    }
    e.health = e.maxHealth;
    const float speedScale = std::min(cfg::kSpeedGrowthCap, 1.0f + cfg::kSpeedGrowthPerWave * static_cast<float>(wave - 1));
    e.speed = stats.speed * speedScale;
    e.wave = wave;
    e.isBoss = (kind == EnemyKind::Boss);
    e.alive = true;
    e.wobble = rng().range(0.0f, kTau);
    e.attackTimer = rng().range(0.6f, 2.2f);
    e.chargeTimer = rng().range(2.0f, 4.0f);
    e.spawnAnim = 0.0f;
    return e;
}

bool damageEnemy(Enemy& enemy, int amount)
{
    if (!enemy.alive || amount <= 0) return false;
    enemy.health -= static_cast<float>(amount);
    enemy.hitFlash = 1.0f;
    if (enemy.health <= 0.0f) {
        enemy.health = 0.0f;
        return true;
    }
    return false;
}

void updateEnemy(Enemy& enemy, World& world, float dt)
{
    if (!enemy.alive) return;

    enemy.spawnAnim = saturate(enemy.spawnAnim + dt * 4.0f);
    enemy.hitFlash = std::max(0.0f, enemy.hitFlash - dt * 6.0f);
    enemy.wobble += dt * (4.0f + enemy.speed * 0.03f);

    // 击退
    enemy.position += enemy.knockback * dt;
    enemy.knockback *= std::exp(-9.0f * dt);

    const Vec2 toPlayer = world.playerPosition() - enemy.position;
    const float distance = std::max(0.001f, toPlayer.length());
    const Vec2 direction = toPlayer / distance;

    Vec2 desired{0.0f, 0.0f};
    const float speed = enemy.speed;
    const bool spawning = enemy.spawnAnim < 0.55f;   // 出生保护：先不动

    switch (enemy.kind) {
    case EnemyKind::Zombie: {
        const Vec2 strafe = direction.perpendicular() * std::sin(enemy.wobble * 0.8f) * 0.22f;
        desired = (direction + strafe).normalized() * speed;
        break;
    }
    case EnemyKind::Runner: {
        // 直线冲刺 + 正弦摆动：比僵尸更难被预判
        const Vec2 strafe = direction.perpendicular() * std::sin(enemy.wobble * 2.1f) * 0.55f;
        const float burst = 1.0f + 0.28f * std::max(0.0f, std::sin(enemy.wobble * 0.9f));
        desired = (direction + strafe).normalized() * speed * burst;
        break;
    }
    case EnemyKind::Slime: {
        desired = direction * speed * (0.85f + 0.25f * std::sin(enemy.wobble));
        break;
    }
    case EnemyKind::SlimeMini: {
        desired = (direction + direction.perpendicular() * std::sin(enemy.wobble * 3.0f) * 0.4f).normalized() * speed;
        break;
    }
    case EnemyKind::Spitter: {
        // 保持中距离并绕圈
        const float ideal = 300.0f;
        const float radial = clampf((distance - ideal) / 120.0f, -1.0f, 1.0f);
        const Vec2 strafe = direction.perpendicular() * 0.85f;
        desired = (direction * radial + strafe).normalized() * speed;
        if (spawning) break;

        enemy.attackTimer -= dt;
        if (enemy.attackTimer <= 0.0f && distance < 560.0f) {
            enemy.attackTimer = 2.1f;
            const float baseAngle = angleOf(direction);
            for (int i = -1; i <= 1; ++i) {
                const float angle = baseAngle + static_cast<float>(i) * 0.17f;
                world.spawnEnemyBullet(enemy.position + fromAngle(angle, enemy.radius),
                                       fromAngle(angle, 300.0f), cfg::kEnemyBulletDamage);
            }
            world.particles().burst(enemy.position, 6, 40.0f, 120.0f, 0.15f, 0.3f, 3.0f, 6.0f, palette::kSpitter);
        }
        break;
    }
    case EnemyKind::Brute: {
        // 靠近后短暂冲刺
        enemy.chargeTimer -= dt;
        if (enemy.chargeTimer <= 0.0f && distance < 300.0f) {
            enemy.chargeTimer = 3.4f;
            enemy.chargeDirection = direction;
            enemy.pulse = 0.45f;
            world.playSound(Sfx::Dash, 0.5f);
        }
        if (enemy.pulse > 0.0f) {
            enemy.pulse -= dt;
            desired = enemy.chargeDirection * speed * 3.1f;
        } else {
            desired = direction * speed;
        }
        break;
    }
    case EnemyKind::Boss: {
        enemy.attackTimer -= dt;
        enemy.chargeTimer -= dt;

        if (enemy.pulse > 0.0f) {
            // 冲刺阶段
            enemy.pulse -= dt;
            desired = enemy.chargeDirection * speed * 3.6f;
            world.particles().trail(enemy.position, enemy.chargeDirection, 0.5f, 90.0f, 0.35f, 8.0f,
                                    palette::kBoss);
        } else {
            desired = direction * speed * 0.85f;
            if (enemy.attackTimer <= 0.0f) {
                enemy.attackTimer = 3.1f;
                // 环形弹幕
                const int count = 14;
                for (int i = 0; i < count; ++i) {
                    const float angle = static_cast<float>(i) * kTau / static_cast<float>(count) + enemy.wobble;
                    world.spawnEnemyBullet(enemy.position + fromAngle(angle, enemy.radius),
                                           fromAngle(angle, 230.0f), 1);
                }
                world.particles().shockwave(enemy.position, enemy.radius, enemy.radius * 2.4f, palette::kBoss, 0.5f);
                world.addShake(0.35f);
                world.playSound(Sfx::Kill, 0.7f);
            }
            if (enemy.chargeTimer <= 0.0f) {
                enemy.chargeTimer = 5.6f;
                enemy.pulse = 0.5f;
                enemy.chargeDirection = direction;
                world.playSound(Sfx::BossRoar, 0.9f);
                world.addShake(0.5f);
            }
        }
        break;
    }
    default:
        desired = direction * speed;
        break;
    }

    if (spawning) desired = desired * 0.15f;

    // 平滑加速，避免瞬间转向
    const float accel = std::max(120.0f, speed * 6.0f);
    enemy.velocity = approachVec(enemy.velocity, desired, accel * dt);
    enemy.position += enemy.velocity * dt;
}

} // namespace ls
