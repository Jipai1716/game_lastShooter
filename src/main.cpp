// =============================================================================
//  main.cpp  —  程序入口
//  说明：同时提供 main / WinMain，因此既能用控制台子系统（调试看日志），
//        也能用 -mwindows 编成纯窗口程序（发布版）。
//
//  命令行参数：
//    --autoplay            机器人自动演示
//    --mute                静音
//    --selftest=<帧数>      无头跑模拟，打印统计（CI/回归用）
//    --capture=<目录>       按脚本截图到指定目录后退出（开发自测）
// =============================================================================
#include <windows.h>
#include <shellapi.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "core/Log.h"
#include "game/Game.h"

namespace {

// -mwindows 编出来的程序没有控制台；自测/帮助需要输出时挂到父进程控制台上。
// 注意：如果 stdout 已经被父进程重定向到文件/管道，就不要再抢过来。
void ensureConsole()
{
    static bool attempted = false;
    if (attempted) return;
    attempted = true;

    const HANDLE stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (stdoutHandle != nullptr && stdoutHandle != INVALID_HANDLE_VALUE) return;

    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE* stream = nullptr;
        freopen_s(&stream, "CONOUT$", "w", stdout);
        freopen_s(&stream, "CONOUT$", "w", stderr);
    }
}

std::vector<std::string> parseCommandLine()
{
    std::vector<std::string> arguments;
    int count = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &count);
    if (argv == nullptr) return arguments;

    for (int i = 1; i < count; ++i) {
        const std::wstring wide(argv[i]);
        const int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), nullptr, 0,
                                             nullptr, nullptr);
        std::string utf8(static_cast<size_t>(size), '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), &utf8[0], size, nullptr, nullptr);
        arguments.push_back(utf8);
    }
    LocalFree(argv);
    return arguments;
}

int runGame(const std::vector<std::string>& arguments)
{
    ls::AppOptions options;

    for (const std::string& argument : arguments) {
        if (argument == "--autoplay" || argument == "-a") {
            options.autoplay = true;
        } else if (argument == "--mute") {
            options.mute = true;
        } else if (argument.rfind("--selftest", 0) == 0) {
            ensureConsole();
            const size_t pos = argument.find('=');
            options.selftestFrames = pos == std::string::npos ? 3000 : std::atoi(argument.c_str() + pos + 1);
            if (options.selftestFrames <= 0) options.selftestFrames = 3000;
        } else if (argument.rfind("--capture=", 0) == 0) {
            ensureConsole();
            options.captureDir = argument.substr(10);
        } else if (argument == "--verbose") {
            ensureConsole();
            ls::setLogEnabled(true);
        } else if (argument == "--help" || argument == "-h") {
            ensureConsole();
            std::printf(
                "LAST SHOOTER\n"
                "  --autoplay        机器人自动演示\n"
                "  --mute            静音\n"
                "  --selftest[=N]    无头自测 N 帧（默认 3000）\n"
                "  --capture=DIR     按脚本截图后退出\n");
            return 0;
        }
    }

    ls::Game game;
    return game.run(options);
}

} // namespace

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    return runGame(parseCommandLine());
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR commandLine, int show)
{
    (void)instance;
    (void)previous;
    (void)commandLine;
    (void)show;
    return runGame(parseCommandLine());
}
