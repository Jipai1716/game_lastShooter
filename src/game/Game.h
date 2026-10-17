// =============================================================================
//  game/Game.h  —  应用层：状态机 + 固定步长主循环 + 各界面
// =============================================================================
#pragma once

#include <string>

#include "core/Input.h"
#include "gfx/Canvas.h"
#include "gfx/Particles.h"
#include "game/Hud.h"
#include "game/Save.h"
#include "game/World.h"

namespace ls {

struct AppOptions
{
    bool autoplay = false;        // 机器人自动演示
    bool mute = false;
    int selftestFrames = 0;       // >0 时只跑无头模拟
    std::string captureDir;       // 非空时按脚本截图后退出（开发自测用）
    bool dumpInput = false;       // 把输入状态写到 inputdump.txt（调试用）
};

class Game
{
public:
    int run(const AppOptions& options);

private:
    enum class State
    {
        Title,
        Playing,
        Paused,
        GameOver
    };

    bool initialize();
    void shutdown();

    void updateFrame(float dt, InputState input);
    void renderFrame();

    void startRun();
    void endRun();

    void updateTitle(float dt, const InputState& input);
    void updatePlaying(float dt, const InputState& input);
    void updatePaused(float dt, const InputState& input);
    void updateGameOver(float dt, const InputState& input);

    void renderTitle();
    void renderPlaying();
    void renderPaused();
    void renderGameOver();
    void renderControlsPanel(float y, float alpha);

    InputState makeBotInput(float dt) const;
    InputState neutralInput() const;

    int runHeadless(int frames);
    int runCapture(const std::string& directory);

    State state_ = State::Title;
    Canvas canvas_;
    Input input_;
    World world_;
    Hud hud_;
    ParticleSystem ambience_;
    Record record_;
    AppOptions options_;

    std::string exeDirectory_;
    float realTime_ = 0.0f;
    float stateTime_ = 0.0f;
    float deathTimer_ = 0.0f;
    float unfocusedTimer_ = 0.0f;
    float accumulator_ = 0.0f;
    bool quit_ = false;
    bool newRecord_ = false;
    int bestComboTier_ = 1;

    // 机器人计时器（mutable 因为 makeBotInput 是 const 且要被多个更新调用）
    mutable float botGrenadeTimer_ = 3.0f;
    mutable float botDashTimer_ = 1.0f;
};

// 可执行文件所在目录（带结尾反斜杠）
std::string executableDirectory();

} // namespace ls
