#include "gfx/Particles.h"

#include <algorithm>
#include <cmath>

#include "core/Random.h"
#include "gfx/Assets.h"

namespace ls {

void ParticleSystem::clear() { particles_.clear(); }

void ParticleSystem::spawn(const Particle& particle)
{
    if (particles_.size() >= kCapacity) {
        particles_.erase(particles_.begin(), particles_.begin() + static_cast<long>(kCapacity / 8) + 1);
    }
    particles_.push_back(particle);
}

void ParticleSystem::update(float dt)
{
    for (size_t i = 0; i < particles_.size();) {
        Particle& p = particles_[i];
        p.life -= dt;
        if (p.life <= 0.0f) {
            particles_[i] = particles_.back();
            particles_.pop_back();
            continue;
        }
        p.position += p.velocity * dt;
        const float damping = std::exp(-p.drag * dt);
        p.velocity *= damping;
        p.rotation += p.spin * dt;
        ++i;
    }
}

void ParticleSystem::draw(Canvas& canvas) const
{
    for (const Particle& p : particles_) {
        const float t = 1.0f - saturate(p.life / p.maxLife); // 0 -> 1 生命进程
        const float fade = 1.0f - t;
        const float size = lerpf(p.size, p.sizeEnd, t);
        const Rgba color = withAlpha(p.color, static_cast<float>(alphaOf(p.color)) / 255.0f * fade);

        switch (p.shape) {
        case 1: {
            // 碎片：旋转的小方块
            const float c = std::cos(p.rotation);
            const float s = std::sin(p.rotation);
            const float h = size * 0.5f;
            const Vec2 a = p.position + Vec2{c * -h - s * -h, s * -h + c * -h};
            const Vec2 b = p.position + Vec2{c * h - s * -h, s * h + c * -h};
            const Vec2 d = p.position + Vec2{c * h - s * h, s * h + c * h};
            const Vec2 e = p.position + Vec2{c * -h - s * h, s * -h + c * h};
            canvas.fillTriangle(a, b, d, color);
            canvas.fillTriangle(a, d, e, color);
            break;
        }
        case 2: {
            // 圆环
            canvas.strokeCircle(p.position, size, std::max(1.0f, size * 0.12f), color);
            break;
        }
        default: {
            const Sprite& glow = size > 26.0f ? assets().glowSoft : assets().glowTight;
            const float scale = std::max(0.25f, size / static_cast<float>(glow.width() * 0.5f));
            if (p.additive) {
                canvas.drawSprite(glow, p.position, scale, p.color, fade, true);
            } else {
                canvas.fillCircle(p.position, size * 0.5f, color);
            }
            break;
        }
        }
    }
}

void ParticleSystem::burst(Vec2 position, int count, float speedMin, float speedMax, float lifeMin, float lifeMax,
                           float sizeMin, float sizeMax, Rgba color, bool additive)
{
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.position = position;
        p.velocity = rng().direction() * rng().range(speedMin, speedMax);
        p.maxLife = rng().range(lifeMin, lifeMax);
        p.life = p.maxLife;
        p.size = rng().range(sizeMin, sizeMax);
        p.sizeEnd = p.size * 0.25f;
        p.color = color;
        p.additive = additive;
        p.drag = rng().range(2.0f, 6.0f);
        p.shape = 0;
        spawn(p);
    }
}

void ParticleSystem::cone(Vec2 position, Vec2 direction, float spreadRadians, int count, float speedMin,
                           float speedMax, float lifeMin, float lifeMax, float sizeMin, float sizeMax, Rgba color)
{
    const float base = angleOf(direction);
    for (int i = 0; i < count; ++i) {
        Particle p;
        const float angle = base + rng().range(-spreadRadians, spreadRadians);
        p.position = position;
        p.velocity = fromAngle(angle, rng().range(speedMin, speedMax));
        p.maxLife = rng().range(lifeMin, lifeMax);
        p.life = p.maxLife;
        p.size = rng().range(sizeMin, sizeMax);
        p.sizeEnd = 0.5f;
        p.color = color;
        p.additive = true;
        p.drag = 8.0f;
        p.shape = 0;
        spawn(p);
    }
}

void ParticleSystem::shockwave(Vec2 position, float startRadius, float endRadius, Rgba color, float life)
{
    Particle p;
    p.position = position;
    p.maxLife = life;
    p.life = life;
    p.size = startRadius;
    p.sizeEnd = endRadius;
    p.color = color;
    p.additive = true;
    p.drag = 0.0f;
    p.shape = 2;
    spawn(p);
}

void ParticleSystem::trail(Vec2 position, Vec2 direction, float spread, float speed, float life, float size,
                           Rgba color)
{
    Particle p;
    const float base = angleOf(direction) + kPi; // 反向喷射
    p.position = position;
    p.velocity = fromAngle(base + rng().range(-spread, spread), speed);
    p.maxLife = life;
    p.life = life;
    p.size = size;
    p.sizeEnd = 0.0f;
    p.color = color;
    p.additive = true;
    p.drag = 5.0f;
    p.shape = 0;
    spawn(p);
}

} // namespace ls
