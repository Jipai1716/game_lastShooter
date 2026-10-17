#include "gfx/Sprite.h"

#include <easyx.h>
#include <windows.h>

#include <algorithm>
#include <cmath>

namespace ls {

namespace {

std::wstring widen(const std::string& utf8)
{
    if (utf8.empty()) return std::wstring();
    const int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring result(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), &result[0], size);
    return result;
}

} // namespace

Rgba Sprite::at(int x, int y) const
{
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return 0u;
    return pixels_[static_cast<size_t>(y) * width_ + x];
}

Sprite Sprite::fromRaw(int width, int height, std::vector<Rgba> pixels)
{
    Sprite s;
    s.width_ = width;
    s.height_ = height;
    s.pixels_ = std::move(pixels);
    if (s.pixels_.size() != static_cast<size_t>(width) * height) {
        s.pixels_.resize(static_cast<size_t>(width) * height, 0u);
    }
    return s;
}

Sprite Sprite::load(const std::string& path, int tolerance)
{
    Sprite sprite = fromFile(path, 0u, false);
    sprite.keyOutBackground(tolerance);
    return sprite;
}

void Sprite::keyOutBackground(int tolerance)
{
    if (empty()) return;

    // 1) 统计四条边框上的颜色，出现最多的两种视为"背景色"
    struct ColorCount
    {
        Rgba color = 0u;
        int count = 0;
    };
    std::vector<ColorCount> counts;
    auto consider = [&counts](Rgba pixel) {
        const Rgba rgb = pixel & 0x00FFFFFFu;
        for (ColorCount& entry : counts) {
            if (entry.color == rgb) {
                ++entry.count;
                return;
            }
        }
        counts.push_back({rgb, 1});
    };
    for (int x = 0; x < width_; ++x) {
        consider(pixels_[static_cast<size_t>(x)]);
        consider(pixels_[static_cast<size_t>(height_ - 1) * width_ + x]);
    }
    for (int y = 0; y < height_; ++y) {
        consider(pixels_[static_cast<size_t>(y) * width_]);
        consider(pixels_[static_cast<size_t>(y) * width_ + width_ - 1]);
    }
    if (counts.empty()) return;
    std::sort(counts.begin(), counts.end(), [](const ColorCount& a, const ColorCount& b) {
        return a.count > b.count;
    });

    Rgba keys[2] = {counts[0].color, counts.size() > 1 ? counts[1].color : counts[0].color};

    auto matchesKey = [&keys, tolerance](Rgba pixel) {
        const Rgba rgb = pixel & 0x00FFFFFFu;
        for (const Rgba key : keys) {
            const int dr = std::abs(static_cast<int>((rgb >> 16) & 0xFF) - static_cast<int>((key >> 16) & 0xFF));
            const int dg = std::abs(static_cast<int>((rgb >> 8) & 0xFF) - static_cast<int>((key >> 8) & 0xFF));
            const int db = std::abs(static_cast<int>(rgb & 0xFF) - static_cast<int>(key & 0xFF));
            if (dr + dg + db <= tolerance * 3) return true;
        }
        return false;
    };

    // 2) 从边框做洪水填充，只挖掉与边框连通的背景
    std::vector<std::uint8_t> visited(static_cast<size_t>(width_) * height_, 0);
    std::vector<std::pair<int, int>> stack;
    stack.reserve(static_cast<size_t>(width_) * height_ / 4 + 16);

    auto trySeed = [&](int x, int y) {
        const size_t index = static_cast<size_t>(y) * width_ + x;
        if (visited[index]) return;
        if (!matchesKey(pixels_[index])) return;
        visited[index] = 1;
        stack.emplace_back(x, y);
    };
    for (int x = 0; x < width_; ++x) {
        trySeed(x, 0);
        trySeed(x, height_ - 1);
    }
    for (int y = 0; y < height_; ++y) {
        trySeed(0, y);
        trySeed(width_ - 1, y);
    }

    while (!stack.empty()) {
        const auto [x, y] = stack.back();
        stack.pop_back();
        pixels_[static_cast<size_t>(y) * width_ + x] = 0u; // 全透明

        const int neighbours[4][2] = {{x - 1, y}, {x + 1, y}, {x, y - 1}, {x, y + 1}};
        for (const auto& n : neighbours) {
            const int nx = n[0];
            const int ny = n[1];
            if (nx < 0 || ny < 0 || nx >= width_ || ny >= height_) continue;
            const size_t index = static_cast<size_t>(ny) * width_ + nx;
            if (visited[index]) continue;
            if (!matchesKey(pixels_[index])) continue;
            visited[index] = 1;
            stack.emplace_back(nx, ny);
        }
    }
}

Sprite Sprite::fromFile(const std::string& path, Rgba keyColor, bool useKey)
{
    Sprite s;
    IMAGE image;
    if (loadimage(&image, widen(path).c_str()) != 0) {
        return s; // 空图，调用方可据此报错
    }
    s.width_ = image.getwidth();
    s.height_ = image.getheight();
    if (s.width_ <= 0 || s.height_ <= 0) return s;

    const size_t count = static_cast<size_t>(s.width_) * s.height_;
    s.pixels_.assign(count, 0u);

    const DWORD* buffer = GetImageBuffer(&image);
    if (buffer == nullptr) {
        s.width_ = s.height_ = 0;
        return s;
    }
    const Rgba key = keyColor & 0x00FFFFFFu;
    for (size_t i = 0; i < count; ++i) {
        const Rgba p = buffer[i];
        if (useKey && (p & 0x00FFFFFFu) == key) {
            s.pixels_[i] = 0u;             // 底色 -> 全透明
        } else {
            s.pixels_[i] = p | 0xFF000000u; // 强制不透明（素材没有真正的 alpha）
        }
    }
    return s;
}

void Sprite::fill(Rgba color)
{
    std::fill(pixels_.begin(), pixels_.end(), color);
}

Sprite Sprite::disc(float radius, Rgba color)
{
    const int r = std::max(1, static_cast<int>(std::ceil(radius)));
    const int size = r * 2 + 1;
    std::vector<Rgba> px(static_cast<size_t>(size) * size, 0u);
    const float r2 = radius * radius;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const float dx = x - r;
            const float dy = y - r;
            const float d2 = dx * dx + dy * dy;
            if (d2 <= r2) {
                // 边缘 1px 软化
                const float edge = radius - std::sqrt(d2);
                const float a = saturate(edge + 0.5f);
                px[static_cast<size_t>(y) * size + x] = withAlpha(color, a * alphaOf(color) / 255.0f);
            }
        }
    }
    return fromRaw(size, size, std::move(px));
}

Sprite Sprite::ring(float radius, float thickness, Rgba color)
{
    const int r = std::max(2, static_cast<int>(std::ceil(radius)) + 2);
    const int size = r * 2 + 1;
    std::vector<Rgba> px(static_cast<size_t>(size) * size, 0u);
    const float outer = radius + thickness * 0.5f;
    const float inner = std::max(0.0f, radius - thickness * 0.5f);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const float dx = x - r;
            const float dy = y - r;
            const float d = std::sqrt(dx * dx + dy * dy);
            if (d <= outer && d >= inner) {
                const float edge = std::min(outer - d, d - inner) + 0.5f;
                px[static_cast<size_t>(y) * size + x] = withAlpha(color, saturate(edge) * alphaOf(color) / 255.0f);
            }
        }
    }
    return fromRaw(size, size, std::move(px));
}

Sprite Sprite::glow(float radius, Rgba color, float falloff)
{
    const int r = std::max(1, static_cast<int>(std::ceil(radius)));
    const int size = r * 2 + 1;
    std::vector<Rgba> px(static_cast<size_t>(size) * size, 0u);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const float dx = (x - r) / radius;
            const float dy = (y - r) / radius;
            const float d = std::sqrt(dx * dx + dy * dy);
            const float a = std::pow(saturate(1.0f - d), falloff);
            px[static_cast<size_t>(y) * size + x] = withAlpha(color, a * alphaOf(color) / 255.0f);
        }
    }
    return fromRaw(size, size, std::move(px));
}

Sprite Sprite::rect(int width, int height, Rgba color)
{
    const int w = std::max(1, width);
    const int h = std::max(1, height);
    std::vector<Rgba> px(static_cast<size_t>(w) * h, color);
    return fromRaw(w, h, std::move(px));
}

Sprite Sprite::scaledNearest(int factor) const
{
    if (factor <= 1 || empty()) return *this;
    const int w = width_ * factor;
    const int h = height_ * factor;
    std::vector<Rgba> px(static_cast<size_t>(w) * h, 0u);
    for (int y = 0; y < h; ++y) {
        const int sy = y / factor;
        for (int x = 0; x < w; ++x) {
            px[static_cast<size_t>(y) * w + x] = pixels_[static_cast<size_t>(sy) * width_ + (x / factor)];
        }
    }
    return fromRaw(w, h, std::move(px));
}

Sprite Sprite::scaledTo(int width, int height) const
{
    if (empty()) return Sprite();
    const int w = std::max(1, width);
    const int h = std::max(1, height);
    std::vector<Rgba> px(static_cast<size_t>(w) * h, 0u);
    for (int y = 0; y < h; ++y) {
        const int sy = static_cast<int>(static_cast<float>(y) / h * height_);
        for (int x = 0; x < w; ++x) {
            const int sx = static_cast<int>(static_cast<float>(x) / w * width_);
            px[static_cast<size_t>(y) * w + x] = pixels_[static_cast<size_t>(clampi(sy, 0, height_ - 1)) * width_ +
                                                           clampi(sx, 0, width_ - 1)];
        }
    }
    return fromRaw(w, h, std::move(px));
}

Sprite Sprite::tinted(Rgba multiply) const
{
    if (empty()) return *this;
    std::vector<Rgba> px(pixels_.size(), 0u);
    for (size_t i = 0; i < pixels_.size(); ++i) {
        const Rgba p = pixels_[i];
        if (alphaOf(p) == 0) continue;
        px[i] = mulColor(p, multiply);
    }
    return fromRaw(width_, height_, std::move(px));
}

Sprite Sprite::silhouette(Rgba color) const
{
    if (empty()) return *this;
    std::vector<Rgba> px(pixels_.size(), 0u);
    for (size_t i = 0; i < pixels_.size(); ++i) {
        const Rgba p = pixels_[i];
        if (alphaOf(p) == 0) continue;
        px[i] = withAlpha(color, alphaOf(p) / 255.0f * alphaOf(color) / 255.0f);
    }
    return fromRaw(width_, height_, std::move(px));
}

Sprite Sprite::rotated(float radians) const
{
    if (empty()) return *this;
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    const float extW = std::fabs(width_ * c) + std::fabs(height_ * s);
    const float extH = std::fabs(width_ * s) + std::fabs(height_ * c);
    const int newW = std::max(1, static_cast<int>(std::ceil(extW)));
    const int newH = std::max(1, static_cast<int>(std::ceil(extH)));

    std::vector<Rgba> px(static_cast<size_t>(newW) * newH, 0u);
    const float dstCx = newW * 0.5f;
    const float dstCy = newH * 0.5f;
    const float srcCx = width_ * 0.5f;
    const float srcCy = height_ * 0.5f;

    for (int y = 0; y < newH; ++y) {
        for (int x = 0; x < newW; ++x) {
            // 反向旋转采样：目标像素 -> 源像素
            const float rx = (x + 0.5f) - dstCx;
            const float ry = (y + 0.5f) - dstCy;
            const float sx = rx * c + ry * s + srcCx;
            const float sy = -rx * s + ry * c + srcCy;
            const int ix = static_cast<int>(std::floor(sx));
            const int iy = static_cast<int>(std::floor(sy));
            if (ix < 0 || iy < 0 || ix >= width_ || iy >= height_) continue;
            px[static_cast<size_t>(y) * newW + x] = pixels_[static_cast<size_t>(iy) * width_ + ix];
        }
    }
    return fromRaw(newW, newH, std::move(px));
}

Sprite Sprite::mirroredX() const
{
    if (empty()) return *this;
    std::vector<Rgba> px(pixels_.size(), 0u);
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            px[static_cast<size_t>(y) * width_ + (width_ - 1 - x)] = pixels_[static_cast<size_t>(y) * width_ + x];
        }
    }
    return fromRaw(width_, height_, std::move(px));
}

Sprite Sprite::outline(Rgba color, int thickness) const
{
    if (empty() || thickness <= 0) return *this;
    const int w = width_ + thickness * 2;
    const int h = height_ + thickness * 2;
    std::vector<Rgba> px(static_cast<size_t>(w) * h, 0u);

    auto opaqueAt = [this](int x, int y) {
        return alphaOf(at(x, y)) > 0;
    };

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int sx = x - thickness;
            const int sy = y - thickness;
            if (!opaqueAt(sx, sy)) {
                bool nearOpaque = false;
                for (int dy = -thickness; dy <= thickness && !nearOpaque; ++dy) {
                    for (int dx = -thickness; dx <= thickness; ++dx) {
                        if (dx * dx + dy * dy > thickness * thickness) continue;
                        if (opaqueAt(sx + dx, sy + dy)) {
                            nearOpaque = true;
                            break;
                        }
                    }
                }
                if (nearOpaque) px[static_cast<size_t>(y) * w + x] = color;
            }
        }
    }
    // 把原图叠回去
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            const Rgba p = pixels_[static_cast<size_t>(y) * width_ + x];
            if (alphaOf(p) == 0) continue;
            px[static_cast<size_t>(y + thickness) * w + (x + thickness)] = p;
        }
    }
    return fromRaw(w, h, std::move(px));
}

} // namespace ls
