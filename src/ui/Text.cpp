#include "ui/Text.h"

#include <easyx.h>
#include <windows.h>

#include <algorithm>
#include <map>
#include <vector>

namespace ls {
namespace ui {

namespace {

Font g_title{64, 700, L"Microsoft YaHei"};
Font g_heading{28, 700, L"Microsoft YaHei"};
Font g_body{20, 400, L"Microsoft YaHei"};
Font g_mono{22, 400, L"Consolas"};
Font g_monoBold{26, 700, L"Consolas"};

// 只有当样式变化时才重新创建 GDI 字体，避免每帧反复 settextstyle
struct ActiveStyle
{
    int size = -1;
    int weight = -1;
    std::wstring face;
    bool italic = false;
    bool valid = false;
};
ActiveStyle g_active;

void applyFont(const Font& font)
{
    if (g_active.valid && g_active.size == font.size && g_active.weight == font.weight &&
        g_active.italic == font.italic && g_active.face == font.face) {
        return;
    }
    settextstyle(font.size, 0, font.face, 0, 0, font.weight, font.italic, false, false, DEFAULT_CHARSET,
                 OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE);
    g_active.size = font.size;
    g_active.weight = font.weight;
    g_active.face = font.face;
    g_active.italic = font.italic;
    g_active.valid = true;
}

COLORREF toColorRef(Rgba color)
{
    return RGB(redOf(color), greenOf(color), blueOf(color));
}

void drawRun(const std::wstring& text, int x, int y, Rgba color)
{
    settextcolor(toColorRef(color));
    outtextxy(x, y, text.c_str());
}

} // namespace

const Font& fontTitle() { return g_title; }
const Font& fontHeading() { return g_heading; }
const Font& fontBody() { return g_body; }
const Font& fontMono() { return g_mono; }
const Font& fontMonoBold() { return g_monoBold; }

int measure(const std::wstring& text, const Font& font)
{
    applyFont(font);
    return textwidth(text.c_str());
}

void draw(const std::wstring& text, Vec2 anchor, const Font& font, Rgba color, Align align, bool shadow)
{
    if (text.empty()) return;
    applyFont(font);
    const int width = textwidth(text.c_str());
    int x = iround(anchor.x);
    if (align == Align::Center) x -= width / 2;
    else if (align == Align::Right) x -= width;
    const int y = iround(anchor.y);
    if (shadow) {
        drawRun(text, x + 1, y + 2, rgba(0, 0, 0, 150));
    }
    drawRun(text, x, y, color);
}

void drawSpaced(const std::wstring& text, Vec2 anchor, const Font& font, Rgba color, float tracking, Align align,
                bool shadow)
{
    if (text.empty()) return;
    applyFont(font);

    int total = 0;
    for (const wchar_t ch : text) {
        total += textwidth(ch);
    }
    total += static_cast<int>(tracking) * static_cast<int>(text.size() > 0 ? text.size() - 1 : 0);

    int x = iround(anchor.x);
    if (align == Align::Center) x -= total / 2;
    else if (align == Align::Right) x -= total;
    const int y = iround(anchor.y);

    for (const wchar_t ch : text) {
        if (shadow) drawRun(std::wstring(1, ch), x + 1, y + 2, rgba(0, 0, 0, 160));
        drawRun(std::wstring(1, ch), x, y, color);
        x += textwidth(ch) + static_cast<int>(tracking);
    }
}

std::wstring wide(const std::string& utf8)
{
    if (utf8.empty()) return std::wstring();
    const int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring result(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), &result[0], size);
    return result;
}

// ---------------------------------------------------------------- 蒙版 ----
namespace {

std::string maskKey(const std::wstring& text, const Font& font)
{
    std::string key = std::to_string(font.size) + "|" + std::to_string(font.weight) + "|" +
                      std::to_string(font.italic ? 1 : 0) + "|";
    for (const wchar_t ch : text) key.push_back(static_cast<char>(ch & 0xFF));
    return key;
}

} // namespace

const Sprite& textMask(const std::wstring& text, const Font& font)
{
    static std::map<std::string, Sprite> cache;
    static const Sprite kEmpty;

    const std::string key = maskKey(text, font);
    const auto it = cache.find(key);
    if (it != cache.end()) return it->second;

    // 1) 用一个离屏 IMAGE 画白字（黑底），再把亮度当覆盖率抠成 alpha
    const int padding = 6;
    const int width = std::max(1, measure(text, font) + padding * 2);
    const int height = font.size + padding * 2 + 8;

    IMAGE canvasImage(width, height);
    SetWorkingImage(&canvasImage);
    // 注意：切到离屏 IMAGE 后 EasyX 会套用该图像的默认样式，
    // 所以字体/颜色/背景模式必须在这里重新设置，不能依赖缓存。
    setbkcolor(RGB(0, 0, 0));
    setbkmode(OPAQUE);
    cleardevice();
    settextcolor(RGB(255, 255, 255));
    settextstyle(font.size, 0, font.face, 0, 0, font.weight, font.italic, false, false, DEFAULT_CHARSET,
                 OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE);
    outtextxy(padding, padding + 1, text.c_str());
    SetWorkingImage(nullptr);
    // 回到屏幕设备后样式已经变了，让缓存失效以便下次重设
    g_active.valid = false;

    const DWORD* buffer = GetImageBuffer(&canvasImage);
    std::vector<Rgba> pixels(static_cast<size_t>(width) * height, 0u);
    bool any = false;
    for (size_t i = 0; i < pixels.size(); ++i) {
        const Rgba p = buffer[i];
        const int luminance = (redOf(p) + greenOf(p) + blueOf(p)) / 3;
        if (luminance > 4) {
            any = true;
            pixels[i] = rgba(255, 255, 255, luminance);
        }
    }
    if (!any) {
        cache.emplace(key, kEmpty);
        return cache[key];
    }
    const auto inserted = cache.emplace(key, Sprite::fromRaw(width, height, std::move(pixels)));
    return inserted.first->second;
}

void drawNeon(Canvas& canvas, const std::wstring& text, Vec2 center, const Font& font, Rgba color,
              float glowStrength, float tracking)
{
    if (text.empty()) return;

    if (tracking <= 0.0f) {
        const Sprite& mask = textMask(text, font);
        if (mask.empty()) return;
        // 外发光：两张放大 + 加法混合
        if (glowStrength > 0.0f) {
            canvas.drawSprite(mask, center, 1.10f, color, 0.32f * glowStrength, true);
            canvas.drawSprite(mask, center, 1.24f, color, 0.18f * glowStrength, true);
        }
        canvas.drawSprite(mask, center, 1.0f, color, 1.0f, false);
        return;
    }

    // 带字距：逐字绘制
    std::vector<const Sprite*> masks;
    std::vector<int> advances;
    masks.reserve(text.size());
    advances.reserve(text.size());
    int totalWidth = 0;
    for (const wchar_t ch : text) {
        // 空格等字符没有可视像素，靠 GDI 的宽度推进来占位
        const int advance = measure(std::wstring(1, ch), font);
        masks.push_back(&textMask(std::wstring(1, ch), font));
        advances.push_back(advance);
        totalWidth += advance;
    }
    totalWidth += static_cast<int>(tracking) * static_cast<int>(text.size() - 1);

    float x = center.x - totalWidth * 0.5f;
    for (size_t i = 0; i < masks.size(); ++i) {
        const int advance = advances[i];
        const Sprite* mask = masks[i];
        if (mask != nullptr && !mask->empty()) {
            const float glyphCenter = x + (static_cast<float>(mask->width()) - 12.0f) * 0.5f;
            const Vec2 position{glyphCenter, center.y};
            if (glowStrength > 0.0f) {
                canvas.drawSprite(*mask, position, 1.10f, color, 0.30f * glowStrength, true);
                canvas.drawSprite(*mask, position, 1.26f, color, 0.16f * glowStrength, true);
            }
            canvas.drawSprite(*mask, position, 1.0f, color, 1.0f, false);
        }
        x += static_cast<float>(advance) + tracking;
    }
}

void drawOutlined(const std::wstring& text, Vec2 anchor, const Font& font, Rgba color, Align align)
{
    static const Rgba kOutline = rgba(6, 10, 16, 235);
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            draw(text, {anchor.x + static_cast<float>(dx), anchor.y + static_cast<float>(dy)}, font, kOutline, align,
                 false);
        }
    }
    draw(text, anchor, font, color, align, false);
}

} // namespace ui
} // namespace ls
