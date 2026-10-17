// =============================================================================
//  ui/Text.h  —  文字绘制（底层走 EasyX / GDI，字形质量比自绘好）
// =============================================================================
#pragma once

#include <string>

#include "gfx/Canvas.h"
#include "gfx/Color.h"
#include "gfx/Sprite.h"

namespace ls {
namespace ui {

enum class Align
{
    Left,
    Center,
    Right
};

struct Font
{
    int size = 20;              // 像素高度
    int weight = 400;           // FW_NORMAL / FW_BOLD
    const wchar_t* face = L"Microsoft YaHei";
    bool italic = false;
};

// 常用字体组合
const Font& fontTitle();   // 标题
const Font& fontHeading(); // 小标题
const Font& fontBody();    // 正文（中文）
const Font& fontMono();    // 数字（等宽，HUD 用）
const Font& fontMonoBold();

int measure(const std::wstring& text, const Font& font);

// 以 anchor 为对齐基准绘制；shadow=true 时带一层深色投影，保证在噪点背景上可读
void draw(const std::wstring& text, Vec2 anchor, const Font& font, Rgba color, Align align = Align::Left,
          bool shadow = true);

// 带字距的绘制（标题霓虹效果）
void drawSpaced(const std::wstring& text, Vec2 anchor, const Font& font, Rgba color, float tracking,
                Align align = Align::Center, bool shadow = true);

// 小工具：UTF-8 -> wstring
std::wstring wide(const std::string& utf8);

// 把文字渲染成带抗锯齿 alpha 的白色蒙版（带缓存）。
// 这样就能用 Canvas 的混合器给文字加霓虹光晕、缩放和淡入淡出。
const Sprite& textMask(const std::wstring& text, const Font& font);

// 霓虹标题：柔和外发光 + 实心字身（用蒙版 sprite 绘制，不是 GDI 直出）
void drawNeon(Canvas& canvas, const std::wstring& text, Vec2 center, const Font& font, Rgba color,
              float glowStrength = 1.0f, float tracking = 0.0f);

// 带描边的正文（叠在最花的画面上也读得清）
void drawOutlined(const std::wstring& text, Vec2 anchor, const Font& font, Rgba color, Align align = Align::Center);

} // namespace ui
} // namespace ls
