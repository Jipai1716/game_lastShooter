// =============================================================================
//  gfx/Sprite.h  —  纯 CPU 像素图元
//  说明：EasyX 只负责「解码 PNG」和「开窗」，图片一旦读进来就变成这里的
//        Sprite（0xAARRGGBB 数组）。后续的抠图、缩放、旋转、染色全部自己算，
//        这样才能做到像素风放大不糊、旋转不带黑边、染色不用重画素材。
// =============================================================================
#pragma once

#include <string>
#include <vector>

#include "gfx/Color.h"

namespace ls {

class Sprite
{
public:
    Sprite() = default;

    int width() const { return width_; }
    int height() const { return height_; }
    bool empty() const { return width_ <= 0 || height_ <= 0; }
    Vec2 size() const { return {static_cast<float>(width_), static_cast<float>(height_)}; }

    const Rgba* pixels() const { return pixels_.data(); }
    Rgba* pixels() { return pixels_.data(); }

    // 越界返回全透明
    Rgba at(int x, int y) const;

    // ---------------------------------------------------------- 构造 ----
    // 从 PNG/BMP 读取，并把 keyColor（美术素材里的纯色底）抠成透明。
    static Sprite fromFile(const std::string& path, Rgba keyColor = kKeyDefault, bool useKey = true);
    // 推荐入口：读图 + 自动识别底色（从边框洪水填充），兼容白底/灰底两套素材
    static Sprite load(const std::string& path, int tolerance = 14);
    static Sprite fromRaw(int width, int height, std::vector<Rgba> pixels);

    static Sprite disc(float radius, Rgba color);
    static Sprite ring(float radius, float thickness, Rgba color);
    static Sprite glow(float radius, Rgba color, float falloff = 2.0f);
    static Sprite rect(int width, int height, Rgba color);

    // ---------------------------------------------------------- 变换 ----
    Sprite scaledNearest(int factor) const;
    Sprite scaledTo(int width, int height) const;
    Sprite tinted(Rgba multiply) const;      // 逐分量相乘
    Sprite silhouette(Rgba color) const;     // 保留形状，颜色全换成 color
    Sprite rotated(float radians) const;     // 最近邻反向采样旋转
    Sprite mirroredX() const;                // 水平镜像（朝向左右翻转用）
    Sprite outline(Rgba color, int thickness = 1) const; // 描边（用于高亮敌人）

    // 把与边框连通的背景色挖成透明（自动识别白底/灰底）
    void keyOutBackground(int tolerance = 14);

    void fill(Rgba color);

    // 素材里统一使用 #C3C3C3 作为“透明”底色
    static constexpr Rgba kKeyDefault = 0xFFC3C3C3u;

private:
    int width_ = 0;
    int height_ = 0;
    std::vector<Rgba> pixels_;
};

} // namespace ls
