// =============================================================================
//  core/Random.h  —  确定性随机数（xorshift32）
//  说明：原项目每次取随机数都 srand(clock())，导致同一帧内的随机值高度相关。
//        这里换成自带状态的 xorshift，可复现、无全局副作用。
// =============================================================================
#pragma once

#include <cstdint>
#include "core/Math.h"

namespace ls {

class Rng
{
public:
    explicit Rng(std::uint32_t seed = 0x9E3779B9u) { reseed(seed); }

    void reseed(std::uint32_t seed)
    {
        state_ = seed ? seed : 0x9E3779B9u;
        for (int i = 0; i < 8; ++i) nextU32(); // 预热，避免低比特质量差
    }

    std::uint32_t nextU32()
    {
        state_ ^= state_ << 13;
        state_ ^= state_ >> 17;
        state_ ^= state_ << 5;
        return state_;
    }

    // [0, 1)
    float nextFloat() { return static_cast<float>(nextU32() >> 8) * (1.0f / 16777216.0f); }

    // [lo, hi)
    float range(float lo, float hi) { return lo + (hi - lo) * nextFloat(); }

    // [lo, hi] 整数
    int rangeInt(int lo, int hi)
    {
        if (hi <= lo) return lo;
        return lo + static_cast<int>(nextU32() % static_cast<std::uint32_t>(hi - lo + 1));
    }

    bool chance(float probability) { return nextFloat() < probability; }

    // 单位向量
    Vec2 direction()
    {
        const float a = range(0.0f, kTau);
        return fromAngle(a);
    }

    // 圆内均匀分布
    Vec2 insideCircle(float radius)
    {
        const float a = range(0.0f, kTau);
        const float r = radius * std::sqrt(nextFloat());
        return fromAngle(a, r);
    }

    // 环带内分布（rMin..rMax）
    Vec2 insideRing(float rMin, float rMax)
    {
        const float a = range(0.0f, kTau);
        const float r = std::sqrt(lerpf(rMin * rMin, rMax * rMax, nextFloat()));
        return fromAngle(a, r);
    }

private:
    std::uint32_t state_ = 0x9E3779B9u;
};

// 全局随机源（游戏逻辑用；需要复现的测试可自己 new 一个）
Rng& rng();

} // namespace ls
