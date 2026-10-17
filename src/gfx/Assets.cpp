#include "gfx/Assets.h"

#include <windows.h>

#include <algorithm>
#include <cmath>

#include "core/Log.h"
#include "core/Math.h"

namespace ls {

namespace {

std::string joinPath(const std::string& a, const std::string& b)
{
    if (a.empty()) return b;
    if (b.empty()) return a;
    const char last = a.back();
    if (last == '\\' || last == '/') return a + b;
    return a + "\\" + b;
}

bool fileExists(const std::string& path)
{
    const DWORD attributes = GetFileAttributesA(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

std::string parentDirectory(const std::string& path)
{
    const size_t pos = path.find_last_of("\\/");
    if (pos == std::string::npos) return std::string();
    return path.substr(0, pos);
}

// 生成暗角：中心透明，边缘为 color
Sprite makeVignette(int width, int height, Rgba color, float innerAlpha, float power)
{
    std::vector<Rgba> px(static_cast<size_t>(width) * height, 0u);
    const float cx = width * 0.5f;
    const float cy = height * 0.5f;
    const float maxDist = std::sqrt(cx * cx + cy * cy);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const float dx = x + 0.5f - cx;
            const float dy = y + 0.5f - cy;
            const float d = std::sqrt(dx * dx + dy * dy) / maxDist;
            const float a = std::pow(saturate((d - 0.35f) / 0.65f), power) * innerAlpha;
            px[static_cast<size_t>(y) * width + x] = withAlpha(color, a);
        }
    }
    return Sprite::fromRaw(width, height, std::move(px));
}

// 背景烘焙：压暗 + 冷色调 + 烘焙暗角 + 轻微色阶
Sprite makeBackground(const Sprite& raw)
{
    if (raw.empty()) return raw;
    const int w = raw.width();
    const int h = raw.height();
    std::vector<Rgba> px(static_cast<size_t>(w) * h, 0u);
    const float cx = w * 0.5f;
    const float cy = h * 0.5f;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const Rgba p = raw.at(x, y);
            const float nx = (x + 0.5f - cx) / cx;
            const float ny = (y + 0.5f - cy) / cy;
            const float d2 = nx * nx + ny * ny;
            float v = 1.0f - 0.55f * d2;   // 暗角
            v = std::max(v, 0.22f);

            int r = static_cast<int>(redOf(p) * 0.44f * v) + 3;
            int g = static_cast<int>(greenOf(p) * 0.52f * v) + 6;
            int b = static_cast<int>(blueOf(p) * 0.80f * v) + 18;

            // 稍微拉高反差，让砖缝更清晰
            r = clampi(r, 0, 255);
            g = clampi(g, 0, 255);
            b = clampi(b, 0, 255);
            px[static_cast<size_t>(y) * w + x] = rgba(r, g, b);
        }
    }
    return Sprite::fromRaw(w, h, std::move(px));
}

Sprite loadRequired(const std::string& path, Rgba key = Sprite::kKeyDefault, bool useKey = true)
{
    Sprite s = useKey ? Sprite::load(path) : Sprite::fromFile(path, key, false);
    if (s.empty()) {
        LS_LOG("missing asset: %s", path.c_str());
    }
    return s;
}

} // namespace

std::string findAssetRoot(const std::string& startDirectory)
{
    std::string dir = startDirectory;
    for (int depth = 0; depth < 5 && !dir.empty(); ++depth) {
        const std::string candidate = joinPath(dir, "assets");
        if (fileExists(joinPath(joinPath(candidate, "image"), "hero.png"))) {
            return candidate;
        }
        dir = parentDirectory(dir);
    }
    // 兜底：当前工作目录
    if (fileExists("assets\\image\\hero.png")) return "assets";
    return std::string();
}

bool Assets::load(const std::string& executableDirectory)
{
    root = findAssetRoot(executableDirectory);
    if (root.empty()) {
        LS_LOG("asset root not found (assets/image/hero.png)");
        return false;
    }
    const std::string imageDir = joinPath(root, "image");
    auto img = [&imageDir](const char* name, Rgba key = Sprite::kKeyDefault, bool useKey = true) {
        return loadRequired(joinPath(imageDir, name), key, useKey);
    };

    // ------------------------------------------------------------ 场景 ----
    background = makeBackground(img("background.png", 0u, false));
    vignette = makeVignette(background.width() > 0 ? background.width() : 800,
                            background.height() > 0 ? background.height() : 800, rgba(0, 0, 0), 0.95f, 1.6f);

    // ------------------------------------------------------------ 玩家 ----
    // 统一加一圈暗描边：16bit 像素画在噪点砖墙上不加描边会糊成一片
    const Rgba kOutline = rgba(6, 10, 18, 230);
    hero = img("hero.png").scaledNearest(2).outline(kOutline, 1);
    heroLeft = hero.mirroredX();

    // ------------------------------------------------------------ 敌人 ----
    zombie = img("enemy.png").scaledNearest(2).outline(kOutline, 1);
    runner = zombie.tinted(rgba(150, 225, 255));
    slime = img("slime.png").scaledNearest(2).outline(kOutline, 1);
    slimeMini = img("slime.png").outline(kOutline, 1).tinted(rgba(190, 255, 160));
    brute = img("enemy.png").scaledNearest(3).outline(kOutline, 2).tinted(rgba(255, 190, 170));
    spitter = img("reimu.png").scaledNearest(2).outline(kOutline, 1).tinted(rgba(235, 205, 255));
    boss = img("enemy.png").scaledNearest(4).outline(kOutline, 2).tinted(rgba(255, 205, 225));

    // ------------------------------------------------------------ 道具 ----
    itemSpeed = img("item_speedUp.png").scaledNearest(2);
    itemAttack = img("item_ATKUp.png").scaledNearest(2);
    itemRate = img("item_ASPup.png").scaledNearest(2);
    itemHeart = img("heart.png").scaledNearest(2);
    itemGrenade = img("boomer.png").scaledNearest(2);

    // ------------------------------------------------------------ 特效 ----
    crosshair = img("crossstar.png").scaledNearest(3);
    explosion = img("boom.png").scaledNearest(3);
    heartIcon = img("heart.png").scaledNearest(1);
    grenadeIcon = img("boomer.png").scaledNearest(1);

    glowSoft = Sprite::glow(64.0f, rgba(255, 255, 255, 255), 2.2f);
    glowTight = Sprite::glow(24.0f, rgba(255, 255, 255, 255), 3.0f);

    loaded_ = !hero.empty() && !background.empty();
    LS_LOG("assets loaded from %s (hero %dx%d, bg %dx%d)", root.c_str(), hero.width(), hero.height(),
           background.width(), background.height());
    return loaded_;
}

Assets& assets()
{
    static Assets instance;
    return instance;
}

} // namespace ls
