#define MINIMP3_ONLY_MP3
#define MINIMP3_IMPLEMENTATION
#include "minimp3_ex.h"
#include "audio.hpp"
#include <malloc.h>
#include <cstring>
#include <filesystem>

extern FILE* g_log;

constexpr size_t outputChannels = 2;        //Number of channels to output
constexpr int voiceNum = 0;

#define BASE_PATH "sdmc:/atmosphere/contents/4200000000000BA6/"
constexpr const char* connectFilePath = BASE_PATH "connect.mp3"; 
constexpr const char* disconnectFilePath = BASE_PATH "disconnect.mp3"; 

audio::audio()
{
    Result res = audrvCreate(&m_driver, &audrenCfg, outputChannels);
    if (R_FAILED(res)) {
        fatalThrow(res);
    }

    constexpr u8 sink_channels[] = { 0, 1 };
    audrvDeviceSinkAdd(&m_driver, AUDREN_DEFAULT_DEVICE_NAME, 2, sink_channels);

    res = audrvUpdate(&m_driver);
    fprintflush(g_log, "audrvUpdate res: %08X\n", res);
    res = audrenStartAudioRenderer();
    fprintflush(g_log, "audrenStartAudioRenderer res: %08X\n", res);
}

audio::~audio() {
    if (m_mempool != NULL) {
        free(m_mempool);
        m_mempool = NULL;
    }
}

audio& audio::getInstance() {
    static audio instance;
    return instance;
}

void audio::play(const char* filePath) { //hacky single threaded audio player
    if (!std::filesystem::exists(filePath)) {
        fprintflush(g_log, "Path doesn't exist: %s\n", filePath);
        return;
    }

    mp3dec_t mp3d;
    mp3dec_file_info_t info;
    if (mp3dec_load(&mp3d, filePath, &info, NULL, NULL)) {
        fprintflush(g_log, "Couldn't load mp3!\n");
        return;
    }

    size_t samplesSize = sizeof(mp3d_sample_t)*info.samples;
    size_t realBufSize = ((samplesSize + (AUDREN_MEMPOOL_ALIGNMENT - 1)) &~ (AUDREN_MEMPOOL_ALIGNMENT - 1));

    if (m_voice > -1) {
        audrvVoiceDrop(&m_driver, m_voice);
        audrvUpdate(&m_driver);
    }

    m_voice = voiceNum;
    bool init = audrvVoiceInit(&m_driver, m_voice, info.channels, PcmFormat_Int16, info.hz);
    fprintflush(g_log, "audrvVoiceInit: %d!\n", (u8)init);
    if (!init) {
        return;
    }

    audrvVoiceSetDestinationMix(&m_driver, m_voice, AUDREN_FINAL_MIX_ID);
    if (info.channels == 1) { // Mono audio
        audrvVoiceSetMixFactor(&m_driver, m_voice, 1.0f, 0, 0);
        audrvVoiceSetMixFactor(&m_driver, m_voice, 1.0f, 0, 1);
    } else { // Stereo
        audrvVoiceSetMixFactor(&m_driver, m_voice, 1.0f, 0, 0);
        audrvVoiceSetMixFactor(&m_driver, m_voice, 0.0f, 0, 1);
        audrvVoiceSetMixFactor(&m_driver, m_voice, 0.0f, 1, 0);
        audrvVoiceSetMixFactor(&m_driver, m_voice, 1.0f, 1, 1);
    }

    if (m_mempoolID > -1) {
        audrvMemPoolDetach(&m_driver, m_mempoolID);
        audrvMemPoolRemove(&m_driver, m_mempoolID);
        m_mempoolID = -1;
    }

    if (m_mempool != NULL) {
        free(m_mempool);
        m_mempool = NULL;
    }

    m_mempool = aligned_alloc(AUDREN_MEMPOOL_ALIGNMENT, realBufSize);
    if (m_mempool == NULL) {
        fprintflush(g_log, "aligned_alloc Failed!\n");
        return;
    }

    std::memset(m_mempool, 0, realBufSize);
    std::memcpy(m_mempool, info.buffer, realBufSize);
    armDCacheFlush(m_mempool, realBufSize);
    free(info.buffer);


    m_mempoolID = audrvMemPoolAdd(&m_driver, m_mempool, realBufSize);
    audrvMemPoolAttach(&m_driver, m_mempoolID);

    m_waveBuf.data_raw = m_mempool;
    m_waveBuf.size = realBufSize;
    m_waveBuf.start_sample_offset = 0;
    m_waveBuf.end_sample_offset = realBufSize/(2*info.channels);

    audrvVoiceAddWaveBuf(&m_driver, m_voice, &m_waveBuf);
    audrvUpdate(&m_driver);
    audrvVoiceStart(&m_driver, m_voice);

    while(m_waveBuf.state == AudioDriverWaveBufState_Playing) {
        fprintflush(g_log, "Sample Count: %d\n", audrvVoiceGetPlayedSampleCount(&m_driver, m_voice));
        audrvUpdate(&m_driver);
    }
    fprintflush(g_log, "Finished playing!\n");
}

void audio::playConnectSound() {
    svcSleepThread(1e+9); //Wait 1 second for connection to fully go through
    this->play(connectFilePath);
}

void audio::playDisconnectSound() {
    this->play(disconnectFilePath);
}
