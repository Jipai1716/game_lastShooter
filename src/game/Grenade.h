// =============================================================================
//  game/Grenade.h  —  手雷
// =============================================================================
#pragma once

#include "core/Math.h"

namespace ls {

struct Grenade
{
    Vec2 position;
    Vec2 velocity;
    float fuse = 0.0f;
    float spin = 0.0f;
    float spinSpeed = 0.0f;
    bool alive = false;
};

} // namespace ls
