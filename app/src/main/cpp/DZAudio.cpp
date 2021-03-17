//
// Created by bingy on 2021/3/17.
//

#include "DZAudio.h"

DZAudio::DZAudio(int audioStreamIndex, DZJNICall *jniCall, AVCodecContext *codecContext, AVFormatContext *formatContext, SwrContext *swrContext) {
    this->audioStreamIndex = audioStreamIndex;
    this->pJniCall = jniCall;
    this->pCodecContext = codecContext;
    this->pFormatContext = formatContext;
    this->swrContext = swrContext;
    resampleOutBuffer = (uint8_t *)(malloc(pCodecContext->frame_size * 2 * 2));
}

DZAudio::~DZAudio() {
    if(resampleOutBuffer != NULL){
        free(resampleOutBuffer);
        resampleOutBuffer = NULL;
    }
}

void* audioThreadPlay(void * context){
    DZAudio* audio = (DZAudio *)context;
    audio->initCreateOpenSELS();
}

void DZAudio::initCreateOpenSELS(){
//    •	创建引擎接口对象
//    •	设置混音器并且设置参数
//    •	创建播放器
//    •	设置缓存队列和回调函数
//    •	调用回调函数
    SLresult result;

    // create engine
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // realize the engine
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the engine interface, which is needed in order to create other objects
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // create output mix, with environmental reverb specified as a non-required interface
    const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean req[1] = {SL_BOOLEAN_FALSE};
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, ids, req);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // realize the output mix
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the environmental reverb interface
    // this could fail if the environmental reverb effect is not available,
    // either because the feature is not present, excessive CPU load, or
    // the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
    result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
                                              &outputMixEnvironmentalReverb);
    if (SL_RESULT_SUCCESS == result) {
        result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
                outputMixEnvironmentalReverb, &reverbSettings);
        (void)result;
    }


    // configure audio source
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1,
                                   SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
                                   SL_SPEAKER_FRONT_LEFT|SL_SPEAKER_FRONT_RIGHT, SL_BYTEORDER_LITTLEENDIAN};
    /*
     * Enable Fast Audio when possible:  once we set the same rate to be the native, fast audio path
     * will be triggered
     */
    if(bqPlayerSampleRate) {
        format_pcm.samplesPerSec = bqPlayerSampleRate;       //sample rate in mili second
    }
    SLDataSource audioSrc = {&loc_bufq, &format_pcm};

    // configure audio sink
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};

    /*
     * create audio player:
     *     fast audio does not support when SL_IID_EFFECTSEND is required, skip it
     *     for fast audio case
     */
    const SLInterfaceID ids2[3] = {SL_IID_BUFFERQUEUE, SL_IID_VOLUME, SL_IID_EFFECTSEND,
            /*SL_IID_MUTESOLO,*/};
    const SLboolean req2[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
            /*SL_BOOLEAN_TRUE,*/ };

    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &audioSrc, &audioSnk,
                                                bqPlayerSampleRate? 2 : 3, ids2, req2);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // realize the player
    result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the play interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the buffer queue interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
                                             &bqPlayerBufferQueue);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // register callback on the buffer queue, 每次回调this会带给playCallback里面的Context
    result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallbackDN, this);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // get the effect send interface
    bqPlayerEffectSend = NULL;
    if( 0 == bqPlayerSampleRate) {
        result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_EFFECTSEND,
                                                 &bqPlayerEffectSend);
        assert(SL_RESULT_SUCCESS == result);
        (void)result;
    }

#if 0   // mute/solo is not supported for sources that are known to be mono, as this is
    // get the mute/solo interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_MUTESOLO, &bqPlayerMuteSolo);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;
#endif

    // get the volume interface
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // set the player's state to playing
    result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    bqPlayerCallbackDN(bqPlayerBufferQueue, this);
}

int DZAudio::resampleAudio(){
    int dataSize = 0;
    AVPacket *pPacket = av_packet_alloc();
    AVFrame *pFrame = av_frame_alloc();
    int index = 0;
    while(av_read_frame(pFormatContext, pPacket) >= 0){
        if(pPacket->stream_index == audioStreamIndex) {
            //Packet包，压缩的数据，解码成pcm数据
            int avcodecSendPacketRes = avcodec_send_packet(pCodecContext, pPacket);
            if (avcodecSendPacketRes == 0) {
                int avcodecReceiveFrameRes = avcodec_receive_frame(pCodecContext, pFrame);
                if (avcodecReceiveFrameRes == 0) {
                    //已经把AVPacket解码成AVFrame
                    index++;
                    LOGE("解码第%d帧", index);
                    //调用重采样的方法：
                    dataSize = swr_convert(swrContext, &resampleOutBuffer, pFrame->nb_samples, (const uint8_t **)(pFrame->data), pFrame->nb_samples);
                    LOGE("解码第%d帧", index);
//                    //在native层创建C数组
//                    memcpy(jPcmData, resampleOutBuffer, avSamplesBufferSize);
//                    //传0同步到java jbyteArray，并释放native jbyte*数组， 参考数组的细节处理章节.
//                    //  JNI_COMMIT仅仅同步，不释放native数组
//                    jniEnv->ReleaseByteArrayElements(jPcmByteArray, jPcmData, JNI_COMMIT);
//                    pJniCall->callAudioTrackWrite(threadMode, jPcmByteArray, 0, avSamplesBufferSize);
                    break;
                }
            }
        }

        //解引用
        av_packet_unref(pPacket);
        av_frame_unref(pFrame);
    }

//    pFrame->data

    //1. 解引用数据data 2. 销毁pPacket结构体内存 3. pPacket = NULL
    av_packet_free(&pPacket);
    av_frame_free(&pFrame);
    return dataSize;
}

// this callback handler is called every time a buffer finishes playing
void DZAudio::bqPlayerCallbackDN(SLAndroidSimpleBufferQueueItf bq, void *context)
{
    LOGE("bqPlayerCallbackDN========");
    DZAudio *pAudio = (DZAudio *)(context);
    int dataSize = pAudio->resampleAudio();
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

void DZAudio::play() {
    pthread_t playThreadT;
    pthread_create(&playThreadT, NULL, audioThreadPlay, this);
    pthread_detach(playThreadT);
}


