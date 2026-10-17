#include "game/World.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "core/Random.h"
#include "core/Log.h"
#include "gfx/Assets.h"
#include "ui/Text.h"

namespace ls {

namespace {

// 数值转文字（伤害飘字用）
std::wstring toText(int value)
{
    wchar_t buffer[32];
    std::swprintf(buffer, 32, L"%d", value);
    return std::wstring(buffer);
}

// 距离内线性衰减的伤害倍率
float falloffFactor(float distance, float radius)
{
    if (distance >= radius) return 0.0f;
    const float inner = radius * 0.55f;
    if (distance <= inner) return 1.0f;
    return 1.0f - (distance - inner) / (radius - inner) * 0.6f;
}

} // namespace

World::World()
{
    bullets_.reserve(256);
    enemyBullets_.reserve(160);
    grenades_.reserve(16);
    pickups_.reserve(48);
    floatingTexts_.reserve(64);
    enemies_.reserve(64);
    explosions_.reserve(16);
}

void World::reset(unsigned int seed)
{
    rng().reseed(seed);

    resetPlayer(player_, {cfg::kArenaLeft + cfg::kArenaWidth * 0.5f, cfg::kArenaTop + cfg::kArenaHeight * 0.5f});
    enemies_.clear();
    bullets_.clear();
    enemyBullets_.clear();
    grenades_.clear();
    pickups_.clear();
    floatingTexts_.clear();
    explosions_.clear();
    particles_.clear();
    wave_.reset();

    stats_ = RunStats{};
    combo_ = 0;
    comboTimer_ = 0.0f;
    shakeTrauma_ = 0.0f;
    shakeOffset_ = Vec2{};
    hitstopTimer_ = 0.0f;
    timeScale_ = 1.0f;
    runnerTimer_ = 0.0f;
}

// ------------------------------------------------------------------ 更新 ----
void World::update(float dt, const InputState& input)
{
    stats_.elapsed += dt;
    updateShake(dt);

    // 命中停顿：冻结模拟，只让粒子慢放，制造打击感
    if (hitstopTimer_ > 0.0f) {
        hitstopTimer_ -= dt;
        particles_.update(dt * 0.2f);
        return;
    }

    if (player_.alive) {
        updatePlayer(player_, *this, input, dt);
    } else {
        player_.muzzleFlash = 0.0f;
    }

    updateEnemies(dt);
    updateBullets(dt);
    updateGrenades(dt);
    updateExplosions(dt);
    updatePickups(dt);
    resolveCollisions();
    wave_.update(*this, dt);
    updateCombo(dt);
    updateFloatingTexts(dt);
    particles_.update(dt);
}

void World::updateEnemies(float dt)
{
    // 敌人之间互相排斥，避免叠在一起变成一坨
    for (size_t i = 0; i < enemies_.size(); ++i) {
        Enemy& a = enemies_[i];
        if (!a.alive) continue;
        for (size_t j = i + 1; j < enemies_.size(); ++j) {
            Enemy& b = enemies_[j];
            if (!b.alive) continue;
            const Vec2 delta = b.position - a.position;
            const float minDist = (a.radius + b.radius) * 0.92f;
            const float distSq = delta.lengthSq();
            if (distSq >= minDist * minDist || distSq < 0.0001f) continue;
            const float dist = std::sqrt(distSq);
            const Vec2 push = delta / dist * ((minDist - dist) * 0.5f);
            const float aWeight = a.isBoss ? 0.1f : 1.0f;
            const float bWeight = b.isBoss ? 0.1f : 1.0f;
            a.position -= push * aWeight;
            b.position += push * bWeight;
        }
    }

    for (Enemy& enemy : enemies_) {
        if (!enemy.alive) continue;
        updateEnemy(enemy, *this, dt);

        // 敌人也被空气墙约束（Boss 允许压到边缘）
        const Rect arena = this->arena();
        const float margin = enemy.radius;
        enemy.position.x = clampf(enemy.position.x, arena.left() - margin * 0.5f, arena.right() + margin * 0.5f);
        enemy.position.y = clampf(enemy.position.y, arena.top() - margin * 0.5f, arena.bottom() + margin * 0.5f);
    }

    // 清理死亡敌人
    enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(), [](const Enemy& e) { return !e.alive; }),
                   enemies_.end());
}

void World::updateBullets(float dt)
{
    // 玩家子弹：撞墙即消失（打中边缘会溅火花）
    for (Bullet& bullet : bullets_.all()) {
        if (!bullet.alive) continue;
        bullet.position += bullet.velocity * dt;
        bullet.life -= dt;
        const Rect arena = this->arena();
        if (bullet.position.x < arena.left() || bullet.position.x > arena.right() ||
            bullet.position.y < arena.top() || bullet.position.y > arena.bottom()) {
            bullet.alive = false;
            particles_.burst(bullet.position, 3, 30.0f, 90.0f, 0.1f, 0.2f, 3.0f, 6.0f, palette::kGold);
        }
        if (bullet.life <= 0.0f) bullet.alive = false;
    }
    bullets_.removeDead();

    for (Bullet& bullet : enemyBullets_.all()) {
        if (!bullet.alive) continue;
        bullet.position += bullet.velocity * dt;
        bullet.life -= dt;
        const Rect arena = this->arena();
        if (!arena.contains(bullet.position) || bullet.life <= 0.0f) {
            bullet.alive = false;
        }
    }
    enemyBullets_.removeDead();
}

void World::updateGrenades(float dt)
{
    const Rect arena = this->arena();
    for (Grenade& grenade : grenades_) {
        if (!grenade.alive) continue;
        grenade.position += grenade.velocity * dt;
        grenade.velocity *= std::exp(-cfg::kGrenadeDrag * dt);
        grenade.fuse -= dt;
        grenade.spin += grenade.spinSpeed * dt;

        // 撞墙反弹
        const float margin = 8.0f;
        if (grenade.position.x < arena.left() + margin) {
            grenade.position.x = arena.left() + margin;
            grenade.velocity.x = -grenade.velocity.x * 0.55f;
        } else if (grenade.position.x > arena.right() - margin) {
            grenade.position.x = arena.right() - margin;
            grenade.velocity.x = -grenade.velocity.x * 0.55f;
        }
        if (grenade.position.y < arena.top() + margin) {
            grenade.position.y = arena.top() + margin;
            grenade.velocity.y = -grenade.velocity.y * 0.55f;
        } else if (grenade.position.y > arena.bottom() - margin) {
            grenade.position.y = arena.bottom() - margin;
            grenade.velocity.y = -grenade.velocity.y * 0.55f;
        }

        if (grenade.fuse <= 0.0f) {
            grenade.alive = false;
            explodeAt(grenade.position, cfg::kGrenadeRadius, cfg::kGrenadeDamage, true);
        } else if (grenade.fuse < 0.22f) {
            // 快炸时冒火花
            particles_.burst(grenade.position, 1, 20.0f, 70.0f, 0.1f, 0.25f, 3.0f, 7.0f, palette::kGold);
        }
    }
    grenades_.erase(std::remove_if(grenades_.begin(), grenades_.end(), [](const Grenade& g) { return !g.alive; }),
                    grenades_.end());
}

void World::updateExplosions(float dt)
{
    for (ExplosionVisual& explosion : explosions_) {
        explosion.life -= dt;
    }
    explosions_.erase(std::remove_if(explosions_.begin(), explosions_.end(),
                                     [](const ExplosionVisual& e) { return e.life <= 0.0f; }),
                      explosions_.end());
}

void World::updatePickups(float dt)
{
    for (Pickup& pickup : pickups_) {
        if (!pickup.alive) continue;
        pickup.life -= dt;
        pickup.phase += dt * 2.6f;
        if (pickup.life <= 0.0f) {
            pickup.alive = false;
            continue;
        }
        const Vec2 toPlayer = player_.position - pickup.position;
        const float distance = toPlayer.length();
        if (player_.alive && distance < cfg::kPickupMagnetRadius) {
            const float speed = cfg::kPickupMagnetSpeed * (1.0f - distance / cfg::kPickupMagnetRadius) + 90.0f;
            pickup.position += toPlayer.normalized() * speed * dt;
        }
    }
    pickups_.erase(std::remove_if(pickups_.begin(), pickups_.end(), [](const Pickup& p) { return !p.alive; }),
                   pickups_.end());
}

void World::updateFloatingTexts(float dt)
{
    for (FloatingText& text : floatingTexts_) {
        text.life -= dt;
        text.position += text.velocity * dt;
        text.velocity *= std::exp(-3.0f * dt);
    }
    floatingTexts_.erase(std::remove_if(floatingTexts_.begin(), floatingTexts_.end(),
                                        [](const FloatingText& t) { return t.life <= 0.0f; }),
                         floatingTexts_.end());
}

void World::updateCombo(float dt)
{
    if (combo_ <= 0) return;
    comboTimer_ -= dt;
    if (comboTimer_ <= 0.0f) {
        combo_ = 0;
        comboTimer_ = 0.0f;
    }
}

void World::updateShake(float dt)
{
    shakeTrauma_ = std::max(0.0f, shakeTrauma_ - cfg::kShakeDecay * dt);
    const float power = shakeTrauma_ * shakeTrauma_;
    const float magnitude = cfg::kShakeMagnitude * power;
    shakeOffset_ = {rng().range(-magnitude, magnitude), rng().range(-magnitude, magnitude)};
}

// -------------------------------------------------------------- 碰撞结算 ----
void World::resolveCollisions()
{
    const Circle playerCircle{player_.position, player_.radius * 0.8f};

    // ---- 玩家子弹 -> 敌人 ----
    for (Bullet& bullet : bullets_.all()) {
        if (!bullet.alive) continue;
        for (Enemy& enemy : enemies_) {
            if (!enemy.alive) continue;
            if (enemy.spawnAnim < 0.35f) continue;   // 出生保护
            if (distanceSq(bullet.position, enemy.position) > (bullet.radius + enemy.radius) * (bullet.radius + enemy.radius)) {
                continue;
            }
            const bool killed = damageEnemy(enemy, bullet.damage);
            const Vec2 impact = bullet.position;
            particles_.burst(impact, killed ? 12 : 6, 60.0f, 220.0f, 0.15f, 0.35f, 4.0f, 10.0f,
                             killed ? enemyStats(enemy.kind).accent : palette::kGold);
            particles_.cone(impact, bullet.velocity.normalized(), 0.5f, 3, 60.0f, 180.0f, 0.1f, 0.2f, 3.0f, 7.0f,
                            palette::kWhite);
            spawnFloatingText(toText(bullet.damage), enemy.position + Vec2{0.0f, -enemy.radius - 6.0f},
                              killed ? palette::kGold : palette::kText, 0.55f);
            addShake(0.06f);
            playSound(Sfx::Hit, 0.45f);

            if (killed) {
                onEnemyDeath(enemy);
                enemy.alive = false;
            }

            if (bullet.pierce > 0) {
                --bullet.pierce;
            } else {
                bullet.alive = false;
                break;
            }
        }
    }
    bullets_.removeDead();

    // ---- 敌人子弹 -> 玩家 ----
    for (Bullet& bullet : enemyBullets_.all()) {
        if (!bullet.alive) continue;
        if (overlaps(Circle{bullet.position, bullet.radius}, playerCircle)) {
            bullet.alive = false;
            damagePlayer(bullet.damage, bullet.position);
        }
    }
    enemyBullets_.removeDead();

    // ---- 敌人 -> 玩家（接触伤害） ----
    if (player_.alive) {
        for (Enemy& enemy : enemies_) {
            if (!enemy.alive || enemy.spawnAnim < 0.4f) continue;
            if (!overlaps(Circle{enemy.position, enemy.radius * 0.85f}, playerCircle)) continue;
            const int damage = enemyStats(enemy.kind).damage;
            damagePlayer(damage, enemy.position);
            // 接触后敌人被弹开一点
            enemy.knockback += (enemy.position - player_.position).normalized() * 180.0f;
            if (!player_.isInvulnerable()) break;
        }
    }

    // ---- 拾取物 -> 玩家 ----
    if (player_.alive) {
        for (Pickup& pickup : pickups_) {
            if (!pickup.alive) continue;
            if (distanceSq(pickup.position, player_.position) > (cfg::kPickupRadius + player_.radius) *
                                                                   (cfg::kPickupRadius + player_.radius)) {
                continue;
            }
            pickup.alive = false;
            const int tier = comboTier();
            addScore(25 * tier);
            spawnFloatingText(pickupLabel(pickup.kind), player_.position + Vec2{0.0f, -40.0f}, pickupColor(pickup.kind),
                              0.8f);
            particles_.burst(pickup.position, 12, 60.0f, 200.0f, 0.2f, 0.45f, 4.0f, 10.0f, pickupColor(pickup.kind));
            switch (pickup.kind) {
            case PickupKind::Speed:
                if (player_.speedBonus < 5.0f) player_.speedBonus += 1.0f;
                break;
            case PickupKind::Damage:
                if (player_.damageBonus < 4.0f) player_.damageBonus += 1.0f;
                break;
            case PickupKind::FireRate:
                if (player_.rateBonus < 5.0f) player_.rateBonus += 1.0f;
                break;
            case PickupKind::Health:
                if (player_.maxHealth < cfg::kPlayerMaxHealthCap) player_.maxHealth += 1;
                player_.health = std::min(player_.maxHealth, player_.health + 1);
                playSound(Sfx::Heal, 0.85f);
                break;
            case PickupKind::Grenade:
                player_.grenades = std::min(cfg::kMaxGrenades, player_.grenades + 1);
                break;
            default:
                break;
            }
            if (pickup.kind != PickupKind::Health) playSound(Sfx::Pickup, 0.6f);
            addShake(0.06f);
        }
        pickups_.erase(std::remove_if(pickups_.begin(), pickups_.end(), [](const Pickup& p) { return !p.alive; }),
                       pickups_.end());
    }
}

// ------------------------------------------------------------ 伤害与死亡 ----
void World::damagePlayer(int amount, Vec2 sourcePosition)
{
    if (!player_.alive || player_.isInvulnerable()) return;

    player_.health -= amount;
    player_.invulnTimer = cfg::kPlayerInvulnTime;
    player_.hitFlash = 1.0f;
    stats_.hitsTaken += amount;
    combo_ = 0;
    comboTimer_ = 0.0f;

    const Vec2 push = (player_.position - sourcePosition).normalized();
    player_.velocity += push * 300.0f;

    addShake(0.6f);
    requestHitstop(0.06f);
    playSound(Sfx::Hurt, 0.9f);
    particles_.burst(player_.position, 16, 80.0f, 300.0f, 0.2f, 0.5f, 4.0f, 12.0f, palette::kDanger);
    particles_.shockwave(player_.position, player_.radius, player_.radius * 3.0f, palette::kDanger, 0.35f);

    if (player_.health <= 0) {
        player_.health = 0;
        player_.alive = false;
        addShake(1.0f);
        timeScale_ = 1.0f;
        particles_.burst(player_.position, 40, 100.0f, 420.0f, 0.3f, 0.9f, 5.0f, 16.0f, palette::kDanger);
        particles_.shockwave(player_.position, player_.radius, 220.0f, palette::kDanger, 0.7f);
        playSound(Sfx::GameOver, 1.0f);
    }
}

void World::onEnemyDeath(const Enemy& enemy)
{
    const EnemyStats& stats = enemyStats(enemy.kind);

    ++combo_;
    comboTimer_ = cfg::kComboWindow;
    const int gain = stats.score * comboTier();
    addScore(gain);
    stats_.kills += 1;

    spawnFloatingText(L"+" + toText(gain), enemy.position + Vec2{0.0f, -enemy.radius - 14.0f}, palette::kGold,
                      enemy.isBoss ? 1.4f : 0.85f);

    particles_.burst(enemy.position, enemy.isBoss ? 46 : 16, 80.0f, enemy.isBoss ? 420.0f : 260.0f, 0.25f, 0.7f,
                     4.0f, enemy.isBoss ? 20.0f : 12.0f, stats.accent);
    particles_.shockwave(enemy.position, enemy.radius * 0.6f, enemy.radius * (enemy.isBoss ? 4.5f : 2.4f), stats.accent,
                          enemy.isBoss ? 0.7f : 0.3f);
    // 溅血（用碎片形状，看起来更像“实体”被打碎）
    for (int i = 0; i < (enemy.isBoss ? 18 : 6); ++i) {
        Particle shard;
        shard.position = enemy.position;
        shard.velocity = rng().direction() * rng().range(60.0f, 240.0f);
        shard.maxLife = rng().range(0.3f, 0.7f);
        shard.life = shard.maxLife;
        shard.size = rng().range(3.0f, 7.0f);
        shard.sizeEnd = 1.0f;
        shard.color = stats.accent;
        shard.additive = false;
        shard.drag = 3.0f;
        shard.shape = 1;
        shard.rotation = rng().range(0.0f, kTau);
        shard.spin = rng().range(-8.0f, 8.0f);
        particles_.spawn(shard);
    }

    playSound(Sfx::Kill, enemy.isBoss ? 1.0f : 0.55f);
    addShake(enemy.isBoss ? 1.0f : 0.18f);

    if (enemy.isBoss) {
        requestHitstop(0.1f);
        explosions_.push_back({enemy.position, 160.0f, 0.8f, 0.8f});
        // Boss 必掉两个道具
        spawnPickup(PickupKind::Health, enemy.position + Vec2{-30.0f, 0.0f});
        spawnPickup(rng().chance(0.5f) ? PickupKind::Damage : PickupKind::Grenade,
                    enemy.position + Vec2{30.0f, 0.0f});
    }

    // 史莱姆分裂
    if (stats.canSplit) {
        for (int i = 0; i < 2; ++i) {
            const Vec2 offset = fromAngle(rng().range(0.0f, kTau), enemy.radius * 0.8f);
            Enemy mini = makeEnemy(EnemyKind::SlimeMini, enemy.position + offset, enemy.wave);
            mini.spawnAnim = 0.4f;
            mini.knockback = offset.normalized() * 190.0f;
            enemies_.push_back(mini);
        }
    }

    rollDrop(enemy);
}

void World::rollDrop(const Enemy& enemy)
{
    float chance = cfg::kPickupDropChance;
    if (enemy.kind == EnemyKind::Brute) chance = 0.5f;
    if (enemy.isBoss) return; // Boss 的掉落已在死亡时单独处理
    if (!rng().chance(chance)) return;

    float weightHealth = 1.6f;
    float weightDamage = 1.3f;
    float weightRate = 1.3f;
    float weightSpeed = 1.0f;
    float weightGrenade = 0.9f;
    if (player_.health <= 2) weightHealth *= 3.0f;
    if (player_.grenades == 0) weightGrenade *= 3.5f;

    const float total = weightHealth + weightDamage + weightRate + weightSpeed + weightGrenade;
    float roll = rng().range(0.0f, total);
    PickupKind kind = PickupKind::Health;
    if ((roll -= weightHealth) <= 0.0f) kind = PickupKind::Health;
    else if ((roll -= weightDamage) <= 0.0f) kind = PickupKind::Damage;
    else if ((roll -= weightRate) <= 0.0f) kind = PickupKind::FireRate;
    else if ((roll -= weightSpeed) <= 0.0f) kind = PickupKind::Speed;
    else kind = PickupKind::Grenade;

    spawnPickup(kind, enemy.position);
}

// ------------------------------------------------------------ 生成与服务 ----
void World::spawnPlayerBullet(Vec2 position, Vec2 direction, int damage)
{
    Bullet& bullet = bullets_.spawn();
    bullet.position = position;
    bullet.velocity = direction.normalized() * cfg::kBulletSpeed;
    bullet.life = cfg::kBulletLife;
    bullet.radius = cfg::kBulletRadius;
    bullet.damage = damage;
    bullet.hostile = false;
    bullet.color = palette::kGold;
    // 攻击力堆高后子弹变粗、可穿透
    bullet.radius += static_cast<float>(damage - cfg::kBulletBaseDamage) * 1.2f;
    bullet.pierce = damage >= 4 ? 1 : 0;
}

void World::spawnEnemyBullet(Vec2 position, Vec2 velocity, int damage)
{
    Bullet& bullet = enemyBullets_.spawn();
    bullet.position = position;
    bullet.velocity = velocity;
    bullet.life = 3.0f;
    bullet.radius = 7.0f;
    bullet.damage = damage;
    bullet.hostile = true;
    bullet.color = palette::kSpitter;
}

void World::spawnEnemy(EnemyKind kind, Vec2 position)
{
    if (static_cast<int>(enemies_.size()) >= cfg::kMaxAliveEnemies + 6) return;
    enemies_.push_back(makeEnemy(kind, position, std::max(1, wave())));
}

void World::spawnEnemyAtEdge(EnemyKind kind)
{
    const Rect arena = this->arena();
    const float margin = 34.0f + enemyStats(kind).radius;
    Vec2 position;
    switch (rng().rangeInt(0, 3)) {
    case 0: position = {rng().range(0.0f, arena.w), -margin}; break;
    case 1: position = {arena.w + margin, rng().range(0.0f, arena.h)}; break;
    case 2: position = {rng().range(0.0f, arena.w), arena.h + margin}; break;
    default: position = {-margin, rng().range(0.0f, arena.h)}; break;
    }

    // 别在玩家脸上刷怪
    const Vec2 toPlayer = player_.position - position;
    if (toPlayer.length() < 220.0f) {
        position = arena.center() - toPlayer.normalized() * (arena.w * 0.55f);
        position.x = clampf(position.x, -margin, arena.w + margin);
        position.y = clampf(position.y, -margin, arena.h + margin);
    }

    spawnEnemy(kind, position);
    // 出生点预警光
    particles_.burst(position, 10, 40.0f, 140.0f, 0.2f, 0.45f, 4.0f, 10.0f, enemyStats(kind).accent);
    particles_.shockwave(position, 6.0f, 60.0f, enemyStats(kind).accent, 0.45f);
}

void World::spawnPickup(PickupKind kind, Vec2 position)
{
    Pickup pickup;
    pickup.kind = kind;
    pickup.position = position;
    pickup.life = cfg::kPickupLife;
    pickup.phase = rng().range(0.0f, kTau);
    pickup.alive = true;
    pickups_.push_back(pickup);
}

void World::throwGrenade(Vec2 position, Vec2 velocity)
{
    Grenade grenade;
    grenade.position = position;
    grenade.velocity = velocity;
    grenade.fuse = cfg::kGrenadeFuse;
    grenade.spinSpeed = rng().range(-10.0f, 10.0f);
    grenade.alive = true;
    grenades_.push_back(grenade);
}

void World::explodeAt(Vec2 position, float radius, int damage, bool fromPlayer)
{
    explosions_.push_back({position, radius, 0.45f, 0.45f});

    particles_.burst(position, 34, 120.0f, 520.0f, 0.25f, 0.75f, 6.0f, 22.0f, palette::kGold);
    particles_.burst(position, 18, 60.0f, 260.0f, 0.3f, 0.9f, 8.0f, 26.0f, palette::kDanger);
    particles_.shockwave(position, radius * 0.25f, radius * 1.25f, palette::kGold, 0.45f);
    particles_.shockwave(position, radius * 0.1f, radius * 0.85f, palette::kWhite, 0.3f);
    for (int i = 0; i < 12; ++i) {
        Particle shard;
        shard.position = position;
        shard.velocity = rng().direction() * rng().range(120.0f, 420.0f);
        shard.maxLife = rng().range(0.3f, 0.8f);
        shard.life = shard.maxLife;
        shard.size = rng().range(4.0f, 9.0f);
        shard.sizeEnd = 1.0f;
        shard.color = palette::kGold;
        shard.additive = false;
        shard.drag = 2.4f;
        shard.shape = 1;
        shard.rotation = rng().range(0.0f, kTau);
        shard.spin = rng().range(-12.0f, 12.0f);
        particles_.spawn(shard);
    }

    addShake(0.85f);
    requestHitstop(0.05f);
    playSound(Sfx::Explosion, 1.0f);

    if (!fromPlayer) return;

    for (Enemy& enemy : enemies_) {
        if (!enemy.alive) continue;
        const float distance = ls::distance(enemy.position, position);
        const float factor = falloffFactor(distance, radius + enemy.radius);
        if (factor <= 0.0f) continue;
        const int applied = std::max(1, static_cast<int>(static_cast<float>(damage) * factor + 0.5f));
        const bool killed = damageEnemy(enemy, applied);
        enemy.knockback += (enemy.position - position).normalized() * (380.0f * factor);
        spawnFloatingText(toText(applied), enemy.position + Vec2{0.0f, -enemy.radius - 8.0f}, palette::kGold, 0.8f);
        if (killed) {
            onEnemyDeath(enemy);
            enemy.alive = false;
        }
    }

    // 爆炸会清除范围内的敌方弹幕
    for (Bullet& bullet : enemyBullets_.all()) {
        if (!bullet.alive) continue;
        if (distance(bullet.position, position) < radius * 1.1f) {
            bullet.alive = false;
            particles_.burst(bullet.position, 3, 40.0f, 120.0f, 0.1f, 0.25f, 3.0f, 6.0f, palette::kSpitter);
        }
    }
    enemyBullets_.removeDead();
}

void World::spawnFloatingText(const std::wstring& text, Vec2 position, Rgba color, float scale)
{
    if (floatingTexts_.size() >= 96) return;
    FloatingText floating;
    floating.position = position;
    floating.velocity = {rng().range(-18.0f, 18.0f), -62.0f};
    floating.maxLife = cfg::kDamageTextLife * (scale > 1.0f ? 1.4f : 1.0f);
    floating.life = floating.maxLife;
    floating.color = color;
    floating.scale = scale;
    floating.text = text;
    floatingTexts_.push_back(floating);
}

void World::addShake(float amount) { shakeTrauma_ = clampf(shakeTrauma_ + amount, 0.0f, 1.0f); }

void World::requestHitstop(float seconds) { hitstopTimer_ = std::max(hitstopTimer_, seconds); }

void World::playSound(Sfx sfx, float volume) { audio().play(sfx, volume); }

void World::addScore(int amount) { stats_.score += std::max(0, amount); }


int World::enemiesRemaining() const
{
    int count = 0;
    for (const Enemy& enemy : enemies_) {
        if (enemy.alive) ++count;
    }
    return count;
}

void World::debugKillPlayer()
{
    int guard = 0;
    while (player_.alive && guard < 32) {
        player_.invulnTimer = 0.0f;
        player_.dashInvulnTimer = 0.0f;
        damagePlayer(1, player_.position + Vec2{1.0f, 0.0f});
        ++guard;
    }
}

int World::comboTier() const
{
    const int tier = 1 + combo_ / 10;
    return clampi(tier, 1, cfg::kComboMaxMultiplier);
}

float World::comboRatio() const
{
    if (combo_ <= 0) return 0.0f;
    return saturate(comboTimer_ / cfg::kComboWindow);
}

} // namespace ls
