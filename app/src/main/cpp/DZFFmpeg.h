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
#include "DZConstDefine.h"


class DZFFmpeg{
public:
    AVFormatContext *pFormatContext = NULL;
//    AVCodecParameters *pCodecParameters = NULL;
//    AVCodec *pCodec = NULL;
    AVCodecContext* pCodecContext = NULL;
    SwrContext *swrContext = NULL;
    uint8_t *resampleOutBuffer = NULL;
    const char* url = NULL;
    DZJNICall *pJniCall = NULL;
public:
    DZFFmpeg(DZJNICall *dzjniCall, const char* url);
    ~DZFFmpeg();

public:
    void play();

};

#endif //INC_011_FFMPEG_MASTER_DZFFMPEG_H
