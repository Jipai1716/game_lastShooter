#include "audio/Audio.h"

#include <windows.h>
#include <mmsystem.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "core/Log.h"
#include "core/Math.h"
#include "core/Random.h"

#pragma comment(lib, "winmm.lib")

namespace ls {

namespace {

constexpr int kSampleRate = 44100;
constexpr float kSampleRateF = static_cast<float>(kSampleRate);

// -------------------------------------------------------------- 合成器 ----
class Synth
{
public:
    explicit Synth(float duration) : samples_(static_cast<size_t>(duration * kSampleRateF), 0.0f) {}

    float* data() { return samples_.data(); }
    const float* data() const { return samples_.data(); }
    size_t size() const { return samples_.size(); }
    float duration() const { return static_cast<float>(samples_.size()) / kSampleRateF; }

    // 叠加一个带包络的音（phase 以 0..1 归一）
    void addTone(float startTime, float duration, float frequencyStart, float frequencyEnd, float amplitude,
                 int waveform, float attack = 0.005f, float decayPower = 2.0f)
    {
        const size_t start = static_cast<size_t>(startTime * kSampleRateF);
        const size_t count = static_cast<size_t>(duration * kSampleRateF);
        float phase = 0.0f;
        for (size_t i = 0; i < count; ++i) {
            const size_t index = start + i;
            if (index >= samples_.size()) break;
            const float t = static_cast<float>(i) / static_cast<float>(count);
            const float freq = lerpf(frequencyStart, frequencyEnd, t);
            phase += freq / kSampleRateF;
            phase -= std::floor(phase);

            float value = 0.0f;
            switch (waveform) {
            case 0: value = std::sin(phase * kTau); break;                       // sine
            case 1: value = phase < 0.5f ? 1.0f : -1.0f; break;                  // square
            case 2: value = phase * 2.0f - 1.0f; break;                          // saw
            case 3: value = 4.0f * std::fabs(phase - 0.5f) - 1.0f; break;        // triangle
            case 4: value = (phase < 0.25f || (phase > 0.5f && phase < 0.75f)) ? 1.0f : -1.0f; break;
            default: value = std::sin(phase * kTau); break;
            }
            const float env = envelope(t, attack, decayPower);
            samples_[index] += value * amplitude * env;
        }
    }

    // 噪声（可做带通/低通的粗糙版本）
    void addNoise(float startTime, float duration, float amplitude, float lowPassStart, float lowPassEnd,
                  float attack = 0.002f, float decayPower = 3.0f)
    {
        const size_t start = static_cast<size_t>(startTime * kSampleRateF);
        const size_t count = static_cast<size_t>(duration * kSampleRateF);
        float state = 0.0f;
        for (size_t i = 0; i < count; ++i) {
            const size_t index = start + i;
            if (index >= samples_.size()) break;
            const float t = static_cast<float>(i) / static_cast<float>(count);
            const float cutoff = lerpf(lowPassStart, lowPassEnd, t);
            const float alpha = saturate(cutoff / kSampleRateF * 6.2831853f);
            const float noise = rng().range(-1.0f, 1.0f);
            state += alpha * (noise - state);
            const float env = envelope(t, attack, decayPower);
            samples_[index] += state * amplitude * env;
        }
    }

    // 简单的软削波，避免爆音
    void finalize(float gain)
    {
        for (float& sample : samples_) {
            float v = sample * gain;
            v = v / (1.0f + std::fabs(v) * 0.6f);
            sample = clampf(v, -1.0f, 1.0f);
        }
    }

private:
    static float envelope(float t, float attack, float decayPower)
    {
        float env = 1.0f;
        if (attack > 0.0f && t < attack) env = t / attack;
        const float remain = saturate((1.0f - t) / std::max(0.001f, 1.0f - attack));
        env *= std::pow(remain, decayPower);
        return env;
    }

    std::vector<float> samples_;
};

// ------------------------------------------------------------ WAV 写出 ----
bool writeWav(const std::wstring& path, const Synth& synth)
{
    FILE* file = _wfopen(path.c_str(), L"wb");
    if (file == nullptr) return false;

    const std::uint32_t dataBytes = static_cast<std::uint32_t>(synth.size() * sizeof(std::int16_t));
    const std::uint32_t riffSize = 36 + dataBytes;
    const std::uint16_t channels = 1;
    const std::uint16_t bitsPerSample = 16;
    const std::uint32_t byteRate = kSampleRate * channels * bitsPerSample / 8;
    const std::uint16_t blockAlign = static_cast<std::uint16_t>(channels * bitsPerSample / 8);
    const std::uint16_t audioFormat = 1; // PCM

    std::fwrite("RIFF", 1, 4, file);
    std::fwrite(&riffSize, 4, 1, file);
    std::fwrite("WAVE", 1, 4, file);
    std::fwrite("fmt ", 1, 4, file);
    const std::uint32_t fmtSize = 16;
    std::fwrite(&fmtSize, 4, 1, file);
    std::fwrite(&audioFormat, 2, 1, file);
    std::fwrite(&channels, 2, 1, file);
    const std::uint32_t sampleRate = kSampleRate;
    std::fwrite(&sampleRate, 4, 1, file);
    std::fwrite(&byteRate, 4, 1, file);
    std::fwrite(&blockAlign, 2, 1, file);
    std::fwrite(&bitsPerSample, 2, 1, file);
    std::fwrite("data", 1, 4, file);
    std::fwrite(&dataBytes, 4, 1, file);

    for (size_t i = 0; i < synth.size(); ++i) {
        const float clamped = clampf(synth.data()[i], -1.0f, 1.0f);
        const std::int16_t sample = static_cast<std::int16_t>(clamped * 32000.0f);
        std::fwrite(&sample, sizeof(sample), 1, file);
    }
    std::fclose(file);
    return true;
}

// ---------------------------------------------------------------- 配方 ----
void buildShoot(Synth& s)
{
    s.addTone(0.0f, 0.09f, 900.0f, 220.0f, 0.55f, 2, 0.002f, 2.6f);
    s.addTone(0.0f, 0.06f, 1500.0f, 700.0f, 0.22f, 3, 0.001f, 3.5f);
    s.addNoise(0.0f, 0.05f, 0.18f, 6000.0f, 1200.0f, 0.001f, 3.0f);
    s.finalize(1.0f);
}

void buildHit(Synth& s)
{
    s.addNoise(0.0f, 0.07f, 0.55f, 8000.0f, 1500.0f, 0.001f, 2.4f);
    s.addTone(0.0f, 0.05f, 420.0f, 160.0f, 0.20f, 2, 0.001f, 2.5f);
    s.finalize(1.0f);
}

void buildKill(Synth& s)
{
    s.addNoise(0.0f, 0.22f, 0.6f, 5000.0f, 400.0f, 0.002f, 2.0f);
    s.addTone(0.0f, 0.18f, 180.0f, 60.0f, 0.5f, 0, 0.002f, 2.2f);
    s.finalize(1.1f);
}

void buildExplosion(Synth& s)
{
    s.addNoise(0.0f, 0.55f, 0.8f, 2200.0f, 90.0f, 0.004f, 1.8f);
    s.addTone(0.0f, 0.35f, 120.0f, 40.0f, 0.75f, 0, 0.003f, 1.6f);
    s.addTone(0.0f, 0.12f, 700.0f, 120.0f, 0.25f, 2, 0.002f, 2.5f);
    s.finalize(1.15f);
}

void buildPickup(Synth& s)
{
    s.addTone(0.0f, 0.06f, 660.0f, 660.0f, 0.35f, 3, 0.002f, 1.2f);
    s.addTone(0.06f, 0.10f, 990.0f, 1320.0f, 0.35f, 3, 0.002f, 1.6f);
    s.finalize(1.0f);
}

void buildHeal(Synth& s)
{
    s.addTone(0.0f, 0.30f, 523.0f, 523.0f, 0.28f, 0, 0.01f, 1.5f);
    s.addTone(0.0f, 0.30f, 659.0f, 659.0f, 0.22f, 0, 0.02f, 1.5f);
    s.addTone(0.0f, 0.34f, 784.0f, 784.0f, 0.22f, 0, 0.03f, 1.4f);
    s.finalize(1.0f);
}

void buildHurt(Synth& s)
{
    s.addTone(0.0f, 0.28f, 260.0f, 90.0f, 0.6f, 2, 0.002f, 1.6f);
    s.addNoise(0.0f, 0.18f, 0.35f, 3000.0f, 300.0f, 0.001f, 2.2f);
    s.finalize(1.1f);
}

void buildReload(Synth& s)
{
    s.addNoise(0.0f, 0.05f, 0.5f, 9000.0f, 3000.0f, 0.001f, 3.5f);
    s.addNoise(0.16f, 0.06f, 0.55f, 7000.0f, 2000.0f, 0.001f, 3.0f);
    s.addTone(0.16f, 0.05f, 300.0f, 200.0f, 0.18f, 1, 0.001f, 3.0f);
    s.finalize(1.0f);
}

void buildEmpty(Synth& s)
{
    s.addNoise(0.0f, 0.04f, 0.5f, 12000.0f, 5000.0f, 0.001f, 4.0f);
    s.addTone(0.0f, 0.03f, 220.0f, 180.0f, 0.15f, 1, 0.001f, 3.0f);
    s.finalize(0.9f);
}

void buildWaveStart(Synth& s)
{
    s.addTone(0.0f, 0.65f, 220.0f, 330.0f, 0.30f, 0, 0.12f, 1.2f);
    s.addTone(0.0f, 0.65f, 277.0f, 415.0f, 0.22f, 0, 0.14f, 1.2f);
    s.addTone(0.0f, 0.65f, 330.0f, 494.0f, 0.20f, 3, 0.16f, 1.2f);
    s.finalize(1.0f);
}

void buildDash(Synth& s)
{
    s.addNoise(0.0f, 0.18f, 0.5f, 800.0f, 5000.0f, 0.01f, 2.0f);
    s.addTone(0.0f, 0.16f, 700.0f, 300.0f, 0.15f, 3, 0.005f, 2.0f);
    s.finalize(1.0f);
}

void buildBossRoar(Synth& s)
{
    s.addTone(0.0f, 0.85f, 70.0f, 45.0f, 0.65f, 2, 0.05f, 1.3f);
    s.addTone(0.0f, 0.85f, 105.0f, 68.0f, 0.35f, 1, 0.08f, 1.3f);
    s.addNoise(0.0f, 0.85f, 0.30f, 900.0f, 200.0f, 0.05f, 1.5f);
    s.finalize(1.1f);
}

void buildGameOver(Synth& s)
{
    s.addTone(0.00f, 0.22f, 440.0f, 440.0f, 0.30f, 3, 0.005f, 1.4f);
    s.addTone(0.18f, 0.22f, 330.0f, 330.0f, 0.30f, 3, 0.005f, 1.4f);
    s.addTone(0.36f, 0.60f, 220.0f, 110.0f, 0.35f, 3, 0.005f, 1.6f);
    s.finalize(1.0f);
}

void buildAll(const std::wstring& directory, std::wstring* out)
{
    struct Recipe
    {
        const wchar_t* name;
        float duration;
        void (*build)(Synth&);
    };
    const Recipe recipes[] = {
        {L"shoot", 0.12f, buildShoot},
        {L"hit", 0.10f, buildHit},
        {L"kill", 0.26f, buildKill},
        {L"explosion", 0.60f, buildExplosion},
        {L"pickup", 0.20f, buildPickup},
        {L"heal", 0.38f, buildHeal},
        {L"hurt", 0.30f, buildHurt},
        {L"reload", 0.26f, buildReload},
        {L"empty", 0.06f, buildEmpty},
        {L"wave", 0.70f, buildWaveStart},
        {L"dash", 0.20f, buildDash},
        {L"roar", 0.90f, buildBossRoar},
        {L"over", 1.00f, buildGameOver},
    };

    for (int i = 0; i < static_cast<int>(Sfx::Count); ++i) {
        Synth synth(recipes[i].duration);
        recipes[i].build(synth);
        std::wstring path = directory + L"ls_" + recipes[i].name + L".wav";
        if (writeWav(path, synth)) {
            out[i] = path;
        }
    }
}

// ------------------------------------------------------------- MCI 封装 ----
bool mciOpen(const std::wstring& alias, const std::wstring& file)
{
    std::wstring command = L"open \"" + file + L"\" type waveaudio alias " + alias;
    MCIERROR error = mciSendStringW(command.c_str(), nullptr, 0, nullptr);
    return error == 0;
}

void mciSend(const std::wstring& command)
{
    mciSendStringW(command.c_str(), nullptr, 0, nullptr);
}

} // namespace

Audio& audio()
{
    static Audio instance;
    return instance;
}

bool Audio::init()
{
    // 音效落在临时目录，不污染仓库
    wchar_t tempPath[MAX_PATH]{};
    DWORD length = GetTempPathW(MAX_PATH, tempPath);
    if (length == 0 || length >= MAX_PATH) return false;
    std::wstring directory = tempPath;
    directory += L"lastShooter_sfx\\";
    CreateDirectoryW(directory.c_str(), nullptr);

    buildAll(directory, files_);

    // 每个音效开两条通道，保证同一种音效可以重叠播放两次
    for (int i = 0; i < static_cast<int>(Sfx::Count); ++i) {
        if (files_[i].empty()) continue;
        for (int slot = 0; slot < kVoicesPerSfx; ++slot) {
            const std::wstring alias = aliasFor(i, slot);
            if (mciOpen(alias, files_[i])) {
                aliasOpen_[i][slot] = true;
                ready_ = true;
            }
        }
    }
    LS_LOG("audio init: %s", ready_ ? "ok" : "failed");
    return ready_;
}

std::wstring Audio::aliasFor(int sfx, int slot)
{
    return L"ls_" + std::to_wstring(sfx) + L"_" + std::to_wstring(slot);
}

void Audio::shutdown()
{
    for (int i = 0; i < static_cast<int>(Sfx::Count); ++i) {
        for (int slot = 0; slot < kVoicesPerSfx; ++slot) {
            if (aliasOpen_[i][slot]) {
                mciSend(L"close " + aliasFor(i, slot));
                aliasOpen_[i][slot] = false;
            }
        }
    }
    ready_ = false;
}

void Audio::play(Sfx sfx, float volume)
{
    (void)volume; // 音量在合成阶段就写进了波形（MCI 的 waveaudio 通道不支持独立音量）
    if (!ready_) return;
    const int index = static_cast<int>(sfx);
    if (index < 0 || index >= static_cast<int>(Sfx::Count)) return;

    // 限流：MCI 调用是同步的，同一音效刷太密会拖慢主循环
    const ULONGLONG now = GetTickCount64();
    if (now - lastPlay_[index] < kMinIntervalMs) return;
    lastPlay_[index] = now;

    int slot = nextSlot_[index];
    for (int attempt = 0; attempt < kVoicesPerSfx; ++attempt) {
        const int candidate = (slot + attempt) % kVoicesPerSfx;
        if (!aliasOpen_[index][candidate]) continue;
        const std::wstring alias = aliasFor(index, candidate);
        mciSend(L"seek " + alias + L" to start");
        if (mciSendStringW((L"play " + alias).c_str(), nullptr, 0, nullptr) == 0) {
            nextSlot_[index] = (candidate + 1) % kVoicesPerSfx;
            return;
        }
    }
}

} // namespace ls
