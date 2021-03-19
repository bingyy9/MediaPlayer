//
// Created by bingy on 2021/3/17.
//

#include "DZAudio.h"

DZAudio::DZAudio(int audioStreamIndex, DZJNICall *jniCall, DZPlayerStatus *playerStatus) : DZMedia(audioStreamIndex, jniCall, playerStatus){
}

DZAudio::~DZAudio() {
    release();
}

void* audioThreadPlay(void * context){
    LOGE("audioThreadPlay====");
    DZAudio* audio = (DZAudio *)context;
    LOGE("audioThreadPlay %p " , audio);
    audio->initCreateOpenSELS();
    LOGE("audioThreadPlay222 %p " , audio);
}

// this callback handler is called every time a buffer finishes playing
void playerCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
    LOGE("bqPlayerCallbackDN========");
    DZAudio *pAudio = (DZAudio *)(context);
    int dataSize = pAudio->resampleAudio(context);
    (*bq)->Enqueue(bq, pAudio->resampleOutBuffer, dataSize);
//    LOGE("bqPlayerCallbackDN----- begin");
//    if(pcmFile != NULL && !feof(pcmFile)){
//        fread(pcmBuffer, 1, 44100*2*2, pcmFile);
//        // enqueue another buffer
//        (*bqPlayerBufferQueue)->Enqueue(bq, pcmBuffer, 44100*2*2);
//    } else {
//        if(pcmFile != NULL){
//            fclose(pcmFile);
//            free(pcmBuffer);
//            pcmBuffer = NULL;
//            pcmFile = NULL;
//        }
//    }
}

void DZAudio::initCreateOpenSELS(){
    LOGE("initCreateOpenSELS");
//    •	创建引擎接口对象
//    •	设置混音器并且设置参数
//    •	创建播放器
//    •	设置缓存队列和回调函数
//    •	调用回调函数
    SLObjectItf engineObject = NULL;
    SLEngineItf engineEngine;
    slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    LOGE("initCreateOpenSELS1111");
    // realize the engine
    (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    // get the engine interface, which is needed in order to create other objects
    (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    // 3.2 设置混音器
    static SLObjectItf outputMixObject = NULL;
    const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean req[1] = {SL_BOOLEAN_FALSE};
    (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, ids, req);
    (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;
    (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                     &outputMixEnvironmentalReverb);
    SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;
    (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(outputMixEnvironmentalReverb,
                                                                      &reverbSettings);
    // 3.3 创建播放器
    SLObjectItf pPlayer = NULL;
    SLPlayItf pPlayItf = NULL;
    SLDataLocator_AndroidSimpleBufferQueue simpleBufferQueue = {
            SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM formatPcm = {
            SL_DATAFORMAT_PCM,
            2,
            SL_SAMPLINGRATE_44_1,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
            SL_BYTEORDER_LITTLEENDIAN};
    SLDataSource audioSrc = {&simpleBufferQueue, &formatPcm};
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&outputMix, NULL};
    SLInterfaceID interfaceIds[3] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_PLAYBACKRATE};
    SLboolean interfaceRequired[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
    (*engineEngine)->CreateAudioPlayer(engineEngine, &pPlayer, &audioSrc, &audioSnk, 3,
                                       interfaceIds, interfaceRequired);
    (*pPlayer)->Realize(pPlayer, SL_BOOLEAN_FALSE);
    (*pPlayer)->GetInterface(pPlayer, SL_IID_PLAY, &pPlayItf);
    // 3.4 设置缓存队列和回调函数
    SLAndroidSimpleBufferQueueItf playerBufferQueue;
    (*pPlayer)->GetInterface(pPlayer, SL_IID_BUFFERQUEUE, &playerBufferQueue);
    // 每次回调 this 会被带给 playerCallback 里面的 context
    (*playerBufferQueue)->RegisterCallback(playerBufferQueue, playerCallback, this);
    // 3.5 设置播放状态
    (*pPlayItf)->SetPlayState(pPlayItf, SL_PLAYSTATE_PLAYING);
    // 3.6 调用回调函数
    playerCallback(playerBufferQueue, this);
    LOGE("initCreateOpenSELS22222");
}

int DZAudio::resampleAudio(void *context){
    DZAudio *pAudio = (DZAudio *)(context);
    int dataSize = 0;
    AVPacket *pPacket = NULL;
    AVFrame *pFrame = av_frame_alloc();
    while(pAudio->pPlayerStatus != NULL && !pAudio->pPlayerStatus->isExist){
        pPacket = pAudio->pPacketQueue->pop();
        if(pPacket == NULL){
            //sleep
            continue;
        }
        //Packet包，压缩的数据，解码成pcm数据
        int avcodecSendPacketRes = avcodec_send_packet(pAudio->pCodecContext, pPacket);
        if (avcodecSendPacketRes == 0) {
            int avcodecReceiveFrameRes = avcodec_receive_frame(pAudio->pCodecContext, pFrame);
            if (avcodecReceiveFrameRes == 0) {
                //已经把AVPacket解码成AVFrame
                LOGE("DZAudio 解码帧");
                //调用重采样的方法：dataSize 返回的是重采样的个数，也就是pFrame->nb_samples
                dataSize = swr_convert(pAudio->swrContext, &pAudio->resampleOutBuffer, pFrame->nb_samples, (const uint8_t **)(pFrame->data), pFrame->nb_samples);
                dataSize = dataSize * 2 * 2;
                LOGE("DZAudio 解码帧，dataSize = %d, nb_samples = %d, frame_size = %d", dataSize, pFrame->nb_samples, pAudio->pCodecContext->frame_size);
//                    //在native层创建C数组
//                    memcpy(jPcmData, resampleOutBuffer, avSamplesBufferSize);
//                    //传0同步到java jbyteArray，并释放native jbyte*数组， 参考数组的细节处理章节.
//                    //  JNI_COMMIT仅仅同步，不释放native数组
//                    jniEnv->ReleaseByteArrayElements(jPcmByteArray, jPcmData, JNI_COMMIT);
//                    pJniCall->callAudioTrackWrite(threadMode, jPcmByteArray, 0, avSamplesBufferSize);
                break;
            }
        }
        av_packet_unref(pPacket);
        av_frame_unref(pFrame);
    }

    av_packet_free(&pPacket);
    av_frame_free(&pFrame);
    return dataSize;
};



void DZAudio::play() {
    if(async){
        //一个线程解码播放
        pthread_t playThreadT;
        pthread_create(&playThreadT, NULL, audioThreadPlay, this);
        pthread_detach(playThreadT);
    } else {
        initCreateOpenSELS();
    }
}

void DZAudio::privateAnalysisStream(ThreadMode threadMode, AVFormatContext *pFormatContext) {
    //--------------重采样start
    int64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
    enum AVSampleFormat out_sample_fmt = AVSampleFormat::AV_SAMPLE_FMT_S16;
    int out_sample_rate = AUDIO_SAMPLE_RATE; //44100
    int64_t in_ch_layout = pCodecContext->channel_layout;
    enum AVSampleFormat in_sample_fmt = pCodecContext->sample_fmt;
    int in_sample_rate = pCodecContext->sample_rate;
    swrContext = swr_alloc_set_opts(NULL, out_ch_layout, out_sample_fmt, out_sample_rate
            , in_ch_layout, in_sample_fmt, in_sample_rate, 0, NULL);
    if(swrContext == NULL){
        onJniPlayError(threadMode, AVCODEC_OPEN_2_ERROR_CODE, "swrContext is null");
        return;
    }
    int swrInitRes = swr_init(swrContext);
    if(swrInitRes < 0){
        onJniPlayError(threadMode, AVCODEC_OPEN_2_ERROR_CODE, "swrInitRes < 0 is null");
        return;
    }
    //write pcm到缓冲区。pFrame->data -> javabyte
    //1s 44100采样点， 2通道， 16字节   1秒的大小=44100*2*2
    //1帧不是1秒， 1秒都多少帧pFrame->nb_samples
    //size是播放指定的大小，是最终输出的大小
//    int outChannels = av_get_channel_layout_nb_channels(out_ch_layout);
//    int avSamplesBufferSize = av_samples_get_buffer_size(NULL, outChannels, pCodecParameters->frame_size, out_sample_fmt, 0);
    resampleOutBuffer = (uint8_t *)(malloc(pCodecContext->frame_size * 2 * 2));
//    uint8_t *resampleOutBuffer = (uint8_t *)malloc(avSamplesBufferSize);
}

void DZAudio::onJniPlayError(ThreadMode threadMode, int code, char *msg) {
    //释放资源
    release();
    pJniCall->onPlayError(threadMode, code, msg);
}

void DZAudio::release() {
    DZMedia::release();
    if(swrContext != NULL){
        swr_free(&swrContext);
        free(swrContext);
        swrContext = NULL;
    }

    if(resampleOutBuffer != NULL){
        free(resampleOutBuffer);
        resampleOutBuffer = NULL;
    }

    if(pPlayerStatus != NULL){
        delete pPlayerStatus;
        pPlayerStatus = NULL;
    }
}


