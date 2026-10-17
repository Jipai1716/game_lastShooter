// =============================================================================
//  gfx/Particles.h  —  粒子系统（固定池，超出容量时覆盖最旧的粒子）
// =============================================================================
#pragma once

#include <vector>

#include "core/Math.h"
#include "gfx/Canvas.h"
#include "gfx/Sprite.h"

namespace ls {

struct Particle
{
    Vec2 position;
    Vec2 velocity;
    float life = 0.0f;
    float maxLife = 1.0f;
    float size = 2.0f;
    float sizeEnd = 0.0f;
    float drag = 4.0f;
    Rgba color = palette::kWhite;
    float rotation = 0.0f;
    float spin = 0.0f;
    int shape = 0;      // 0=柔光点 1=方块碎片 2=扩散圆环
    bool additive = true;
};

class ParticleSystem
{
public:
    void clear();
    void update(float dt);
    void draw(Canvas& canvas) const;

    void spawn(const Particle& particle);

    // ------------------------------------------------------- 预设发射 ----
    // 径向爆散
    void burst(Vec2 position, int count, float speedMin, float speedMax, float lifeMin, float lifeMax,
               float sizeMin, float sizeMax, Rgba color, bool additive = true);
    // 单方向锥形（枪口火花）
    void cone(Vec2 position, Vec2 direction, float spreadRadians, int count, float speedMin, float speedMax,
              float lifeMin, float lifeMax, float sizeMin, float sizeMax, Rgba color);
    // 扩散圆环
    void shockwave(Vec2 position, float startRadius, float endRadius, Rgba color, float life);
    // 沿路径的碎片（拖尾）
    void trail(Vec2 position, Vec2 direction, float spread, float speed, float life, float size, Rgba color);

private:
    std::vector<Particle> particles_;
    static constexpr size_t kCapacity = 900;
};

} // namespace ls
