//
// Created by bingy on 2021/3/17.
//

#include "DZAudio.h"

DZAudio::DZAudio(int audioStreamIndex, DZJNICall *jniCall,  AVFormatContext *formatContext) {
    this->audioStreamIndex = audioStreamIndex;
    this->pJniCall = jniCall;
//    this->pCodecContext = codecContext;
    this->pFormatContext = formatContext;
//    this->swrContext = swrContext;
    pPacketQueue = new DZAVPacketQueue();
    pPlayerStatus = new DZPlayerStatus();
}

DZAudio::~DZAudio() {
    audio_release();

}

void* audioThreadPlay(void * context){
    LOGE("audioThreadPlay====");
    DZAudio* audio = (DZAudio *)context;
    LOGE("audioThreadPlay %p " , audio);
    audio->initCreateOpenSELS();
    LOGE("audioThreadPlay222 %p " , audio);
}



void* threadReadPacket(void * context){
    LOGE("threadReadPacket====");
    DZAudio* audio = (DZAudio *)context;

    while(audio->pPlayerStatus != NULL && !audio->pPlayerStatus->isExist){
        AVPacket *pPacket = av_packet_alloc();
        if(av_read_frame(audio->pFormatContext, pPacket) >= 0){
            if(pPacket->stream_index == audio->audioStreamIndex) {
                audio->pPacketQueue->push(pPacket);
            } else {
                //解引用
                av_packet_free(&pPacket);
            }
        } else {
            av_packet_free(&pPacket);
            //睡眠，尽量不去消耗CPU资源，也可以退出销毁线程
        }
    }
}

void DZAudio::initCreateOpenSELS(){
    LOGE("initCreateOpenSELS");
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

int DZAudio::resampleAudio2(){
    int dataSize = 0;
    AVPacket *pPacket = NULL;
    AVFrame *pFrame = av_frame_alloc();
    while(this->pPlayerStatus != NULL && !this->pPlayerStatus->isExist){
        pPacket = this->pPacketQueue->pop();
        if(pPacket == NULL){
            //sleep
            continue;
        }
        //Packet包，压缩的数据，解码成pcm数据
        int avcodecSendPacketRes = avcodec_send_packet(pCodecContext, pPacket);
        if (avcodecSendPacketRes == 0) {
            int avcodecReceiveFrameRes = avcodec_receive_frame(pCodecContext, pFrame);
            if (avcodecReceiveFrameRes == 0) {
                //已经把AVPacket解码成AVFrame
                LOGE("解码帧");
                //调用重采样的方法：dataSize 返回的是重采样的个数，也就是pFrame->nb_samples
                dataSize = swr_convert(swrContext, &resampleOutBuffer, pFrame->nb_samples, (const uint8_t **)(pFrame->data), pFrame->nb_samples);
                dataSize = dataSize * 2 * 2;
                LOGE("解码帧，dataSize = %d, nb_samples = %d, frame_size = %d", dataSize, pFrame->nb_samples, pCodecContext->frame_size);
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
                    //调用重采样的方法：dataSize 返回的是重采样的个数，也就是pFrame->nb_samples
                    dataSize = swr_convert(swrContext, &resampleOutBuffer, pFrame->nb_samples, (const uint8_t **)(pFrame->data), pFrame->nb_samples);
                    dataSize = dataSize * 2 * 2;
                    LOGE("解码第%d帧，dataSize = %d, nb_samples = %d, frame_size = %d", index, dataSize, pFrame->nb_samples, pCodecContext->frame_size);
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
    int dataSize = 0;
    if(pAudio->async){
        dataSize = pAudio->resampleAudio2();
    } else {
        dataSize = pAudio->resampleAudio();
    }
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
    if(async){
        //一个线程解码播放
        pthread_t playThreadT;
        pthread_create(&playThreadT, NULL, audioThreadPlay, this);
        pthread_detach(playThreadT);


        //一个线程读取packet
        pthread_t readPacketThreadT;
        pthread_create(&readPacketThreadT, NULL, threadReadPacket, this);
        pthread_detach(readPacketThreadT);
    } else {
        initCreateOpenSELS();
    }
}

void DZAudio::analysisStream(ThreadMode threadMode, AVStream **streams) {

    AVCodecParameters *pCodecParameters = streams[audioStreamIndex]->codecpar;
    //查找解码
    AVCodec *pCodec = avcodec_find_decoder(pCodecParameters->codec_id);
    if(pCodec == NULL){
        LOGE("avCodec is null");
        onJniPlayError(threadMode, CODED_FIND_DECODER_ERROR_CODE, "avCodec is null");
        return;
    }


    //打开解码器
    pCodecContext = avcodec_alloc_context3(pCodec);
    if(pCodecContext == NULL){
        LOGE("pCodecContext is null");
        onJniPlayError(threadMode, ALLOCATE_CONTEXT_ERROR_CODE, "pCodecContext is null");
        return;
    }

    int avcodecParametersToContextRes = avcodec_parameters_to_context(pCodecContext, pCodecParameters);
    if(avcodecParametersToContextRes  < 0){
        LOGE("codec parameters to context error : %s ", av_err2str(avcodecParametersToContextRes));
        onJniPlayError(threadMode, AVCODEC_PARAM_TO_CONTEXT_ERROR_CODE, av_err2str(avcodecParametersToContextRes));
        return;
    }
    int avcodecOpenRes = avcodec_open2(pCodecContext, pCodec, NULL);
    if(avcodecOpenRes != 0){
        LOGE("codec audio open fail : %s ", av_err2str(avcodecOpenRes));
        onJniPlayError(threadMode, AVCODEC_OPEN_2_ERROR_CODE, av_err2str(avcodecOpenRes));
        return;
    }
//    avcodec_open2(pFormatContext->streams[audioStreamIndex]->codec, pCodec, NULL);

    LOGE("%d, %d", pCodecParameters->sample_rate, pCodecParameters->channels);

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
//    resampleOutBuffer = (uint8_t *)(malloc(pCodecContext->frame_size * 2 * 2));
//    uint8_t *resampleOutBuffer = (uint8_t *)malloc(avSamplesBufferSize);
}

void DZAudio::onJniPlayError(ThreadMode threadMode, int code, char *msg) {
    //释放资源
    audio_release();
    pJniCall->onPlayError(threadMode, code, msg);
}

void DZAudio::audio_release() {
    if(pCodecContext != NULL){
        avcodec_close(pCodecContext);
        avcodec_free_context(&pCodecContext);
        pCodecContext = NULL;
    }

    if(swrContext != NULL){
        swr_free(&swrContext);
        free(swrContext);
        swrContext = NULL;
    }

    if(resampleOutBuffer != NULL){
        free(resampleOutBuffer);
        resampleOutBuffer = NULL;
    }
    if(pPacketQueue != NULL){
        delete pPacketQueue;
        pPacketQueue = NULL;
    }

    if(pPlayerStatus != NULL){
        delete pPlayerStatus;
        pPlayerStatus = NULL;
    }
}


