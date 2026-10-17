// =============================================================================
//  gfx/Canvas.h  —  软件渲染画布
//  说明：EasyX 只提供一个窗口和一块 32 位显示缓冲，这里是自写的混合器。
//        好处：alpha 混合、加法混合（发光）、裁剪全在自己手里，行为可预期，
//        且不依赖 GDI 的 putimage 语义。文字仍走 GDI（渲染质量更好）。
// =============================================================================
#pragma once

#include "gfx/Color.h"
#include "gfx/Sprite.h"

namespace ls {

class Canvas
{
public:
    Canvas() = default;
    ~Canvas();

    Canvas(const Canvas&) = delete;
    Canvas& operator=(const Canvas&) = delete;

    bool create(int width, int height, const wchar_t* title);
    void destroy();
    bool isOpen() const { return pixels_ != nullptr; }

    int width() const { return width_; }
    int height() const { return height_; }
    Rect bounds() const { return {0.0f, 0.0f, static_cast<float>(width_), static_cast<float>(height_)}; }
    Vec2 center() const { return {width_ * 0.5f, height_ * 0.5f}; }

    // 每帧：清屏 -> 各种绘制 -> 提交
    void beginFrame(Rgba clearColor = palette::kInk);
    void endFrame();

    void setClip(const Rect& rect);
    void clearClip();

    // ------------------------------------------------------------ 位图 ----
    void blit(const Sprite& sprite, Vec2 center, float alpha = 1.0f);
    void blitAt(const Sprite& sprite, int left, int top, float alpha = 1.0f);
    void blitAdd(const Sprite& sprite, Vec2 center, float alpha = 1.0f);
    void blitAddAt(const Sprite& sprite, int left, int top, float alpha = 1.0f);

    // 通用绘制：最近邻缩放 + 染色 + alpha + 混合模式（发光、粒子用）
    void drawSprite(const Sprite& sprite, Vec2 center, float scale = 1.0f, Rgba tint = rgba(255, 255, 255),
                    float alpha = 1.0f, bool additive = false);

    // ------------------------------------------------------------ 图元 ----
    void fillRect(const Rect& rect, Rgba color);
    void fillRoundRect(const Rect& rect, float radius, Rgba color);
    void fillCircle(Vec2 center, float radius, Rgba color);
    void fillRing(Vec2 center, float innerRadius, float outerRadius, Rgba color);
    void strokeCircle(Vec2 center, float radius, float thickness, Rgba color);
    void line(Vec2 a, Vec2 b, float thickness, Rgba color);
    void fillTriangle(Vec2 a, Vec2 b, Vec2 c, Rgba color);

    // ------------------------------------------------------- 整屏效果 ----
    void fillScreen(Rgba color);              // 半透明整屏叠加（含 alpha）
    void darken(float amount);                // 整屏压暗
    void desaturate(float amount);            // 整屏去色（死亡/暂停用）

    // 直接访问（粒子/特殊绘制用）
    Rgba* pixels() { return pixels_; }
    const Rgba* pixels() const { return pixels_; }
    int stride() const { return width_; }

    static Rgba blendPixel(Rgba dst, Rgba src, int alpha);
    static Rgba addPixel(Rgba dst, Rgba src, int alpha);

private:
    void syncSurface();
    void resetClip();

    int width_ = 0;
    int height_ = 0;
    Rgba* pixels_ = nullptr;

    int clipLeft_ = 0;
    int clipTop_ = 0;
    int clipRight_ = 0;
    int clipBottom_ = 0;

    bool open_ = false;
};

} // namespace ls
