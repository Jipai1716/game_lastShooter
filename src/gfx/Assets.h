// =============================================================================
//  gfx/Assets.h  —  资源仓库
//  说明：所有图片在启动时一次性读入、抠图、放大、预旋转、调色，运行时零 IO。
//        美术素材是 16bit 风格的像素画，统一 2 倍最近邻放大，保持锐利。
//        全局单例是刻意为之：资源是只读的进程级缓存，不值得层层传递。
// =============================================================================
#pragma once

#include <string>
#include <vector>

#include "gfx/Color.h"
#include "gfx/Sprite.h"

namespace ls {

struct Assets
{
    std::string root;     // assets 目录（绝对路径）

    // ---------------------------------------------------------- 场景 ----
    Sprite background;    // 已压暗 + 冷色调 + 暗角烘焙
    Sprite vignette;      // 独立暗角（受击/低血量时叠）

    // ---------------------------------------------------------- 玩家 ----
    Sprite hero;                                    // 基础朝向（朝上）
    Sprite heroLeft;                                // 水平镜像（朝左时用）

    // ---------------------------------------------------------- 敌人 ----
    Sprite zombie;
    Sprite runner;
    Sprite slime;
    Sprite slimeMini;
    Sprite brute;
    Sprite spitter;
    Sprite boss;

    // ---------------------------------------------------------- 道具 ----
    Sprite itemSpeed;
    Sprite itemAttack;
    Sprite itemRate;
    Sprite itemHeart;
    Sprite itemGrenade;

    // ---------------------------------------------------------- 特效 ----
    Sprite crosshair;
    Sprite explosion;
    Sprite heartIcon;
    Sprite grenadeIcon;

    Sprite glowSoft;   // 通用柔光
    Sprite glowTight;  // 紧凑光斑

    // 加载全部资源；rootPath 为仓库根目录（会自动寻找 assets/）
    bool load(const std::string& executableDirectory);
    bool loaded() const { return loaded_; }

private:
    bool loaded_ = false;
};

Assets& assets();

// 在若干层父目录中定位 assets 目录；失败返回空串
std::string findAssetRoot(const std::string& startDirectory);

} // namespace ls
