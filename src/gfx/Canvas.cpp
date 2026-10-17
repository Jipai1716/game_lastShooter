#include "gfx/Canvas.h"

#include <easyx.h>
#include <windows.h>

#include <algorithm>
#include <cmath>

#include "core/Log.h"

namespace ls {

namespace {

inline int clampInt(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

// 0..255 的整数混合权重
inline int scaleAlpha(int srcAlpha, int globalAlpha)
{
    return globalAlpha >= 255 ? srcAlpha : (srcAlpha * globalAlpha + 127) / 255;
}

} // namespace

Canvas::~Canvas() { destroy(); }

bool Canvas::create(int width, int height, const wchar_t* title)
{
    if (open_) return true;
    HWND hwnd = initgraph(width, height, 0);
    if (hwnd == nullptr) {
        LS_LOG("Canvas::create failed");
        return false;
    }
    if (title != nullptr) SetWindowTextW(hwnd, title);
    setbkmode(TRANSPARENT);
    width_ = width;
    height_ = height;
    open_ = true;
    BeginBatchDraw();
    syncSurface();
    resetClip();
    return pixels_ != nullptr;
}

void Canvas::destroy()
{
    if (!open_) return;
    EndBatchDraw();
    closegraph();
    open_ = false;
    pixels_ = nullptr;
    width_ = height_ = 0;
}

void Canvas::syncSurface()
{
    pixels_ = reinterpret_cast<Rgba*>(GetImageBuffer(nullptr));
    width_ = getwidth();
    height_ = getheight();
    resetClip();
}

void Canvas::resetClip()
{
    clipLeft_ = 0;
    clipTop_ = 0;
    clipRight_ = width_;
    clipBottom_ = height_;
}

void Canvas::setClip(const Rect& rect)
{
    clipLeft_ = clampInt(static_cast<int>(std::floor(rect.left())), 0, width_);
    clipTop_ = clampInt(static_cast<int>(std::floor(rect.top())), 0, height_);
    clipRight_ = clampInt(static_cast<int>(std::ceil(rect.right())), 0, width_);
    clipBottom_ = clampInt(static_cast<int>(std::ceil(rect.bottom())), 0, height_);
}

void Canvas::clearClip() { resetClip(); }

void Canvas::beginFrame(Rgba clearColor)
{
    syncSurface();
    if (pixels_ == nullptr) return;
    const Rgba color = clearColor | 0xFF000000u;
    std::fill(pixels_, pixels_ + static_cast<size_t>(width_) * height_, color);
}

void Canvas::endFrame() { FlushBatchDraw(); }

// ---------------------------------------------------------------- 混合 ----
Rgba Canvas::blendPixel(Rgba dst, Rgba src, int alpha)
{
    if (alpha >= 255) return src | 0xFF000000u;
    const int ia = 255 - alpha;
    const int r = (redOf(src) * alpha + redOf(dst) * ia) / 255;
    const int g = (greenOf(src) * alpha + greenOf(dst) * ia) / 255;
    const int b = (blueOf(src) * alpha + blueOf(dst) * ia) / 255;
    return rgba(r, g, b);
}

Rgba Canvas::addPixel(Rgba dst, Rgba src, int alpha)
{
    const int r = std::min(255, redOf(dst) + redOf(src) * alpha / 255);
    const int g = std::min(255, greenOf(dst) + greenOf(src) * alpha / 255);
    const int b = std::min(255, blueOf(dst) + blueOf(src) * alpha / 255);
    return rgba(r, g, b);
}

// ---------------------------------------------------------------- 位图 ----
namespace {

template <bool kAdditive>
void blitImpl(Rgba* dstPixels, int dstW, int dstH, int clipL, int clipT, int clipR, int clipB,
              const Sprite& sprite, int left, int top, int globalAlpha)
{
    if (sprite.empty() || globalAlpha <= 0) return;

    const int x0 = std::max(left, clipL);
    const int y0 = std::max(top, clipT);
    const int x1 = std::min(left + sprite.width(), clipR);
    const int y1 = std::min(top + sprite.height(), clipB);
    if (x0 >= x1 || y0 >= y1) return;

    const Rgba* srcPixels = sprite.pixels();
    const int srcW = sprite.width();

    for (int y = y0; y < y1; ++y) {
        Rgba* dst = dstPixels + static_cast<size_t>(y) * dstW + x0;
        const Rgba* src = srcPixels + static_cast<size_t>(y - top) * srcW + (x0 - left);
        for (int x = x0; x < x1; ++x, ++dst, ++src) {
            const Rgba sp = *src;
            int sa = static_cast<int>((sp >> 24) & 0xFF);
            if (sa == 0) continue;
            sa = scaleAlpha(sa, globalAlpha);
            if (sa == 0) continue;
            if constexpr (kAdditive) {
                *dst = Canvas::addPixel(*dst, sp, sa);
            } else {
                *dst = Canvas::blendPixel(*dst, sp, sa);
            }
        }
    }
}

} // namespace

void Canvas::blit(const Sprite& sprite, Vec2 center, float alpha)
{
    const int left = iround(center.x - sprite.width() * 0.5f);
    const int top = iround(center.y - sprite.height() * 0.5f);
    blitAt(sprite, left, top, alpha);
}

void Canvas::blitAt(const Sprite& sprite, int left, int top, float alpha)
{
    if (pixels_ == nullptr) return;
    const int a = static_cast<int>(saturate(alpha) * 255.0f + 0.5f);
    blitImpl<false>(pixels_, width_, height_, clipLeft_, clipTop_, clipRight_, clipBottom_, sprite, left, top, a);
}

void Canvas::blitAdd(const Sprite& sprite, Vec2 center, float alpha)
{
    const int left = iround(center.x - sprite.width() * 0.5f);
    const int top = iround(center.y - sprite.height() * 0.5f);
    blitAddAt(sprite, left, top, alpha);
}

void Canvas::blitAddAt(const Sprite& sprite, int left, int top, float alpha)
{
    if (pixels_ == nullptr) return;
    const int a = static_cast<int>(saturate(alpha) * 255.0f + 0.5f);
    blitImpl<true>(pixels_, width_, height_, clipLeft_, clipTop_, clipRight_, clipBottom_, sprite, left, top, a);
}

void Canvas::drawSprite(const Sprite& sprite, Vec2 center, float scale, Rgba tint, float alpha, bool additive)
{
    if (pixels_ == nullptr || sprite.empty() || scale <= 0.0f) return;
    const int globalAlpha = static_cast<int>(saturate(alpha) * 255.0f + 0.5f);
    if (globalAlpha == 0) return;

    const bool plain = (scale == 1.0f) && ((tint & 0x00FFFFFFu) == 0x00FFFFFFu);
    if (plain) {
        const int left = iround(center.x - sprite.width() * 0.5f);
        const int top = iround(center.y - sprite.height() * 0.5f);
        if (additive) blitImpl<true>(pixels_, width_, height_, clipLeft_, clipTop_, clipRight_, clipBottom_, sprite,
                                    left, top, globalAlpha);
        else blitImpl<false>(pixels_, width_, height_, clipLeft_, clipTop_, clipRight_, clipBottom_, sprite, left,
                             top, globalAlpha);
        return;
    }

    const int dstW = std::max(1, iround(sprite.width() * scale));
    const int dstH = std::max(1, iround(sprite.height() * scale));
    const int left = iround(center.x - dstW * 0.5f);
    const int top = iround(center.y - dstH * 0.5f);

    const int x0 = std::max(left, clipLeft_);
    const int y0 = std::max(top, clipTop_);
    const int x1 = std::min(left + dstW, clipRight_);
    const int y1 = std::min(top + dstH, clipBottom_);
    if (x0 >= x1 || y0 >= y1) return;

    const float invScale = 1.0f / scale;
    const int srcW = sprite.width();
    const int srcH = sprite.height();
    const Rgba* srcPixels = sprite.pixels();

    for (int y = y0; y < y1; ++y) {
        const int sy = clampi(static_cast<int>((y - top) * invScale), 0, srcH - 1);
        Rgba* dst = pixels_ + static_cast<size_t>(y) * width_;
        const Rgba* srcRow = srcPixels + static_cast<size_t>(sy) * srcW;
        for (int x = x0; x < x1; ++x) {
            const int sx = clampi(static_cast<int>((x - left) * invScale), 0, srcW - 1);
            const Rgba sp = mulColor(srcRow[sx], tint);
            int sa = alphaOf(sp);
            if (sa == 0) continue;
            sa = scaleAlpha(sa, globalAlpha);
            if (sa == 0) continue;
            if (additive) dst[x] = addPixel(dst[x], sp, sa);
            else dst[x] = blendPixel(dst[x], sp, sa);
        }
    }
}

// ---------------------------------------------------------------- 图元 ----
void Canvas::fillScreen(Rgba color)
{
    const int a = alphaOf(color);
    if (a <= 0 || pixels_ == nullptr) return;
    const size_t count = static_cast<size_t>(width_) * height_;
    for (size_t i = 0; i < count; ++i) {
        pixels_[i] = blendPixel(pixels_[i], color, a);
    }
}

void Canvas::fillRect(const Rect& rect, Rgba color)
{
    if (pixels_ == nullptr) return;
    const int x0 = std::max(static_cast<int>(std::floor(rect.left())), clipLeft_);
    const int y0 = std::max(static_cast<int>(std::floor(rect.top())), clipTop_);
    const int x1 = std::min(static_cast<int>(std::ceil(rect.right())), clipRight_);
    const int y1 = std::min(static_cast<int>(std::ceil(rect.bottom())), clipBottom_);
    if (x0 >= x1 || y0 >= y1) return;

    const int a = alphaOf(color);
    if (a >= 255) {
        const Rgba solid = color | 0xFF000000u;
        for (int y = y0; y < y1; ++y) {
            Rgba* row = pixels_ + static_cast<size_t>(y) * width_;
            std::fill(row + x0, row + x1, solid);
        }
        return;
    }
    for (int y = y0; y < y1; ++y) {
        Rgba* row = pixels_ + static_cast<size_t>(y) * width_ + x0;
        for (int x = x0; x < x1; ++x, ++row) {
            *row = blendPixel(*row, color, a);
        }
    }
}

void Canvas::fillRoundRect(const Rect& rect, float radius, Rgba color)
{
    const float r = clampf(radius, 0.0f, std::min(rect.w, rect.h) * 0.5f);
    if (r <= 0.5f) {
        fillRect(rect, color);
        return;
    }
    // 中间横条 + 上下两条 + 四角圆
    fillRect({rect.x + r, rect.y, rect.w - r * 2.0f, rect.h}, color);
    fillRect({rect.x, rect.y + r, r, rect.h - r * 2.0f}, color);
    fillRect({rect.x + rect.w - r, rect.y + r, r, rect.h - r * 2.0f}, color);
    fillCircle({rect.x + r, rect.y + r}, r, color);
    fillCircle({rect.x + rect.w - r, rect.y + r}, r, color);
    fillCircle({rect.x + r, rect.y + rect.h - r}, r, color);
    fillCircle({rect.x + rect.w - r, rect.y + rect.h - r}, r, color);
}

void Canvas::fillCircle(Vec2 center, float radius, Rgba color)
{
    if (pixels_ == nullptr || radius <= 0.0f) return;
    const int x0 = std::max(static_cast<int>(std::floor(center.x - radius)), clipLeft_);
    const int y0 = std::max(static_cast<int>(std::floor(center.y - radius)), clipTop_);
    const int x1 = std::min(static_cast<int>(std::ceil(center.x + radius)), clipRight_);
    const int y1 = std::min(static_cast<int>(std::ceil(center.y + radius)), clipBottom_);
    if (x0 >= x1 || y0 >= y1) return;

    const int a = alphaOf(color);
    const float r2 = radius * radius;
    for (int y = y0; y < y1; ++y) {
        const float dy = (y + 0.5f) - center.y;
        Rgba* row = pixels_ + static_cast<size_t>(y) * width_;
        for (int x = x0; x < x1; ++x) {
            const float dx = (x + 0.5f) - center.x;
            if (dx * dx + dy * dy > r2) continue;
            row[x] = a >= 255 ? (color | 0xFF000000u) : blendPixel(row[x], color, a);
        }
    }
}

void Canvas::fillRing(Vec2 center, float innerRadius, float outerRadius, Rgba color)
{
    if (pixels_ == nullptr || outerRadius <= 0.0f) return;
    const int x0 = std::max(static_cast<int>(std::floor(center.x - outerRadius)), clipLeft_);
    const int y0 = std::max(static_cast<int>(std::floor(center.y - outerRadius)), clipTop_);
    const int x1 = std::min(static_cast<int>(std::ceil(center.x + outerRadius)), clipRight_);
    const int y1 = std::min(static_cast<int>(std::ceil(center.y + outerRadius)), clipBottom_);
    if (x0 >= x1 || y0 >= y1) return;

    const int a = alphaOf(color);
    const float inner2 = innerRadius * innerRadius;
    const float outer2 = outerRadius * outerRadius;
    for (int y = y0; y < y1; ++y) {
        const float dy = (y + 0.5f) - center.y;
        Rgba* row = pixels_ + static_cast<size_t>(y) * width_;
        for (int x = x0; x < x1; ++x) {
            const float dx = (x + 0.5f) - center.x;
            const float d2 = dx * dx + dy * dy;
            if (d2 > outer2 || d2 < inner2) continue;
            row[x] = a >= 255 ? (color | 0xFF000000u) : blendPixel(row[x], color, a);
        }
    }
}

void Canvas::strokeCircle(Vec2 center, float radius, float thickness, Rgba color)
{
    fillRing(center, std::max(0.0f, radius - thickness * 0.5f), radius + thickness * 0.5f, color);
}

void Canvas::line(Vec2 a, Vec2 b, float thickness, Rgba color)
{
    const Vec2 delta = b - a;
    const float length = delta.length();
    if (length < 0.01f) return;
    const float step = std::max(0.5f, thickness * 0.4f);
    const int steps = static_cast<int>(length / step) + 1;
    const Vec2 dir = delta / length;
    const float half = thickness * 0.5f;
    for (int i = 0; i <= steps; ++i) {
        const Vec2 p = a + dir * (step * i);
        fillCircle(p, half, color);
    }
}

void Canvas::fillTriangle(Vec2 p0, Vec2 p1, Vec2 p2, Rgba color)
{
    if (pixels_ == nullptr) return;
    const int minX = static_cast<int>(std::floor(std::min({p0.x, p1.x, p2.x})));
    const int maxX = static_cast<int>(std::ceil(std::max({p0.x, p1.x, p2.x})));
    const int minY = static_cast<int>(std::floor(std::min({p0.y, p1.y, p2.y})));
    const int maxY = static_cast<int>(std::ceil(std::max({p0.y, p1.y, p2.y})));

    const int x0 = std::max(minX, clipLeft_);
    const int y0 = std::max(minY, clipTop_);
    const int x1 = std::min(maxX, clipRight_);
    const int y1 = std::min(maxY, clipBottom_);
    if (x0 >= x1 || y0 >= y1) return;

    auto edge = [](const Vec2& a, const Vec2& b, const Vec2& p) {
        return (p.x - a.x) * (b.y - a.y) - (p.y - a.y) * (b.x - a.x);
    };
    const int alpha = alphaOf(color);
    for (int y = y0; y < y1; ++y) {
        Rgba* row = pixels_ + static_cast<size_t>(y) * width_;
        for (int x = x0; x < x1; ++x) {
            const Vec2 p{x + 0.5f, y + 0.5f};
            const float w0 = edge(p0, p1, p);
            const float w1 = edge(p1, p2, p);
            const float w2 = edge(p2, p0, p);
            if ((w0 < 0 || w1 < 0 || w2 < 0) && (w0 > 0 || w1 > 0 || w2 > 0)) continue;
            row[x] = alpha >= 255 ? (color | 0xFF000000u) : blendPixel(row[x], color, alpha);
        }
    }
}

// ------------------------------------------------------------ 整屏效果 ----
void Canvas::darken(float amount)
{
    if (pixels_ == nullptr) return;
    const float f = 1.0f - saturate(amount);
    const size_t count = static_cast<size_t>(width_) * height_;
    for (size_t i = 0; i < count; ++i) {
        const Rgba p = pixels_[i];
        pixels_[i] = rgba(static_cast<int>(redOf(p) * f), static_cast<int>(greenOf(p) * f),
                          static_cast<int>(blueOf(p) * f));
    }
}

void Canvas::desaturate(float amount)
{
    if (pixels_ == nullptr) return;
    const float f = saturate(amount);
    const size_t count = static_cast<size_t>(width_) * height_;
    for (size_t i = 0; i < count; ++i) {
        const Rgba p = pixels_[i];
        const int gray = (redOf(p) * 30 + greenOf(p) * 59 + blueOf(p) * 11) / 100;
        const int r = static_cast<int>(lerpf(redOf(p), gray, f));
        const int g = static_cast<int>(lerpf(greenOf(p), gray, f));
        const int b = static_cast<int>(lerpf(blueOf(p), gray, f));
        pixels_[i] = rgba(r, g, b);
    }
}

} // namespace ls
