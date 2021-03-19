//
// Created by bingy on 2021/3/18.
//

#ifndef INC_011_FFMPEG_MASTER_DZMEDIA_H
#define INC_011_FFMPEG_MASTER_DZMEDIA_H

extern "C" {
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libavcodec/avcodec.h"
};

#include "DZJNICall.h"
#include "DZConstDefine.h"
#include "DZAVPacketQueue.h"
#include "DZPlayerStatus.h"


class DZMedia{
public:
    int streamIndex = -1;
    AVCodecContext* pCodecContext = NULL;
    DZJNICall *pJniCall = NULL;
    DZAVPacketQueue* pPacketQueue = NULL;
    DZPlayerStatus* pPlayerStatus = NULL;
    bool async = false;

    //record current play time
    double currentTime = 0;
    //回调到Java层，上次更新的时间
    double lastUpdateTime = 0;
    //整个视频的时长
    int duration = 0;

    //时间基
    AVRational timeBase;

public:
    DZMedia(int streamIndex, DZJNICall *pJniCall, DZPlayerStatus* pPlayerStatus);
    ~DZMedia();

public:
    virtual void play() = 0;
    void analysisStream(ThreadMode threadMode, AVFormatContext *pFormatContext);
    virtual void privateAnalysisStream(ThreadMode threadMode, AVFormatContext *pFormatContext) = 0;
    virtual void release();
    void callPlayerJniError(ThreadMode threadMode, int code, char *msg);
private:
    void publicAnalysisStream(ThreadMode threadMode, AVFormatContext *pFormatContext);

};

#endif //INC_011_FFMPEG_MASTER_DZMEDIA_H
