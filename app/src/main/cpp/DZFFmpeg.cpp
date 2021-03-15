//
// Created by bingy on 2021/3/14.
//

#include "DZFFmpeg.h"



DZFFmpeg::DZFFmpeg(DZJNICall *dzjniCall, const char *url) {
    this->pJniCall = dzjniCall;
    this->url = url;
}

DZFFmpeg::~DZFFmpeg() {
    release();
}

void DZFFmpeg::play() {
    LOGE("DZFFmpeg::play start");
    av_register_all();
    avformat_network_init();
    AVFormatContext *pFormatContext = NULL;
    int formatOpenInputRes = 0;
    int avformatFindStreamInfo = 0;
    int audioStreamIndex = -1;
    AVCodecParameters *pCodecParameters = NULL;
    AVCodec *pCodec = NULL;
    AVCodecContext* pCodecContext = NULL;
    int avcodecParametersToContextRes = -1;
    int avcodecOpenRes = -1;
    int index = 0;
    AVPacket *pPacket = NULL;
    AVFrame *pFrame = NULL;
    int avSamplesBufferSize = 0;
    jbyteArray jPcmByteArray = NULL;
    jbyte* jPcmData = NULL;

    formatOpenInputRes = avformat_open_input(&pFormatContext, url, NULL, NULL);
    if(formatOpenInputRes != 0){
        //回调Java层
        //释放资源
        LOGE("format open input error: %s", av_err2str(formatOpenInputRes));
        onJniPlayError(FIND_STREAM_ERROR_CODE, av_err2str(formatOpenInputRes));
        return;
    }

    avformatFindStreamInfo = avformat_find_stream_info(pFormatContext, NULL);
    if(avformatFindStreamInfo < 0){
        LOGE("avformat_find_stream_info error: %s", av_err2str(avformatFindStreamInfo));
        onJniPlayError(FIND_STREAM_INFO_ERROR_CODE, av_err2str(avformatFindStreamInfo));
        return;
    }

    //查找音频流的index
    audioStreamIndex = av_find_best_stream(pFormatContext, AVMediaType::AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if(audioStreamIndex < 0){
        LOGE("av_find_best_stream find audio stream error: %s", av_err2str(audioStreamIndex));
        onJniPlayError(FIND_BEST_STREAM_ERROR_CODE, av_err2str(audioStreamIndex));
        return;
    }

    pCodecParameters = pFormatContext->streams[audioStreamIndex]->codecpar;
    //查找解码
    pCodec = avcodec_find_decoder(pCodecParameters->codec_id);
    if(pCodec == NULL){
        LOGE("avCodec is null");
        onJniPlayError(CODED_FIND_DECODER_ERROR_CODE, "avCodec is null");
        return;
    }


    //打开解码器
    pCodecContext = avcodec_alloc_context3(pCodec);
    if(pCodecContext == NULL){
        LOGE("pCodecContext is null");
        onJniPlayError(ALLOCATE_CONTEXT_ERROR_CODE, "pCodecContext is null");
        return;
    }

    avcodecParametersToContextRes = avcodec_parameters_to_context(pCodecContext, pCodecParameters);
    if(avcodecParametersToContextRes  < 0){
        LOGE("codec parameters to context error : %s ", av_err2str(avcodecParametersToContextRes));
        onJniPlayError(AVCODEC_PARAM_TO_CONTEXT_ERROR_CODE, av_err2str(avcodecParametersToContextRes));
        return;
    }
    avcodecOpenRes = avcodec_open2(pCodecContext, pCodec, NULL);
    if(avcodecOpenRes != 0){
        LOGE("codec audio open fail : %s ", av_err2str(avcodecOpenRes));
        onJniPlayError(AVCODEC_OPEN_2_ERROR_CODE, av_err2str(avcodecOpenRes));
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
        onJniPlayError(AVCODEC_OPEN_2_ERROR_CODE, "swrContext is null");
        return;
    }
    int swrInitRes = swr_init(swrContext);
    if(swrInitRes < 0){
        onJniPlayError(AVCODEC_OPEN_2_ERROR_CODE, "swrInitRes < 0 is null");
        return;
    }
    //write pcm到缓冲区。pFrame->data -> javabyte
    //1s 44100采样点， 2通道， 16字节   1秒的大小=44100*2*2
    //1帧不是1秒， 1秒都多少帧pFrame->nb_samples
    //size是播放指定的大小，是最终输出的大小
    int outChannels = av_get_channel_layout_nb_channels(out_ch_layout);
    avSamplesBufferSize = av_samples_get_buffer_size(NULL, outChannels, pCodecParameters->frame_size, out_sample_fmt, 0);

    uint8_t *resampleOutBuffer = (uint8_t *)malloc(avSamplesBufferSize);
    //-------------重采样end

    jPcmByteArray = pJniCall->jniEnv->NewByteArray(avSamplesBufferSize);
    jPcmData = pJniCall->jniEnv->GetByteArrayElements(jPcmByteArray, NULL);

    pPacket = av_packet_alloc();
    pFrame = av_frame_alloc();
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
                    pJniCall->jniEnv->ReleaseByteArrayElements(jPcmByteArray, jPcmData, JNI_COMMIT);
                    pJniCall->callAudioTrackWrite(jPcmByteArray, 0, avSamplesBufferSize);

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
    pJniCall->jniEnv->ReleaseByteArrayElements(jPcmByteArray, jPcmData, 0);
    pJniCall->jniEnv->DeleteLocalRef(jPcmByteArray);
}

void DZFFmpeg::onJniPlayError(int code, char *msg) {
    //释放资源
    release();
    pJniCall->onPlayError(code, msg);

}

void DZFFmpeg::release() {
    if(pCodecContext != NULL){
        avcodec_close(pCodecContext);
        avcodec_free_context(&pCodecContext);
        pCodecContext = NULL;
    }

    if(pFormatContext != NULL){
        avformat_close_input(&pFormatContext); //释放流资源
        avformat_free_context(pFormatContext); //释放结构体内存
        pFormatContext = NULL;
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
    avformat_network_deinit();
}
