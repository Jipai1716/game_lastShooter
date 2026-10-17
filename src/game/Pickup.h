// =============================================================================
//  game/Pickup.h  —  掉落物
// =============================================================================
#pragma once

#include "core/Math.h"
#include "gfx/Color.h"
#include "gfx/Sprite.h"

namespace ls {

enum class PickupKind : int
{
    Speed = 0,
    Damage,
    FireRate,
    Health,
    Grenade,
    Count
};

struct Pickup
{
    PickupKind kind = PickupKind::Speed;
    Vec2 position;
    float life = 0.0f;
    float phase = 0.0f;   // 上下浮动相位
    bool alive = false;
};

const wchar_t* pickupLabel(PickupKind kind);
const Sprite& pickupSprite(PickupKind kind);
Rgba pickupColor(PickupKind kind);

} // namespace ls
