//
// Created by bingy on 2021/3/14.
//

#ifndef INC_011_FFMPEG_MASTER_DZFFMPEG_H
#define INC_011_FFMPEG_MASTER_DZFFMPEG_H

extern "C" {
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
};

#include "DZJNICall.h"
#include "DZAudio.h"
#include "DZConstDefine.h"
#include <pthread.h>


class DZFFmpeg{
public:
    AVFormatContext *pFormatContext = NULL;
    AVCodecContext* pCodecContext = NULL;
    SwrContext *swrContext = NULL;
    char* url = NULL;
    DZJNICall *pJniCall = NULL;
    DZAudio *pAudio = NULL;
    uint8_t *resampleOutBuffer = NULL;

public:
    DZFFmpeg(DZJNICall *dzjniCall, const char* url);
    ~DZFFmpeg();

public:
    void play();
    void onJniPlayError(ThreadMode threadMode, int code, char *msg);
    void prepare();
    void prepareAsync();
    void prepare(ThreadMode threadMode);
    void prepareAsync(ThreadMode threadMode);

private:
    void release();
};

#endif //INC_011_FFMPEG_MASTER_DZFFMPEG_H
