#include "game/Bullet.h"

#include <algorithm>

namespace ls {

void BulletPool::reserve(size_t capacity) { bullets_.reserve(capacity); }

void BulletPool::clear() { bullets_.clear(); }

Bullet& BulletPool::spawn()
{
    bullets_.push_back(Bullet{});
    bullets_.back().alive = true;
    return bullets_.back();
}

void BulletPool::update(float dt)
{
    for (Bullet& bullet : bullets_) {
        if (!bullet.alive) continue;
        bullet.position += bullet.velocity * dt;
        bullet.life -= dt;
        if (bullet.life <= 0.0f) bullet.alive = false;
    }
    removeDead();
}

void BulletPool::removeDead()
{
    bullets_.erase(std::remove_if(bullets_.begin(), bullets_.end(),
                                  [](const Bullet& b) { return !b.alive; }),
                   bullets_.end());
}

size_t BulletPool::aliveCount() const
{
    size_t count = 0;
    for (const Bullet& bullet : bullets_) {
        if (bullet.alive) ++count;
    }
    return count;
}

} // namespace ls
