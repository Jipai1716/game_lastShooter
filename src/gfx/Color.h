// =============================================================================
//  gfx/Color.h  —  颜色表示（0xAARRGGBB）与调色板
//  说明：像素格式与 EasyX 的 32 位缓冲区一致：高字节 alpha，低字节蓝。
// =============================================================================
#pragma once

#include <cstdint>

#include "core/Math.h"

namespace ls {

using Rgba = std::uint32_t;

constexpr Rgba rgba(int r, int g, int b, int a = 255)
{
    return (static_cast<Rgba>(a & 0xFF) << 24) | (static_cast<Rgba>(r & 0xFF) << 16) |
           (static_cast<Rgba>(g & 0xFF) << 8) | static_cast<Rgba>(b & 0xFF);
}

constexpr int alphaOf(Rgba c) { return static_cast<int>((c >> 24) & 0xFF); }
constexpr int redOf(Rgba c) { return static_cast<int>((c >> 16) & 0xFF); }
constexpr int greenOf(Rgba c) { return static_cast<int>((c >> 8) & 0xFF); }
constexpr int blueOf(Rgba c) { return static_cast<int>(c & 0xFF); }

// 替换/缩放 alpha
inline Rgba withAlpha(Rgba c, float a)
{
    return (c & 0x00FFFFFFu) | (static_cast<Rgba>(saturate(a) * 255.0f + 0.5f) << 24);
}

// 整体亮度缩放（保留 alpha）
inline Rgba scaleColor(Rgba c, float factor)
{
    const int a = alphaOf(c);
    const int r = static_cast<int>(redOf(c) * factor);
    const int g = static_cast<int>(greenOf(c) * factor);
    const int b = static_cast<int>(blueOf(c) * factor);
    return rgba(std::min(r, 255), std::min(g, 255), std::min(b, 255), a);
}

inline Rgba lerpColor(Rgba a, Rgba b, float t)
{
    t = saturate(t);
    const int r = static_cast<int>(lerpf(static_cast<float>(redOf(a)), static_cast<float>(redOf(b)), t));
    const int g = static_cast<int>(lerpf(static_cast<float>(greenOf(a)), static_cast<float>(greenOf(b)), t));
    const int bl = static_cast<int>(lerpf(static_cast<float>(blueOf(a)), static_cast<float>(blueOf(b)), t));
    const int al = static_cast<int>(lerpf(static_cast<float>(alphaOf(a)), static_cast<float>(alphaOf(b)), t));
    return rgba(r, g, bl, al);
}

// 逐分量相乘（用于染色）
inline Rgba mulColor(Rgba c, Rgba tint)
{
    const int r = redOf(c) * redOf(tint) / 255;
    const int g = greenOf(c) * greenOf(tint) / 255;
    const int b = blueOf(c) * blueOf(tint) / 255;
    const int a = alphaOf(c) * alphaOf(tint) / 255;
    return rgba(r, g, b, a);
}

// --------------------------------------------------------------- 调色板 ----
namespace palette {

// 主色调：暗夜都市 + 霓虹青绿
constexpr Rgba kInk = rgba(10, 14, 22);          // 最深背景
constexpr Rgba kPanel = rgba(18, 24, 36);        // 面板
constexpr Rgba kPanelEdge = rgba(46, 66, 92);    // 面板描边
constexpr Rgba kText = rgba(226, 238, 248);      // 主文字
constexpr Rgba kTextDim = rgba(132, 154, 178);   // 次要文字
constexpr Rgba kAccent = rgba(96, 240, 214);     // 主强调（青绿）
constexpr Rgba kAccentDim = rgba(52, 150, 138);
constexpr Rgba kGold = rgba(255, 208, 96);       // 分数/奖励
constexpr Rgba kDanger = rgba(255, 84, 96);      // 危险/伤害
constexpr Rgba kDangerDeep = rgba(180, 32, 52);
constexpr Rgba kHeal = rgba(255, 116, 150);      // 生命
constexpr Rgba kWhite = rgba(255, 255, 255);
constexpr Rgba kBlack = rgba(0, 0, 0);

// 敌人配色
constexpr Rgba kZombie = rgba(150, 255, 120);
constexpr Rgba kRunner = rgba(120, 220, 255);
constexpr Rgba kSlime = rgba(120, 255, 170);
constexpr Rgba kBrute = rgba(255, 132, 108);
constexpr Rgba kSpitter = rgba(214, 150, 255);
constexpr Rgba kBoss = rgba(255, 86, 120);

} // namespace palette

} // namespace ls
