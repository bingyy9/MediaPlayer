//
// Created by bingy on 2021/3/17.
//

#ifndef INC_011_FFMPEG_MASTER_DZAUDIO_H
#define INC_011_FFMPEG_MASTER_DZAUDIO_H

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <pthread.h>
#include <assert.h>

extern "C" {
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
};

#include "DZJNICall.h"
#include "DZConstDefine.h"
#include "DZAVPacketQueue.h"
#include "DZPlayerStatus.h"


class DZAudio{
public:
    AVFormatContext *pFormatContext = NULL;
    AVCodecContext* pCodecContext = NULL;
    SwrContext *swrContext = NULL;
    uint8_t *resampleOutBuffer = NULL;
    DZJNICall *pJniCall = NULL;
    int audioStreamIndex = -1;

    // engine interfaces
    SLObjectItf engineObject = NULL;
    SLEngineItf engineEngine;
    // output mix interfaces
    SLObjectItf outputMixObject = NULL;
    SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;
    // buffer queue player interfaces
    SLObjectItf bqPlayerObject = NULL;
    SLPlayItf bqPlayerPlay;
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
    SLEffectSendItf bqPlayerEffectSend;
    SLMuteSoloItf bqPlayerMuteSolo;
    SLVolumeItf bqPlayerVolume;
    SLmilliHertz bqPlayerSampleRate = 0;
    jint   bqPlayerBufSize = 0;
    short *resampleBuf = NULL;
    FILE *pcmFile = NULL;
    void *pcmBuffer = NULL;
    // aux effect on the output mix, used by the buffer queue player
    const SLEnvironmentalReverbSettings reverbSettings =
            SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;
    DZAVPacketQueue* pPacketQueue = NULL;
    DZPlayerStatus* pPlayerStatus = NULL;
    bool async = false;

public:
    DZAudio(int audioStreamIndex, DZJNICall *jniCal, AVFormatContext *pFormatContext);
    ~DZAudio();

    void play();

    void initCreateOpenSELS();

    int resampleAudio();
    int resampleAudio2();

    static void bqPlayerCallbackDN(SLAndroidSimpleBufferQueueItf bq, void *context);

    void analysisStream(ThreadMode mode, AVStream **pStream);

    void onJniPlayError(ThreadMode threadMode, int code, char *msg);

    void audio_release();
};


#endif //INC_011_FFMPEG_MASTER_DZAUDIO_H
