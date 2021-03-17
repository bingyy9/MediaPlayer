//
// Created by bingy on 2021/3/17.
//

#ifndef INC_011_FFMPEG_MASTER_DZAVPACKETQUEUE_H
#define INC_011_FFMPEG_MASTER_DZAVPACKETQUEUE_H

#include <queue>
#include <pthread.h>

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

using namespace std;

class DZAVPacketQueue{
private:
    std::queue<AVPacket *> *pPacketQueue;
    pthread_mutex_t mutex;
    pthread_cond_t cond;


public:
    DZAVPacketQueue();
    ~DZAVPacketQueue();

    void push(AVPacket *packet);
    AVPacket *pop();
    void clear();
};

#endif //INC_011_FFMPEG_MASTER_DZAVPACKETQUEUE_H
