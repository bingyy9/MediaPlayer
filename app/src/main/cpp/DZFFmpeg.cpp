//
// Created by bingy on 2021/3/14.
//

#include <assert.h>
#include "DZFFmpeg.h"


DZFFmpeg::DZFFmpeg(DZJNICall *dzjniCall, const char *url) {
    this->pJniCall = dzjniCall;
    //复制一份url， 怕主线程销毁了url
    this->url = (char *)malloc(strlen(url) + 1);
    memcpy(this->url, url, strlen(url) + 1);
    pPlayerStatus = new DZPlayerStatus();
}

DZFFmpeg::~DZFFmpeg() {
    release();
}

void *threadPlay(void* context){
    DZFFmpeg *pFFmpeg = (DZFFmpeg *)context;
    pFFmpeg->prepare(THREAD_CHILD);
    return 0;
}

void* threadReadPacket(void * context){
    LOGE("threadReadPacket====");
    DZFFmpeg* pFFmpeg = (DZFFmpeg *)context;

    while(pFFmpeg->pPlayerStatus != NULL && !pFFmpeg->pPlayerStatus->isExist){
        AVPacket *pPacket = av_packet_alloc();
        if(av_read_frame(pFFmpeg->pFormatContext, pPacket) >= 0){
            if(pPacket->stream_index == pFFmpeg->pAudio->streamIndex) {
                pFFmpeg->pAudio->pPacketQueue->push(pPacket);
            } else if(pPacket->stream_index == pFFmpeg->pVideo->streamIndex) {
                pFFmpeg->pVideo->pPacketQueue->push(pPacket);
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

void DZFFmpeg::play() {

    //一个线程读取packet
    pthread_t readPacketThreadT;
    pthread_create(&readPacketThreadT, NULL, threadReadPacket, this);
    pthread_detach(readPacketThreadT);

    if(pAudio != NULL){
        pAudio->play();
    }
    if(pVideo != NULL){
        pVideo->play();
    }

//    prepare(THREAD_MAIN);
//    prepare4SLES(THREAD_MAIN);

//    pthread_t playThreadT;
//    pthread_create(&playThreadT, NULL, threadPlay, this);
////    pthread_detach(playThreadT);
//    pthread_join(playThreadT, NULL);
//    prepare();

}

void DZFFmpeg::onJniPlayError(ThreadMode threadMode, int code, char *msg) {
    //释放资源
    release();
    pJniCall->onPlayError(threadMode, code, msg);

}

void DZFFmpeg::release() {
    if(pAudio != NULL){
        free(pAudio);
        pAudio = NULL;
    }

    if(pVideo != NULL){
        free(pVideo);
        pVideo = NULL;
    }

    if(pFormatContext != NULL){
        avformat_close_input(&pFormatContext); //释放流资源
        avformat_free_context(pFormatContext); //释放结构体内存
        pFormatContext = NULL;
    }

    avformat_network_deinit();
    if(this->url != NULL){
        free (this->url);
        this->url = NULL;
    }

    if(pPlayerStatus != NULL){
        delete pPlayerStatus;
        pPlayerStatus = NULL;
    }
}

void DZFFmpeg::prepare() {
    LOGE("DZFFmpeg:: prepare");
    prepare(THREAD_MAIN);
}

void *threadPrepare(void* context){
    DZFFmpeg *pFFmpeg = (DZFFmpeg *)context;
    pFFmpeg->prepareAsync(THREAD_CHILD);
    return 0;
}

void DZFFmpeg::prepareAsync() {
    pthread_t prepareThreadT;
    pthread_create(&prepareThreadT, NULL, threadPrepare,this);
    pthread_detach(prepareThreadT);
}

void DZFFmpeg::prepare(ThreadMode threadMode) {
    av_register_all();
    avformat_network_init();
    AVFormatContext *pFormatContext = NULL;
    AVCodecParameters *pCodecParameters = NULL;
    AVCodec *pCodec = NULL;
    AVCodecContext* pCodecContext = NULL;

    LOGE("DZFFmpeg::prepare url = %s", url);
    int formatOpenInputRes = avformat_open_input(&pFormatContext, url, NULL, NULL);
    if(formatOpenInputRes != 0){
        //回调Java层
        //释放资源
        LOGE("format open input error: %s", av_err2str(formatOpenInputRes));
        onJniPlayError(threadMode, FIND_STREAM_ERROR_CODE, av_err2str(formatOpenInputRes));
        return;
    }

    int avformatFindStreamInfo = avformat_find_stream_info(pFormatContext, NULL);
    if(avformatFindStreamInfo < 0){
        LOGE("avformat_find_stream_info error: %s", av_err2str(avformatFindStreamInfo));
        onJniPlayError(threadMode, FIND_STREAM_INFO_ERROR_CODE, av_err2str(avformatFindStreamInfo));
        return;
    }

    //查找音频流的index
    int audioStreamIndex = av_find_best_stream(pFormatContext, AVMediaType::AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if(audioStreamIndex < 0){
        LOGE("av_find_best_stream find audio stream error: %s", av_err2str(audioStreamIndex));
        onJniPlayError(threadMode, FIND_BEST_STREAM_ERROR_CODE, av_err2str(audioStreamIndex));
        return;
    }



    pCodecParameters = pFormatContext->streams[audioStreamIndex]->codecpar;
    //查找解码
    pCodec = avcodec_find_decoder(pCodecParameters->codec_id);
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
    SwrContext *swrContext = swr_alloc_set_opts(NULL, out_ch_layout, out_sample_fmt, out_sample_rate
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
    int outChannels = av_get_channel_layout_nb_channels(out_ch_layout);
    int avSamplesBufferSize = av_samples_get_buffer_size(NULL, outChannels, pCodecParameters->frame_size, out_sample_fmt, 0);

    uint8_t *resampleOutBuffer = (uint8_t *)malloc(avSamplesBufferSize);
    //-------------重采样end

    JNIEnv *jniEnv = NULL;
    if(threadMode == THREAD_MAIN){
        jniEnv = pJniCall->jniEnv;
    } else if(threadMode == THREAD_CHILD){
        //获取当前线程的JNIEnv，通过JavaVM
        if(pJniCall->javaVM->AttachCurrentThread(&jniEnv, 0) != JNI_OK){
            LOGE("get child thread jniEnv error!!");
            return;
        }
    }

    jbyteArray jPcmByteArray = jniEnv->NewByteArray(avSamplesBufferSize);
    jbyte *jPcmData = jniEnv->GetByteArrayElements(jPcmByteArray, NULL);

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
                    swr_convert(swrContext, &resampleOutBuffer, pFrame->nb_samples, (const uint8_t **)(pFrame->data), pFrame->nb_samples);

                    //在native层创建C数组
                    memcpy(jPcmData, resampleOutBuffer, avSamplesBufferSize);
                    //传0同步到java jbyteArray，并释放native jbyte*数组， 参考数组的细节处理章节.
                    //  JNI_COMMIT仅仅同步，不释放native数组
                    jniEnv->ReleaseByteArrayElements(jPcmByteArray, jPcmData, JNI_COMMIT);
                    pJniCall->callAudioTrackWrite(threadMode, jPcmByteArray, 0, avSamplesBufferSize);

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
    //这行代码肯定要加，但是内存还是会网上涨
    jniEnv->ReleaseByteArrayElements(jPcmByteArray, jPcmData, 0);
    jniEnv->DeleteLocalRef(jPcmByteArray);

    if(threadMode == THREAD_CHILD){
        pJniCall->javaVM->DetachCurrentThread();
    }
}

void DZFFmpeg::prepareAsync(ThreadMode threadMode) {
    av_register_all();
    avformat_network_init();

    LOGE("DZFFmpeg::prepare url = %s", url);
    int formatOpenInputRes = avformat_open_input(&pFormatContext, url, NULL, NULL);
    if(formatOpenInputRes != 0){
        //回调Java层
        //释放资源
        LOGE("format open input error: %s", av_err2str(formatOpenInputRes));
        onJniPlayError(threadMode, FIND_STREAM_ERROR_CODE, av_err2str(formatOpenInputRes));
        return;
    }

    int avformatFindStreamInfo = avformat_find_stream_info(pFormatContext, NULL);
    if(avformatFindStreamInfo < 0){
        LOGE("avformat_find_stream_info error: %s", av_err2str(avformatFindStreamInfo));
        onJniPlayError(threadMode, FIND_STREAM_INFO_ERROR_CODE, av_err2str(avformatFindStreamInfo));
        return;
    }

    //查找音频流的index
    int audioStreamIndex = av_find_best_stream(pFormatContext, AVMediaType::AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if(audioStreamIndex < 0){
        LOGE("av_find_best_stream find audio stream error: %s", av_err2str(audioStreamIndex));
        onJniPlayError(threadMode, FIND_BEST_STREAM_ERROR_CODE, av_err2str(audioStreamIndex));
        return;
    }

    //查找音频流的index
    int videoStreamIndex = av_find_best_stream(pFormatContext, AVMediaType::AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if(videoStreamIndex < 0){
        LOGE("av_find_best_stream find video stream error: %s", av_err2str(audioStreamIndex));
//        onJniPlayError(threadMode, FIND_BEST_STREAM_ERROR_CODE, av_err2str(audioStreamIndex));
//        return;
    }

    pAudio = new DZAudio(audioStreamIndex, pJniCall, pPlayerStatus);
    pAudio->analysisStream(threadMode, pFormatContext);

    if(videoStreamIndex >= 0){
        pVideo = new DZVideo(videoStreamIndex, pJniCall, pPlayerStatus);
        pVideo->analysisStream(threadMode, pFormatContext);
    }

    pJniCall->onPrepared(threadMode);
}

void DZFFmpeg::setSurface(jobject object) {
    if(pVideo != NULL){
        pVideo->setSurface(object);
    }
}




