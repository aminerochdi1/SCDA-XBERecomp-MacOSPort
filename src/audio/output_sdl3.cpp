#ifndef _WIN32
#include "audio/output.h"

#include <SDL3/SDL.h>

#include <atomic>
#include <vector>

#include "core/log.h"

namespace audio {

namespace {

std::atomic<bool> g_surround{false};
std::atomic<bool> g_mono{false};
std::atomic<uint32_t> g_mode{kOutputAuto};
SDL_AudioStream* g_stream = nullptr;
std::function<void(float* out, uint32_t frames)> g_render;

void SDLCALL AudioCallback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount) {
    if (additional_amount <= 0 || !g_render) return;
    int frames = additional_amount / (kChannels * sizeof(float));
    if (frames <= 0) return;

    std::vector<float> buffer(frames * kChannels, 0.0f);
    g_render(buffer.data(), uint32_t(frames));

    if (g_mono) {
        constexpr float kCentreFold = 0.707f;
        for (int i = 0; i < frames; ++i) {
            float* f = &buffer[i * kChannels];
            float m = (f[FrontLeft] + f[FrontRight] + f[Centre] * kCentreFold) * 0.5f;
            f[FrontLeft] = m;
            f[FrontRight] = m;
        }
    }

    SDL_PutAudioStreamData(stream, buffer.data(), frames * kChannels * sizeof(float));
}

}  // namespace

void StartOutput(std::function<void(float* out, uint32_t frames)> render) {
    if (!SDL_WasInit(SDL_INIT_AUDIO)) {
        if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
            LOGE("audio: SDL_InitSubSystem(AUDIO) failed: %s", SDL_GetError());
            return;
        }
    }

    g_render = std::move(render);

    SDL_AudioSpec spec{};
    spec.format = SDL_AUDIO_F32;
    spec.channels = kChannels;
    spec.freq = kRate;

    g_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, AudioCallback, nullptr);
    if (!g_stream) {
        LOGE("audio: SDL_OpenAudioDeviceStream failed: %s", SDL_GetError());
        return;
    }

    g_surround = (g_mode == kOutputSurround);
    g_mono = (g_mode == kOutputMono);

    SDL_ResumeAudioStreamDevice(g_stream);
    LOGI("audio: SDL3 audio output initialized (%u Hz, %u channels)", kRate, kChannels);
}

bool IsSurround() { return g_surround.load(); }
bool IsMono() { return g_mono.load(); }

void SetOutputMode(uint32_t mode) {
    g_mode = mode;
    g_surround = (mode == kOutputSurround);
    g_mono = (mode == kOutputMono);
}

}  // namespace audio
#endif  // !_WIN32
