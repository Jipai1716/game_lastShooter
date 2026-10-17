// =============================================================================
//  game/Bullet.h  —  子弹（玩家/敌人共用结构，用 hostile 区分阵营）
// =============================================================================
#pragma once

#include <vector>

#include "core/Math.h"
#include "gfx/Color.h"

namespace ls {

struct Bullet
{
    Vec2 position;
    Vec2 velocity;
    float life = 0.0f;
    float radius = 5.0f;
    int damage = 1;
    int pierce = 0;         // 还能穿透几个敌人
    bool hostile = false;   // true = 敌人子弹
    Rgba color = palette::kAccent;
    bool alive = false;
};

// 简单的对象池：避免每帧 new/delete
class BulletPool
{
public:
    void reserve(size_t capacity);
    void clear();

    Bullet& spawn();
    void update(float dt);
    void removeDead();

    std::vector<Bullet>& all() { return bullets_; }
    const std::vector<Bullet>& all() const { return bullets_; }
    size_t aliveCount() const;

private:
    std::vector<Bullet> bullets_;
};

} // namespace ls
