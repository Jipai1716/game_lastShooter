// =============================================================================
//  game/Player.h  —  玩家状态与行为
// =============================================================================
#pragma once

#include "core/Input.h"
#include "core/Math.h"
#include "gfx/Color.h"

namespace ls {

class World;

struct Player
{
    Vec2 position;
    Vec2 velocity;
    Vec2 dashDirection{1.0f, 0.0f};
    float radius = 15.0f;
    float aim = -1.5707963f;      // 朝上的初始瞄准角

    int health = 5;
    int maxHealth = 5;
    int grenades = 3;

    int magazine = 18;
    int magazineSize = 18;
    float reloadTimer = 0.0f;
    bool reloading = false;
    float fireTimer = 0.0f;       // 距离下次可射击的时间

    float invulnTimer = 0.0f;
    float dashTimer = 0.0f;
    float dashCooldown = 0.0f;
    float dashInvulnTimer = 0.0f;
    float hitFlash = 0.0f;

    float damageBonus = 0.0f;     // 攻击道具层数
    float rateBonus = 0.0f;       // 攻速道具层数
    float speedBonus = 0.0f;      // 速度道具层数

    float walkPhase = 0.0f;
    float muzzleFlash = 0.0f;
    bool facingLeft = false;

    bool alive = true;

    float fireInterval() const;
    float moveSpeed() const;
    int bulletDamage() const;
    bool isInvulnerable() const { return invulnTimer > 0.0f || dashInvulnTimer > 0.0f; }
};

void resetPlayer(Player& player, Vec2 position);
void updatePlayer(Player& player, World& world, const InputState& input, float dt);

} // namespace ls
