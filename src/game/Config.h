// =============================================================================
//  game/Config.h  —  全部可调参数集中在这里
//  说明：「手感」几乎完全由这些数字决定，集中一处方便整体调优，
//        也避免原项目里那种把 15、1600、1000 硬编码在逻辑中间的做法。
// =============================================================================
#pragma once

#include "gfx/Color.h"

namespace ls {
namespace cfg {

// ------------------------------------------------------------------ 场地 ----
// 窗口尺寸
constexpr int kScreenWidth = 800;
constexpr int kScreenHeight = 800;

// 实际可行走的战场（内缩，给 HUD 留出位置，玩家永远不会被 HUD 挡住）
constexpr float kArenaLeft = 14.0f;
constexpr float kArenaTop = 96.0f;
constexpr float kArenaWidth = 772.0f;
constexpr float kArenaHeight = 592.0f;
constexpr float kArenaRight = kArenaLeft + kArenaWidth;
constexpr float kArenaBottom = kArenaTop + kArenaHeight;

// 逻辑帧率（固定步长），渲染帧率与之解耦
constexpr int kUpdatesPerSecond = 60;
constexpr float kFixedStep = 1.0f / static_cast<float>(kUpdatesPerSecond);
constexpr int kMaxUpdatesPerFrame = 4;
constexpr int kTargetFps = 120;   // 渲染上限（sleep 节流）

// ------------------------------------------------------------------ 玩家 ----
constexpr float kPlayerRadius = 15.0f;
constexpr float kPlayerAccel = 3000.0f;
constexpr float kPlayerMaxSpeed = 240.0f;
constexpr float kPlayerFriction = 16.0f;
constexpr int kPlayerStartHealth = 5;
constexpr int kPlayerMaxHealthCap = 9;
constexpr float kPlayerInvulnTime = 1.15f;

constexpr float kDashSpeed = 760.0f;
constexpr float kDashDuration = 0.15f;
constexpr float kDashCooldown = 1.05f;
constexpr float kDashInvuln = 0.24f;

// ------------------------------------------------------------------ 武器 ----
constexpr int kMagazineSize = 18;
constexpr float kReloadTime = 0.90f;
constexpr float kFireInterval = 0.115f;
constexpr float kBulletSpeed = 1080.0f;
constexpr float kBulletLife = 0.78f;
constexpr float kBulletRadius = 5.0f;
constexpr int kBulletBaseDamage = 1;
constexpr float kBulletSpread = 0.035f;
constexpr float kRecoilKick = 26.0f;

constexpr int kStartGrenades = 3;
constexpr int kMaxGrenades = 5;
constexpr float kGrenadeThrowSpeed = 640.0f;
constexpr float kGrenadeDrag = 2.4f;
constexpr float kGrenadeFuse = 0.70f;
constexpr float kGrenadeRadius = 118.0f;
constexpr int kGrenadeDamage = 9;
constexpr int kEnemyBulletDamage = 1;

// ---------------------------------------------------------------- 连击计分 ----
constexpr float kComboWindow = 2.6f;
constexpr int kComboMaxMultiplier = 8;   // 1 + combo*0.1 封顶

// ------------------------------------------------------------------ 道具 ----
constexpr float kPickupDropChance = 0.13f;
constexpr float kPickupLife = 15.0f;
constexpr float kPickupRadius = 20.0f;
constexpr float kPickupMagnetRadius = 105.0f;
constexpr float kPickupMagnetSpeed = 460.0f;

// ------------------------------------------------------------------ 波次 ----
constexpr int kMaxAliveEnemies = 30;
constexpr float kFirstWaveDelay = 1.4f;
constexpr float kIntermissionTime = 2.6f;
constexpr int kBossEveryNWaves = 5;

// 敌人随波次成长
constexpr float kHealthGrowthPerWave = 0.075f;
constexpr float kSpeedGrowthPerWave = 0.016f;
constexpr float kSpeedGrowthCap = 1.65f;

// ------------------------------------------------------------------ 视觉 ----
constexpr float kDamageTextLife = 0.75f;
constexpr float kShakeDecay = 2.6f;
constexpr float kShakeMagnitude = 9.0f;
constexpr float kHitstopScale = 0.0f;      // 命中停顿时的时间缩放
constexpr float kDeathSlowmo = 0.28f;      // 死亡慢镜头
constexpr float kDeathSlowmoTime = 0.9f;

} // namespace cfg
} // namespace ls
