// =============================================================================
//  core/Math.h  —  向量与常用数学工具
//  说明：整个项目统一使用 float 标量与 Vec2 向量，避免 int 截断带来的抖动。
// =============================================================================
#pragma once

#include <cmath>
#include <algorithm>

namespace ls {

// ------------------------------------------------------------------ 常量 ----
constexpr float kPi = 3.14159265358979f;
constexpr float kTau = 6.28318530717959f;
constexpr float kDeg2Rad = kPi / 180.0f;
constexpr float kRad2Deg = 180.0f / kPi;

// ------------------------------------------------------------------ 向量 ----
struct Vec2
{
    float x = 0.0f;
    float y = 0.0f;

    constexpr Vec2() = default;
    constexpr Vec2(float x_, float y_) : x(x_), y(y_) {}

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2 operator/(float s) const { return {x / s, y / s}; }
    Vec2 operator-() const { return {-x, -y}; }

    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }

    float lengthSq() const { return x * x + y * y; }
    float length() const { return std::sqrt(x * x + y * y); }

    // 归一化；零向量返回 (0,0) 而不是 NaN
    Vec2 normalized() const
    {
        const float len = length();
        return len > 1e-6f ? Vec2{x / len, y / len} : Vec2{0.0f, 0.0f};
    }

    // 旋转 90°（左手系，y 轴向下）
    Vec2 perpendicular() const { return {-y, x}; }
};

inline Vec2 operator*(float s, const Vec2& v) { return v * s; }
inline float dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }
inline float distance(const Vec2& a, const Vec2& b) { return (a - b).length(); }
inline float distanceSq(const Vec2& a, const Vec2& b) { return (a - b).lengthSq(); }

// 极坐标 -> 笛卡尔（y 轴向下，角度 0 指向 +x）
inline Vec2 fromAngle(float radians, float radius = 1.0f)
{
    return {std::cos(radians) * radius, std::sin(radians) * radius};
}

inline float angleOf(const Vec2& v) { return std::atan2(v.y, v.x); }

// ------------------------------------------------------------------ 标量 ----
inline float clampf(float v, float lo, float hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}
inline float saturate(float v) { return clampf(v, 0.0f, 1.0f); }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
inline int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float signf(float v) { return v < 0.0f ? -1.0f : 1.0f; }

// 帧率无关的指数趋近：每秒衰减 rate 比例
inline float approachExp(float current, float target, float rate, float dt)
{
    return target + (current - target) * std::exp(-rate * dt);
}

// 线性趋近：每帧最多移动 maxDelta
inline float approach(float current, float target, float maxDelta)
{
    const float d = target - current;
    if (std::fabs(d) <= maxDelta) return target;
    return current + signf(d) * maxDelta;
}

inline Vec2 approachVec(const Vec2& current, const Vec2& target, float maxDelta)
{
    const Vec2 d = target - current;
    const float len = d.length();
    if (len <= maxDelta || len < 1e-6f) return target;
    return current + d * (maxDelta / len);
}

inline float easeOutBack(float t)
{
    t = saturate(t);
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    const float inv = t - 1.0f;
    return 1.0f + c3 * inv * inv * inv + c1 * inv * inv;
}

// 角度差归一到 [-pi, pi]
inline float wrapAngle(float radians)
{
    while (radians > kPi) radians -= kTau;
    while (radians < -kPi) radians += kTau;
    return radians;
}

// ------------------------------------------------------------------ 形状 ----
struct Circle
{
    Vec2 center;
    float radius = 0.0f;
};

inline bool overlaps(const Circle& a, const Circle& b)
{
    const float r = a.radius + b.radius;
    return distanceSq(a.center, b.center) <= r * r;
}

struct Rect
{
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    float left() const { return x; }
    float top() const { return y; }
    float right() const { return x + w; }
    float bottom() const { return y + h; }
    Vec2 center() const { return {x + w * 0.5f, y + h * 0.5f}; }

    bool contains(const Vec2& p) const
    {
        return p.x >= x && p.x <= x + w && p.y >= y && p.y <= y + h;
    }
    Rect inset(float amount) const
    {
        return {x + amount, y + amount, w - amount * 2.0f, h - amount * 2.0f};
    }
};

inline int iround(float v) { return static_cast<int>(v + (v >= 0.0f ? 0.5f : -0.5f)); }

} // namespace ls
