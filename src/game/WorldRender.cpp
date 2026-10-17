// =============================================================================
//  WorldRender.cpp  —  World 的表现层（绘制）
//  说明：这里只做「把状态画出来」，不修改任何游戏状态。
//        模拟逻辑全部在 World.cpp，因此 --selftest 完全不依赖这一层。
// =============================================================================
#include <algorithm>
#include <cmath>

#include "core/Random.h"
#include "gfx/Assets.h"
#include "game/World.h"
#include "ui/Text.h"

namespace ls {

// ------------------------------------------------------------------ 绘制 ----
void World::draw(Canvas& canvas)
{
    const Vec2 cam = shakeOffset_;
    const int camX = iround(cam.x);
    const int camY = iround(cam.y);

    // 背景和战场边框一起跟随震屏，否则抖动时会出现"地板不动、角色乱晃"的割裂感
    canvas.blitAt(assets().background, camX, camY);

    drawPickups(canvas);
    drawGrenades(canvas);
    drawEnemies(canvas);
    drawPlayer(canvas);
    drawBullets(canvas);
    drawExplosions(canvas);
    particles_.draw(canvas);

    // 战场边框（画在实体之上，让越界的敌人被"框"压住一点，视觉更整齐）
    drawArenaFrame(canvas);

    // 暗角 + 低血量红晕
    canvas.blitAt(assets().vignette, camX, camY);
    if (player_.health <= 2 && player_.alive) {
        const float pulse = 0.22f + 0.16f * std::sin(static_cast<float>(stats_.elapsed) * 6.0f);
        canvas.fillScreen(withAlpha(palette::kDangerDeep, pulse));
    }
    if (player_.hitFlash > 0.0f) {
        canvas.fillScreen(withAlpha(palette::kDanger, player_.hitFlash * 0.28f));
    }

    drawFloatingTexts(canvas);
}

void World::drawArenaFrame(Canvas& canvas)
{
    const Rect base = arena();
    const Rect arena{base.x + shakeOffset_.x, base.y + shakeOffset_.y, base.w, base.h};
    const Rgba ink = rgba(8, 12, 20, 190);

    // 战场外压暗：四条边
    canvas.fillRect({0.0f, 0.0f, static_cast<float>(cfg::kScreenWidth), arena.top()}, ink);
    canvas.fillRect({0.0f, arena.bottom(), static_cast<float>(cfg::kScreenWidth),
                     static_cast<float>(cfg::kScreenHeight) - arena.bottom()}, ink);
    canvas.fillRect({0.0f, arena.top(), arena.left(), arena.h}, ink);
    canvas.fillRect({arena.right(), arena.top(), static_cast<float>(cfg::kScreenWidth) - arena.right(), arena.h}, ink);

    // 内阴影：让战场有"下沉"的深度
    const float shadow = 16.0f;
    for (int i = 0; i < 4; ++i) {
        const float t = static_cast<float>(i) / 4.0f;
        const float alpha = (1.0f - t) * 0.10f;
        canvas.fillRect({arena.left(), arena.top() + static_cast<float>(i) * (shadow / 4.0f), arena.w,
                         shadow / 4.0f}, withAlpha(palette::kBlack, alpha));
        canvas.fillRect({arena.left(), arena.bottom() - static_cast<float>(i + 1) * (shadow / 4.0f), arena.w,
                         shadow / 4.0f}, withAlpha(palette::kBlack, alpha));
    }

    // 边框 + 四角
    const Rgba edge = withAlpha(palette::kAccent, 0.30f);
    canvas.fillRect({arena.left() - 2.0f, arena.top() - 2.0f, arena.w + 4.0f, 2.0f}, edge);
    canvas.fillRect({arena.left() - 2.0f, arena.bottom(), arena.w + 4.0f, 2.0f}, edge);
    canvas.fillRect({arena.left() - 2.0f, arena.top(), 2.0f, arena.h}, edge);
    canvas.fillRect({arena.right(), arena.top(), 2.0f, arena.h}, edge);

    const Rgba bracket = withAlpha(palette::kAccent, 0.75f);
    const float length = 22.0f;
    const float thick = 3.0f;
    // 左上
    canvas.fillRect({arena.left() - 2.0f, arena.top() - 2.0f, length, thick}, bracket);
    canvas.fillRect({arena.left() - 2.0f, arena.top() - 2.0f, thick, length}, bracket);
    // 右上
    canvas.fillRect({arena.right() - length + 2.0f, arena.top() - 2.0f, length, thick}, bracket);
    canvas.fillRect({arena.right(), arena.top() - 2.0f, thick, length}, bracket);
    // 左下
    canvas.fillRect({arena.left() - 2.0f, arena.bottom() - 1.0f, length, thick}, bracket);
    canvas.fillRect({arena.left() - 2.0f, arena.bottom() - length, thick, length}, bracket);
    // 右下
    canvas.fillRect({arena.right() - length + 2.0f, arena.bottom() - 1.0f, length, thick}, bracket);
    canvas.fillRect({arena.right(), arena.bottom() - length, thick, length}, bracket);

    // 地面中央的淡环，给空间一个视觉中心
    const Vec2 center = arena.center();
    canvas.strokeCircle(center, 120.0f, 1.0f, withAlpha(palette::kAccent, 0.10f));
    canvas.strokeCircle(center, 210.0f, 1.0f, withAlpha(palette::kAccent, 0.06f));
}

void World::drawEnemies(Canvas& canvas)
{
    const Vec2 cam = shakeOffset_;

    // 按 y 排序，靠下的后画，产生前后遮挡
    std::vector<const Enemy*> ordered;
    ordered.reserve(enemies_.size());
    for (const Enemy& enemy : enemies_) {
        if (enemy.alive) ordered.push_back(&enemy);
    }
    std::sort(ordered.begin(), ordered.end(), [](const Enemy* a, const Enemy* b) {
        return a->position.y < b->position.y;
    });

    for (const Enemy* ptr : ordered) {
        const Enemy& enemy = *ptr;
        const EnemyStats& stats = enemyStats(enemy.kind);
        const Vec2 position = enemy.position + cam;
        const float spawnT = saturate(enemy.spawnAnim);
        const float appear = easeOutBack(spawnT);

        // 地面影子 + 光环
        canvas.fillCircle({position.x, position.y + enemy.radius * 0.75f}, enemy.radius * 0.7f,
                          rgba(0, 0, 0, 110));
        const float ringPulse = 0.5f + 0.5f * std::sin(enemy.wobble);
        canvas.strokeCircle({position.x, position.y + enemy.radius * 0.75f}, enemy.radius * (0.85f + ringPulse * 0.06f),
                            2.0f, withAlpha(stats.accent, 0.35f + 0.2f * ringPulse));

        const Sprite& sprite = enemySprite(enemy.kind);
        if (sprite.empty()) continue;
        const float bob = std::sin(enemy.wobble * 1.6f) * 1.6f;
        const Vec2 drawPos{position.x, position.y + bob};

        if (spawnT < 1.0f) {
            // 出生时的能量拖影
            canvas.drawSprite(sprite, drawPos, appear, stats.accent, (1.0f - spawnT) * 0.55f, true);
        }
        canvas.blit(sprite, drawPos, spawnT);

        if (enemy.hitFlash > 0.0f) {
            canvas.drawSprite(sprite, drawPos, 1.0f, palette::kWhite, enemy.hitFlash * 0.85f, true);
        }

        // 血量条（Boss 的血条在 HUD 顶部，这里只给巨兽画）
        if (enemy.kind == EnemyKind::Brute) {
            const float width = 44.0f;
            const float height = 4.0f;
            const float top = position.y - sprite.height() * 0.5f - 8.0f;
            const float ratio = saturate(enemy.health / std::max(1.0f, enemy.maxHealth));
            canvas.fillRect({position.x - width * 0.5f - 1.0f, top - 1.0f, width + 2.0f, height + 2.0f},
                            rgba(0, 0, 0, 150));
            canvas.fillRect({position.x - width * 0.5f, top, width * ratio, height}, palette::kBrute);
        }
    }
}

void World::drawPlayer(Canvas& canvas)
{
    if (!player_.alive) return;
    const Vec2 cam = shakeOffset_;
    const Vec2 position = player_.position + cam;
    const Sprite& body = player_.facingLeft ? assets().heroLeft : assets().hero;

    // 影子
    canvas.fillCircle({position.x, position.y + player_.radius * 0.9f}, player_.radius * 0.85f, rgba(0, 0, 0, 120));

    // 无敌帧闪烁
    float alpha = 1.0f;
    if (player_.invulnTimer > 0.0f) {
        alpha = 0.55f + 0.45f * std::fabs(std::sin(player_.invulnTimer * 26.0f));
    }

    // 冲刺残影
    if (player_.dashTimer > 0.0f) {
        for (int i = 1; i <= 3; ++i) {
            const Vec2 ghost = position - player_.dashDirection * static_cast<float>(i) * 12.0f;
            canvas.drawSprite(body, ghost, 1.0f, palette::kAccent, 0.22f / static_cast<float>(i), true);
        }
    }

    // 武器：沿准星方向画一根枪管（正面视角的像素小人只做镜像，不旋转）
    const Vec2 aimDir = fromAngle(player_.aim);
    const Vec2 shoulder = position + aimDir * 6.0f;
    const Vec2 muzzle = position + aimDir * (player_.radius + 26.0f);
    canvas.line(shoulder, position + aimDir * (player_.radius + 14.0f), 8.0f, rgba(18, 22, 30));
    canvas.line(shoulder, position + aimDir * (player_.radius + 16.0f), 5.0f, rgba(112, 128, 150));
    canvas.fillCircle(position + aimDir * (player_.radius + 15.0f), 3.2f, rgba(206, 216, 232));

    canvas.blit(body, position, alpha);

    // 枪身压在人物之上，射击时才看得清朝向
    canvas.line(position + aimDir * 10.0f, muzzle, 6.0f, rgba(18, 22, 30));
    canvas.line(position + aimDir * 10.0f, muzzle, 3.2f, rgba(124, 140, 162));
    canvas.fillCircle(muzzle, 2.6f, rgba(206, 216, 232));

    // 受击白闪
    if (player_.hitFlash > 0.0f) {
        canvas.drawSprite(body, position, 1.0f, palette::kWhite, player_.hitFlash * 0.9f, true);
    }

    // 瞄准激光（半透明，帮助点射）
    for (int i = 0; i < 14; ++i) {
        const float t = static_cast<float>(i) / 14.0f;
        const Vec2 point = muzzle + aimDir * (18.0f + t * 210.0f);
        canvas.fillCircle(point, 1.6f, withAlpha(palette::kAccent, 0.30f * (1.0f - t)));
    }

    // 枪口火光
    if (player_.muzzleFlash > 0.0f) {
        const float strength = player_.muzzleFlash / 0.07f;
        canvas.drawSprite(assets().glowTight, muzzle, 1.4f * strength, palette::kGold, 0.95f * strength, true);
        canvas.drawSprite(assets().glowSoft, muzzle, 0.5f * strength, palette::kGold, 0.5f * strength, true);
    }

    // 装弹进度环
    if (player_.reloading) {
        const float progress = 1.0f - saturate(player_.reloadTimer / cfg::kReloadTime);
        const int dots = 22;
        for (int i = 0; i < dots; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(dots);
            const Vec2 point = position + fromAngle(-kPi * 0.5f + t * kTau, player_.radius + 12.0f);
            const bool on = t <= progress;
            canvas.fillCircle(point, on ? 2.4f : 1.6f,
                              on ? palette::kGold : withAlpha(palette::kTextDim, 0.35f));
        }
    }

    // 冲刺就绪提示
    if (player_.dashCooldown > 0.0f) {
        const float ratio = 1.0f - saturate(player_.dashCooldown / cfg::kDashCooldown);
        const int dots = 16;
        for (int i = 0; i < dots; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(dots);
            if (t > ratio) break;
            const Vec2 point = position + fromAngle(kPi * 0.5f + t * kTau, player_.radius + 20.0f);
            canvas.fillCircle(point, 1.4f, withAlpha(palette::kAccent, 0.5f));
        }
    }
}

void World::drawBullets(Canvas& canvas)
{
    const Vec2 cam = shakeOffset_;

    // 敌方子弹
    for (const Bullet& bullet : enemyBullets_.all()) {
        if (!bullet.alive) continue;
        const Vec2 position = bullet.position + cam;
        canvas.drawSprite(assets().glowSoft, position, 0.42f, palette::kSpitter, 0.9f, true);
        canvas.fillCircle(position, bullet.radius * 0.55f, palette::kWhite);
        canvas.strokeCircle(position, bullet.radius + 1.5f, 1.5f, withAlpha(palette::kSpitter, 0.8f));
    }

    // 玩家子弹
    for (const Bullet& bullet : bullets_.all()) {
        if (!bullet.alive) continue;
        const Vec2 position = bullet.position + cam;
        const Vec2 tail = position - bullet.velocity.normalized() * 18.0f;
        canvas.line(tail, position, bullet.radius * 1.4f, withAlpha(bullet.color, 0.35f));
        canvas.drawSprite(assets().glowTight, position, 0.55f + bullet.radius * 0.03f, bullet.color, 1.0f, true);
        canvas.fillCircle(position, bullet.radius * 0.5f, palette::kWhite);
    }
}

void World::drawPickups(Canvas& canvas)
{
    const Vec2 cam = shakeOffset_;
    for (const Pickup& pickup : pickups_) {
        if (!pickup.alive) continue;
        const float bob = std::sin(pickup.phase) * 3.5f;
        const Vec2 position = pickup.position + cam + Vec2{0.0f, bob};
        const Rgba color = pickupColor(pickup.kind);

        // 即将消失时闪烁
        float alpha = 1.0f;
        if (pickup.life < 3.0f) {
            alpha = 0.4f + 0.6f * std::fabs(std::sin(pickup.life * 9.0f));
        }

        canvas.fillCircle({position.x, position.y + 12.0f}, 9.0f, rgba(0, 0, 0, 90));
        // 深色圆角底 + 描边，让"美术图标"看起来像一个游戏内的道具徽章
        canvas.fillRoundRect({position.x - 13.0f, position.y - 13.0f, 26.0f, 26.0f}, 7.0f,
                             withAlpha(palette::kInk, 0.78f * alpha));
        canvas.drawSprite(assets().glowSoft, position, 0.36f, color, 0.34f * alpha, true);
        canvas.strokeCircle(position, 15.0f + std::sin(pickup.phase * 1.4f) * 1.5f, 1.6f,
                            withAlpha(color, 0.7f * alpha));
        canvas.drawSprite(pickupSprite(pickup.kind), position, 0.62f, palette::kWhite, alpha, false);
    }
}

void World::drawGrenades(Canvas& canvas)
{
    const Vec2 cam = shakeOffset_;
    for (const Grenade& grenade : grenades_) {
        if (!grenade.alive) continue;
        const Vec2 position = grenade.position + cam;
        const float flash = grenade.fuse < 0.25f ? (0.5f + 0.5f * std::sin(grenade.fuse * 90.0f)) : 0.0f;
        canvas.fillCircle({position.x, position.y + 8.0f}, 7.0f, rgba(0, 0, 0, 100));
        canvas.drawSprite(assets().grenadeIcon, position, 1.6f, palette::kWhite, 1.0f, false);
        canvas.drawSprite(assets().glowTight, position, 0.6f + flash * 0.6f,
                          flash > 0.0f ? palette::kDanger : palette::kGold, 0.8f, true);
    }
}

void World::drawExplosions(Canvas& canvas)
{
    const Vec2 cam = shakeOffset_;
    for (const ExplosionVisual& explosion : explosions_) {
        const float t = 1.0f - saturate(explosion.life / explosion.maxLife);
        const Vec2 position = explosion.position + cam;
        const float scale = 0.5f + t * 2.4f;
        canvas.drawSprite(assets().explosion, position, scale, palette::kWhite, (1.0f - t) * 0.9f, true);
        canvas.drawSprite(assets().glowSoft, position, 1.2f + t * 2.6f, palette::kGold, (1.0f - t) * 0.8f, true);
    }
}

void World::drawFloatingTexts(Canvas& canvas)
{
    const Vec2 cam = shakeOffset_;
    const Rect arenaRect = arena().inset(-6.0f);
    for (const FloatingText& text : floatingTexts_) {
        const float fade = saturate(text.life / text.maxLife);
        const Rgba color = lerpColor(rgba(24, 30, 42), text.color, fade);
        const Vec2 position = text.position + cam;
        // 飘出战场的数字直接不画，免得糊到 HUD 面板上
        if (!arenaRect.contains(position)) continue;
        ui::Font font = ui::fontMonoBold();
        font.size = text.scale > 1.1f ? 34 : (text.scale > 0.7f ? 22 : 17);
        ui::draw(text.text, position, font, withAlpha(color, 1.0f), ui::Align::Center, true);
    }
}

} // namespace ls
