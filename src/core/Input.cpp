#include "core/Input.h"

#include <easyx.h>
#include <windows.h>

namespace ls {

namespace {

int virtualKeyOf(Key key)
{
    switch (key) {
    case Key::Up: return VK_UP;
    case Key::Down: return VK_DOWN;
    case Key::Left: return VK_LEFT;
    case Key::Right: return VK_RIGHT;
    case Key::W: return 'W';
    case Key::A: return 'A';
    case Key::S: return 'S';
    case Key::D: return 'D';
    case Key::Space: return VK_SPACE;
    case Key::Shift: return VK_SHIFT;
    case Key::Reload: return 'R';
    case Key::Pause: return VK_ESCAPE;
    case Key::Restart: return VK_F5;
    case Key::Confirm: return VK_RETURN;
    case Key::Grenade: return 'Q';
    case Key::Debug: return VK_F1;
    case Key::MouseLeft: return VK_LBUTTON;
    case Key::MouseRight: return VK_RBUTTON;
    default: return 0;
    }
}

} // namespace

void Input::poll(const Vec2& mouseReference)
{
    HWND hwnd = GetHWnd();

    // 窗口是否还活着（用户点了关闭按钮）
    state_.quit = (hwnd == nullptr) || !IsWindow(hwnd);
    state_.focused = state_.quit ? false : (GetForegroundWindow() == hwnd);

    // ---- 鼠标位置：屏幕坐标 -> 客户区坐标 ----
    POINT cursor;
    if (GetCursorPos(&cursor) && hwnd != nullptr) {
        ScreenToClient(hwnd, &cursor);
        // EasyX 会把窗口客户区放大到逻辑尺寸，这里做等比换算
        RECT client{};
        GetClientRect(hwnd, &client);
        const float clientW = static_cast<float>(client.right - client.left);
        const float clientH = static_cast<float>(client.bottom - client.top);
        float logicalW = static_cast<float>(getwidth());
        float logicalH = static_cast<float>(getheight());
        if (logicalW <= 0.0f) logicalW = mouseReference.x * 2.0f;
        if (logicalH <= 0.0f) logicalH = mouseReference.y * 2.0f;
        const float sx = clientW > 0.0f ? logicalW / clientW : 1.0f;
        const float sy = clientH > 0.0f ? logicalH / clientH : 1.0f;
        state_.mouse.x = cursor.x * sx;
        state_.mouse.y = cursor.y * sy;
    } else {
        state_.mouse = mouseReference;
    }

    // ---- 按键 ----
    for (int i = 0; i < kKeyCount; ++i) {
        const int vk = virtualKeyOf(static_cast<Key>(i));
        bool now = false;
        if (vk != 0) {
            now = (GetAsyncKeyState(vk) & 0x8000) != 0;
        }
        if (!state_.focused && vk != VK_ESCAPE) now = false; // 失焦不吃输入
        const bool was = previous_[i];
        state_.down[i] = now;
        state_.pressed[i] = now && !was;
        state_.released[i] = !now && was;
        previous_[i] = now;
    }
}

} // namespace ls
