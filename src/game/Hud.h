// =============================================================================
//  game/Hud.h  —  游戏内 HUD
// =============================================================================
#pragma once

#include "core/Input.h"
#include "gfx/Canvas.h"
#include "game/World.h"

namespace ls {

class Hud
{
public:
    void draw(Canvas& canvas, const World& world, const InputState& input, float realTime);
    void drawCrosshair(Canvas& canvas, const World& world, const InputState& input) const;

private:
    void drawHealth(Canvas& canvas, const World& world) const;
    void drawScore(Canvas& canvas, const World& world) const;
    void drawWave(Canvas& canvas, const World& world, float realTime) const;
    void drawAmmo(Canvas& canvas, const World& world) const;
    void drawCombo(Canvas& canvas, const World& world) const;
    void drawBuffs(Canvas& canvas, const World& world) const;
    void drawBossBar(Canvas& canvas, const World& world) const;
    void drawBanner(Canvas& canvas, const World& world) const;
};

} // namespace ls
