//
// Created by bingy on 2021/3/18.
//

#include "DZMedia.h"

void DZMedia::analysisStream(ThreadMode threadMode, AVFormatContext *pFormatContext) {
    if(streamIndex >= 0){
        publicAnalysisStream(threadMode, pFormatContext);
        privateAnalysisStream(threadMode, pFormatContext);
    } else {
        LOGE("analysisStream streamIndex is -1");
    }
}

DZMedia::~DZMedia() {
    release();
}

DZMedia::DZMedia(int streamIndex, DZJNICall *pJniCall, DZPlayerStatus *pPlayerStatus) {
    this->streamIndex = streamIndex;
    this->pJniCall = pJniCall;
    this->pPlayerStatus = pPlayerStatus;
    pPacketQueue = new DZAVPacketQueue();
}

void DZMedia::release() {
    //release public part
    if(pPacketQueue != NULL){
        delete pPacketQueue;
        pPacketQueue = NULL;
    }
    if(pCodecContext != NULL){
        avcodec_close(pCodecContext);
        avcodec_free_context(&pCodecContext);
        pCodecContext = NULL;
    }
}

void DZMedia::publicAnalysisStream(ThreadMode threadMode, AVFormatContext *pFormatContext) {
    AVCodecParameters *pCodecParameters = pFormatContext->streams[streamIndex]->codecpar;
    //查找解码
    AVCodec *pCodec = avcodec_find_decoder(pCodecParameters->codec_id);
    if(pCodec == NULL){
        LOGE("avCodec is null");
        callPlayerJniError(threadMode, CODED_FIND_DECODER_ERROR_CODE, "avCodec is null");
        return;
    }


    //打开解码器
    pCodecContext = avcodec_alloc_context3(pCodec);
    if(pCodecContext == NULL){
        LOGE("pCodecContext is null");
        callPlayerJniError(threadMode, ALLOCATE_CONTEXT_ERROR_CODE, "pCodecContext is null");
        return;
    }

    int avcodecParametersToContextRes = avcodec_parameters_to_context(pCodecContext, pCodecParameters);
    if(avcodecParametersToContextRes  < 0){
        LOGE("codec parameters to context error : %s ", av_err2str(avcodecParametersToContextRes));
        callPlayerJniError(threadMode, AVCODEC_PARAM_TO_CONTEXT_ERROR_CODE, av_err2str(avcodecParametersToContextRes));
        return;
    }
    int avcodecOpenRes = avcodec_open2(pCodecContext, pCodec, NULL);
    if(avcodecOpenRes != 0){
        LOGE("codec audio open fail : %s ", av_err2str(avcodecOpenRes));
        callPlayerJniError(threadMode, AVCODEC_OPEN_2_ERROR_CODE, av_err2str(avcodecOpenRes));
        return;
    }
//    avcodec_open2(pFormatContext->streams[audioStreamIndex]->codec, pCodec, NULL);

    LOGE("%d, %d", pCodecParameters->sample_rate, pCodecParameters->channels);

    duration = pFormatContext->duration;
    timeBase = pFormatContext->streams[streamIndex]->time_base;
}

void DZMedia::callPlayerJniError(ThreadMode threadMode, int code, char *msg) {
    release();
    pJniCall->onPlayError(threadMode, code, msg);
}
