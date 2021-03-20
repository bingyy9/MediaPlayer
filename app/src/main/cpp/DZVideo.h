//
// Created by bingy on 2021/3/18.
//

#ifndef INC_011_FFMPEG_MASTER_DZVIDEO_H
#define INC_011_FFMPEG_MASTER_DZVIDEO_H

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <pthread.h>
#include <assert.h>

extern "C" {
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libavutil/time.h"
};
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "DZJNICall.h"
#include "DZConstDefine.h"
#include "DZAVPacketQueue.h"
#include "DZPlayerStatus.h"
#include "DZMedia.h"
#include "DZAudio.h"

class DZVideo : public DZMedia{
public:
    SwsContext *pSwsContext = NULL;
    uint8_t *pFrameBuffer = NULL;
    AVFrame *pRGBAFrame = NULL;
    int mFrameSize = 0;
    jobject pSurface = NULL;
    DZAudio* pAudio;
    //视频的延时时间
    double delayTime = 0;
    //默认情况下最合适的延迟时间 一般1秒24帧 = 0.04
    double defaultDelayTime = 0.04;

public:
    DZVideo(int audioStreamIndex, DZJNICall *jniCall, DZPlayerStatus *playerStatus, DZAudio* pAudio);
    ~DZVideo();
    void play();
    void privateAnalysisStream(ThreadMode threadMode, AVFormatContext *pFormatContext);
    void resampleVideo(void *context);
    void release();
    void setSurface(jobject object);

    //视频同步音频，计算视频需要休眠的时间，单位s
    double getFrameSleepTime(AVFrame *pFrame);
};

#endif //INC_011_FFMPEG_MASTER_DZVIDEO_H
