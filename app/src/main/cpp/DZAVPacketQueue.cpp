//
// Created by bingy on 2021/3/17.
//

#include "DZAVPacketQueue.h"

void DZAVPacketQueue::clear() {
    //清楚队列，和每个AVPacket*的内存数据
    if(pPacketQueue == NULL){
        return;
    }

    while(!pPacketQueue->empty()) {
        AVPacket *packet = pPacketQueue->front();
        pPacketQueue->pop();
        free(packet);
        packet = NULL;
    }
}

DZAVPacketQueue::DZAVPacketQueue() {
    pPacketQueue = new std::queue<AVPacket *>();
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
}

DZAVPacketQueue::~DZAVPacketQueue() {
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    if(pPacketQueue != NULL){
        clear();
        delete pPacketQueue;
        pPacketQueue = NULL;
    }
}

void DZAVPacketQueue::push(AVPacket *packet) {
    if(pPacketQueue == NULL){
        return;
    }

    pthread_mutex_lock(&mutex);
    pPacketQueue->push(packet);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

AVPacket *DZAVPacketQueue::pop() {
    if(pPacketQueue == NULL){
        return NULL;
    }

    pthread_mutex_lock(&mutex);
    while(pPacketQueue->empty()){
        pthread_cond_wait(&cond, &mutex);
    }
    AVPacket *packet = pPacketQueue->front();
    pPacketQueue->pop();
    pthread_mutex_unlock(&mutex);
    return packet;
}
