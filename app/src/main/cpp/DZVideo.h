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
};

#include "DZJNICall.h"
#include "DZConstDefine.h"
#include "DZAVPacketQueue.h"
#include "DZPlayerStatus.h"

class DZVideo{
public:

};

#endif //INC_011_FFMPEG_MASTER_DZVIDEO_H
