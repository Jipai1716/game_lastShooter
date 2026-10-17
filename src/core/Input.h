// =============================================================================
//  core/Input.h  —  键鼠输入快照
//  说明：原项目用 peekmessage 处理鼠标，导致「鼠标不动就不开火」的 BUG。
//        这里改成每帧轮询 GetAsyncKeyState + 光标位置，按下即开火、按住即连发。
//        InputState 是纯数据结构，机器人（demo/selftest）可以直接伪造它。
// =============================================================================
#pragma once

#include <array>

#include "core/Math.h"

namespace ls {

enum class Key : int
{
    Up = 0,
    Down,
    Left,
    Right,
    W, A, S, D,
    Space,
    Shift,
    Reload,
    Pause,
    Restart,
    Confirm,
    Grenade,
    Debug,
    MouseLeft,
    MouseRight,
    Count
};

constexpr int kKeyCount = static_cast<int>(Key::Count);

struct InputState
{
    Vec2 mouse{400.0f, 400.0f};
    std::array<bool, kKeyCount> down{};
    std::array<bool, kKeyCount> pressed{};
    std::array<bool, kKeyCount> released{};
    bool focused = true;   // 窗口是否在前台（失焦时暂停射击）
    bool quit = false;     // 窗口被关闭

    bool isDown(Key k) const { return down[static_cast<int>(k)]; }
    bool isPressed(Key k) const { return pressed[static_cast<int>(k)]; }
    bool isReleased(Key k) const { return released[static_cast<int>(k)]; }

    // 供机器人使用：伪造一次「按下 + 松开」
    void press(Key k)
    {
        down[static_cast<int>(k)] = true;
        pressed[static_cast<int>(k)] = true;
    }
    void release(Key k) { down[static_cast<int>(k)] = false; }
    void clearEdges()
    {
        pressed.fill(false);
        released.fill(false);
    }
};

class Input
{
public:
    void poll(const Vec2& mouseReference);
    const InputState& state() const { return state_; }

private:
    InputState state_{};
    std::array<bool, kKeyCount> previous_{};
};

} // namespace ls
