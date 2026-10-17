#include "game/Player.h"

#include <algorithm>
#include <cmath>

#include "core/Random.h"
#include "gfx/Assets.h"
#include "game/Config.h"
#include "game/World.h"

namespace ls {

float Player::fireInterval() const
{
    return cfg::kFireInterval / (1.0f + rateBonus * 0.16f);
}

float Player::moveSpeed() const
{
    return cfg::kPlayerMaxSpeed * (1.0f + speedBonus * 0.085f);
}

int Player::bulletDamage() const
{
    return cfg::kBulletBaseDamage + static_cast<int>(damageBonus);
}

void resetPlayer(Player& player, Vec2 position)
{
    player = Player{};
    player.position = position;
    player.radius = cfg::kPlayerRadius;
    player.health = cfg::kPlayerStartHealth;
    player.maxHealth = cfg::kPlayerStartHealth;
    player.grenades = cfg::kStartGrenades;
    player.magazineSize = cfg::kMagazineSize;
    player.magazine = cfg::kMagazineSize;
    player.aim = -kPi * 0.5f;
    player.alive = true;
}

namespace {

void startReload(Player& player, World& world)
{
    if (player.reloading || player.magazine >= player.magazineSize) return;
    player.reloading = true;
    player.reloadTimer = cfg::kReloadTime;
    world.playSound(Sfx::Reload, 0.55f);
}

} // namespace

void updatePlayer(Player& player, World& world, const InputState& input, float dt)
{
    if (!player.alive) return;

    // ---------------------------------------------------------- 计时器 ----
    player.fireTimer = std::max(0.0f, player.fireTimer - dt);
    player.invulnTimer = std::max(0.0f, player.invulnTimer - dt);
    player.dashCooldown = std::max(0.0f, player.dashCooldown - dt);
    player.dashInvulnTimer = std::max(0.0f, player.dashInvulnTimer - dt);
    player.hitFlash = std::max(0.0f, player.hitFlash - dt * 4.0f);
    player.muzzleFlash = std::max(0.0f, player.muzzleFlash - dt);

    // ---------------------------------------------------------- 朝向 ----
    const Vec2 toMouse = input.mouse - player.position;
    if (toMouse.lengthSq() > 4.0f) {
        player.aim = angleOf(toMouse);
    }
    // 正面视角的像素小人只做左右镜像，不做旋转（旋转 180° 会像躺在地上）
    player.facingLeft = std::cos(player.aim) < -0.25f;

    // ------------------------------------------------------ 移动输入 ----
    Vec2 moveDir{0.0f, 0.0f};
    if (input.isDown(Key::A) || input.isDown(Key::Left)) moveDir.x -= 1.0f;
    if (input.isDown(Key::D) || input.isDown(Key::Right)) moveDir.x += 1.0f;
    if (input.isDown(Key::W) || input.isDown(Key::Up)) moveDir.y -= 1.0f;
    if (input.isDown(Key::S) || input.isDown(Key::Down)) moveDir.y += 1.0f;
    if (moveDir.lengthSq() > 1.0f) moveDir = moveDir.normalized();

    // ---------------------------------------------------------- 冲刺 ----
    if (player.dashTimer > 0.0f) {
        player.dashTimer -= dt;
        player.velocity = player.dashDirection * cfg::kDashSpeed;
        world.particles().trail(player.position, player.dashDirection, 0.45f, 120.0f, 0.35f, 9.0f,
                                palette::kAccent);
    } else {
        if (input.isPressed(Key::Shift) && player.dashCooldown <= 0.0f) {
            const Vec2 dashDir = moveDir.lengthSq() > 0.01f ? moveDir : fromAngle(player.aim);
            player.dashDirection = dashDir;
            player.dashTimer = cfg::kDashDuration;
            player.dashCooldown = cfg::kDashCooldown;
            player.dashInvulnTimer = cfg::kDashInvuln;
            player.invulnTimer = std::max(player.invulnTimer, 0.05f);
            world.playSound(Sfx::Dash, 0.6f);
            world.particles().burst(player.position, 10, 80.0f, 240.0f, 0.2f, 0.4f, 6.0f, 14.0f,
                                    palette::kAccent);
            world.addShake(0.12f);
        }

        const Vec2 targetVelocity = moveDir * player.moveSpeed();
        if (moveDir.lengthSq() > 0.01f) {
            player.velocity = approachVec(player.velocity, targetVelocity, cfg::kPlayerAccel * dt);
        } else {
            player.velocity = approachVec(player.velocity, Vec2{}, cfg::kPlayerFriction * 0.35f * player.moveSpeed() * dt);
        }
        player.walkPhase += player.velocity.length() * dt * 0.05f;
    }

    player.position += player.velocity * dt;

    // ---------------------------------------------------------- 边界 ----
    const Rect arena = world.arena();
    const float margin = player.radius + 2.0f;
    player.position.x = clampf(player.position.x, arena.left() + margin, arena.right() - margin);
    player.position.y = clampf(player.position.y, arena.top() + margin, arena.bottom() - margin);

    // ---------------------------------------------------------- 装弹 ----
    if (player.reloading) {
        player.reloadTimer -= dt;
        if (player.reloadTimer <= 0.0f) {
            player.reloading = false;
            player.magazine = player.magazineSize;
        }
    } else if (player.magazine <= 0) {
        startReload(player, world);
    }
    if (input.isPressed(Key::Reload)) {
        startReload(player, world);
    }

    // ---------------------------------------------------------- 射击 ----
    const bool triggerHeld = input.isDown(Key::MouseLeft) && input.focused;
    if (triggerHeld && !player.reloading && player.fireTimer <= 0.0f) {
        if (player.magazine > 0) {
            player.magazine -= 1;
            player.fireTimer = player.fireInterval();
            player.muzzleFlash = 0.07f;
            world.stats().shotsFired += 1;

            const float spread = rng().range(-cfg::kBulletSpread, cfg::kBulletSpread) *
                                 (1.0f + player.velocity.length() / 900.0f);
            const float angle = player.aim + spread;
            const Vec2 muzzle = player.position + fromAngle(angle, player.radius + 16.0f);
            world.spawnPlayerBullet(muzzle, fromAngle(angle), player.bulletDamage());

            // 后坐力：沿射击反方向轻推
            player.velocity -= fromAngle(angle) * cfg::kRecoilKick;
            world.particles().cone(muzzle, fromAngle(angle), 0.35f, 4, 90.0f, 260.0f, 0.1f, 0.22f, 4.0f, 9.0f,
                                   palette::kGold);
            world.particles().burst(muzzle, 3, 20.0f, 90.0f, 0.12f, 0.25f, 3.0f, 7.0f, palette::kWhite);
            world.addShake(0.06f);
            world.playSound(Sfx::Shoot, 0.5f);
        } else {
            player.fireTimer = 0.2f;
            world.playSound(Sfx::Empty, 0.5f);
        }
    }

    // ---------------------------------------------------------- 手雷 ----
    if ((input.isPressed(Key::Grenade) || input.isPressed(Key::MouseRight)) && player.grenades > 0) {
        player.grenades -= 1;
        const Vec2 dir = fromAngle(player.aim);
        world.throwGrenade(player.position + dir * (player.radius + 10.0f), dir * cfg::kGrenadeThrowSpeed +
                                                                              player.velocity * 0.35f);
        world.playSound(Sfx::Empty, 0.9f);
        world.addShake(0.1f);
    }
}

} // namespace ls
