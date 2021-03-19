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
};
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "DZJNICall.h"
#include "DZConstDefine.h"
#include "DZAVPacketQueue.h"
#include "DZPlayerStatus.h"
#include "DZMedia.h"

class DZVideo : public DZMedia{
public:
    SwsContext *pSwsContext = NULL;
    uint8_t *pFrameBuffer = NULL;
    AVFrame *pRGBAFrame = NULL;
    int mFrameSize = 0;
    jobject pSurface = NULL;

public:
    DZVideo(int audioStreamIndex, DZJNICall *jniCall, DZPlayerStatus *playerStatus);
    ~DZVideo();
    void play();
    void privateAnalysisStream(ThreadMode threadMode, AVFormatContext *pFormatContext);
    void resampleVideo(void *context);
    void release();
    void setSurface(jobject object);
};

#endif //INC_011_FFMPEG_MASTER_DZVIDEO_H
