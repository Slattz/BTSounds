#pragma once

#include <mutex>
#include <switch.h>
#include <cstdio>

#define ENABLE_LOGGING 0

#if ENABLE_LOGGING == 1
#define fprintflush(file, string, ...) \
    fprintf(file, string, ## __VA_ARGS__); \
    fflush(file)

#else

#define fprintflush(file, string, ...) \
    (void)file; (void)string;
#endif

constexpr AudioRendererConfig audrenCfg = {
    .output_rate     = AudioRendererOutputRate_48kHz,
    .num_voices      = 4,
    .num_effects     = 0,
    .num_sinks       = 1,
    .num_mix_objs    = 1,
    .num_mix_buffers = 2,
};

class audio
{
public:
    static audio& getInstance();

    void playConnectSound();
    void playDisconnectSound();
private:
    audio();
    ~audio();
    
    void play(const char* filePath);

    AudioDriver m_driver;
    void* m_mempool = NULL;
    int m_voice = -1;
    int m_mempoolID = -1;
    AudioDriverWaveBuf m_waveBuf = {0};

};

