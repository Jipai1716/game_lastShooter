// =============================================================================
//  audio/Audio.h  —  音效（MCI 多通道 + 程序化合成 WAV）
//  说明：原项目只有一个 mciSendString("play ... shoot.mp3")，连点两下就断音。
//        这里启动时用合成器现场生成全部音效（不需要额外素材），
//        再用 MCI 开 8 条通道轮转播放，实现简单混音。
// =============================================================================
#pragma once

#include <string>

namespace ls {

enum class Sfx : int
{
    Shoot = 0,
    Hit,
    Kill,
    Explosion,
    Pickup,
    Heal,
    Hurt,
    Reload,
    Empty,
    WaveStart,
    Dash,
    BossRoar,
    GameOver,
    Count
};

class Audio
{
public:
    bool init();
    void shutdown();

    void play(Sfx sfx, float volume = 1.0f);
    void setMasterVolume(float volume) { masterVolume_ = volume; }
    float masterVolume() const { return masterVolume_; }
    bool ready() const { return ready_; }

private:
    static constexpr int kVoicesPerSfx = 2;
    static constexpr unsigned long long kMinIntervalMs = 28;

    static std::wstring aliasFor(int sfx, int slot);

    bool ready_ = false;
    float masterVolume_ = 0.8f;
    std::wstring files_[static_cast<int>(Sfx::Count)];
    bool aliasOpen_[static_cast<int>(Sfx::Count)][kVoicesPerSfx]{};
    int nextSlot_[static_cast<int>(Sfx::Count)]{};
    unsigned long long lastPlay_[static_cast<int>(Sfx::Count)]{};
};

Audio& audio();

} // namespace ls
