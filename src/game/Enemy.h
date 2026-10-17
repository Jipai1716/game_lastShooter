// =============================================================================
//  game/Enemy.h  —  敌人数据、成长表与行为声明
//  说明：5 种常规敌人 + 1 种 Boss，行为实现放在 Enemy.cpp（需要 World 的服务）。
// =============================================================================
#pragma once

#include <string>

#include "core/Math.h"
#include "gfx/Color.h"
#include "gfx/Sprite.h"

namespace ls {

class World;

enum class EnemyKind : int
{
    Zombie = 0,
    Runner,
    Slime,
    SlimeMini,
    Spitter,
    Brute,
    Boss,
    Count
};

struct EnemyStats
{
    const char* name;
    float health;
    float speed;
    float radius;
    int damage;
    int score;
    Rgba accent;
    float scoreWeight;   // 波次预算消耗
    bool canSplit;       // 死亡时分裂
};

const EnemyStats& enemyStats(EnemyKind kind);

struct Enemy
{
    EnemyKind kind = EnemyKind::Zombie;
    Vec2 position;
    Vec2 velocity;
    Vec2 knockback;        // 受击击退速度，会快速衰减
    float health = 1.0f;
    float maxHealth = 1.0f;
    float radius = 14.0f;
    float speed = 60.0f;
    float hitFlash = 0.0f;    // 白闪 0..1
    float spawnAnim = 0.0f;   // 出生动画 0..1
    float wobble = 0.0f;      // 走动的相位
    float attackTimer = 0.0f; // 远程攻击冷却
    float pulse = 0.0f;       // Boss 蓄力/脉冲
    float chargeTimer = 0.0f; // 冲刺/爆发计时
    Vec2 chargeDirection;
    bool alive = false;
    bool isBoss = false;
    int wave = 1;
};

// 生成一个敌人（会按波次缩放血量/速度）
Enemy makeEnemy(EnemyKind kind, Vec2 position, int wave);

// 扣血；返回 true 表示这一击把它打死了
bool damageEnemy(Enemy& enemy, int amount);

// 单帧行为更新（追逐、远程、Boss 阶段等）
void updateEnemy(Enemy& enemy, World& world, float dt);

// 该类型在当前波次是否已解锁
bool isKindUnlocked(EnemyKind kind, int wave);

// 敌人使用的贴图
const Sprite& enemySprite(EnemyKind kind);

} // namespace ls
