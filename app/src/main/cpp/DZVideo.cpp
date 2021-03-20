//
// Created by bingy on 2021/3/18.
//


#include "DZVideo.h"

DZVideo::DZVideo(int audioStreamIndex, DZJNICall *jniCall, DZPlayerStatus *playerStatus, DZAudio* pAudio)
    : DZMedia(audioStreamIndex, jniCall, playerStatus){
    this->pAudio = pAudio;
}


DZVideo::~DZVideo() {
    release();
}

void* videoThreadPlay(void * context){
    LOGE("audioThreadPlay====");
    DZVideo* video = (DZVideo *)context;
    LOGE("videoThreadPlay start %p " , video);
    video->resampleVideo(context);
    LOGE("videoThreadPlay end %p " , video);
}

void DZVideo::play() {
    if(async){
        //一个线程解码播放
        pthread_t playThreadT;
        pthread_create(&playThreadT, NULL, videoThreadPlay, this);
        pthread_detach(playThreadT);
    } else {
        resampleVideo(this);
    }
}

void DZVideo::privateAnalysisStream(ThreadMode threadMode, AVFormatContext *pFormatContext) {
    //初始化转换上下文
    pSwsContext = sws_getContext(pCodecContext->width, pCodecContext->height, pCodecContext->pix_fmt
            , pCodecContext->width, pCodecContext->height, AV_PIX_FMT_RGBA
            , SWS_BILINEAR, NULL, NULL, NULL);
    pRGBAFrame = av_frame_alloc();  //存储解压后转换后的数据
    mFrameSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA, pCodecContext->width, pCodecContext->height, 1);
    pFrameBuffer = (uint8_t *) malloc(mFrameSize * sizeof(uint8_t));
    av_image_fill_arrays(pRGBAFrame->data, pRGBAFrame->linesize, pFrameBuffer, AV_PIX_FMT_RGBA, pCodecContext->width, pCodecContext->height, 1);

    int num = pFormatContext->streams[streamIndex]->avg_frame_rate.num;
    int den = pFormatContext->streams[streamIndex]->avg_frame_rate.den;
    if(den != 0 && num != 0){
//        //25/1
//        int fps = num/den;
//        defaultDelayTime = 1.0f/fps;
        defaultDelayTime = 1.0f * den /num;
        LOGE("DZVideo num = %d, den = %d, defaultDelayTime = %lf", num, den, defaultDelayTime);
    }
}

void DZVideo::release() {
    DZMedia::release();
    if (pSwsContext != NULL) {
        sws_freeContext(pSwsContext);
        free(pSwsContext);
        pSwsContext = NULL;
    }

    if (pFrameBuffer != NULL) {
        free(pFrameBuffer);
        pFrameBuffer = NULL;
    }

    if (pRGBAFrame != NULL) {
        av_frame_free(&pRGBAFrame);
//        free(pRGBAFrame);
        pRGBAFrame = NULL;
    }

    //注意pJniCall需要在free（pVideo)之后销毁
    if (pJniCall != NULL) {
        pJniCall->jniEnv->DeleteGlobalRef(pSurface);
    }
}

void DZVideo::resampleVideo(void *context) {
    LOGE("DZVideo resampleVideo!");
    DZVideo *pVideo = (DZVideo *)context;

    if(pVideo->pSurface == NULL){
        LOGE("DZVideo resampleVideo pSurface is null!");
        return;
    }

    //获取当前线程的JNIEnv，通过JavaVM
    JNIEnv *env = NULL;
    if(pVideo->pJniCall->javaVM->AttachCurrentThread(&env, 0) != JNI_OK){
        LOGE("DZVideo get child thread jniEnv error!!");
        return;
    }

    //************注意pVideo->pJniCall->jniEnv这个是主线程的
    //获取窗体
    ANativeWindow* pNativeWindow = ANativeWindow_fromSurface(env, pVideo->pSurface);

    //设置缓冲区属性
    ANativeWindow_setBuffersGeometry(pNativeWindow,
            pVideo->pCodecContext->width,
            pVideo->pCodecContext->height,
            WINDOW_FORMAT_RGBA_8888);
    //缓冲区Window的buffer
    ANativeWindow_Buffer outBuffer;

    LOGE("DZVideo resampleVideo!33333");

    int dataSize = 0;
    AVPacket *pPacket = NULL;
    AVFrame *pFrame = av_frame_alloc();
    while(pVideo->pPlayerStatus != NULL && !pVideo->pPlayerStatus->isExist){
        pPacket = pVideo->pPacketQueue->pop();
        if(pPacket == NULL){
            //sleep
            continue;
        }
        //Packet包，压缩的数据，解码成pcm数据
        int avcodecSendPacketRes = avcodec_send_packet(pVideo->pCodecContext, pPacket);
        if (avcodecSendPacketRes == 0) {
            int avcodecReceiveFrameRes = avcodec_receive_frame(pVideo->pCodecContext, pFrame);
            if (avcodecReceiveFrameRes == 0) {
                //已经把AVPacket解码成AVFrame
                LOGE("DZVideo 解码帧");
                //调用重采样的方法：dataSize 返回的是重采样的个数，也就是pFrame->nb_samples
                //pFrame->data一般都是YUV420P的。SurfaceView需要显示RGBA8888，因此需要转换。
                sws_scale(pVideo->pSwsContext, pFrame->data, pFrame->linesize, 0, pVideo->pCodecContext->height
                        , pVideo->pRGBAFrame->data, pVideo->pRGBAFrame->linesize);


                //在播放之前判断一下需要休眠多久
                double frameSleepTime = pVideo->getFrameSleepTime(pFrame); //返回的单位是秒
                LOGE("DZVideo frameSleepTime %lf", frameSleepTime);
                av_usleep(frameSleepTime * 1000 * 1000); //单位是微妙

                //拿到转换后的RGBA8888data后如何渲染？ 放入缓冲区
                ANativeWindow_lock(pNativeWindow, &outBuffer, NULL);
                memcpy(outBuffer.bits, pVideo->pFrameBuffer, mFrameSize);
                ANativeWindow_unlockAndPost(pNativeWindow);
            }
        }
        av_packet_unref(pPacket);
        av_frame_unref(pFrame);
    }

    av_packet_free(&pPacket);
    av_frame_free(&pFrame);
    pVideo->pJniCall->javaVM->DetachCurrentThread();

}

void DZVideo::setSurface(jobject object) {
    LOGE("setSurface");
//    //获取当前线程的JNIEnv，通过JavaVM
//    JNIEnv *env = NULL;
//    if(pJniCall->javaVM->AttachCurrentThread(&env, 0) != JNI_OK){
//        LOGE("DZVideo get child thread jniEnv error!!");
//        return;
//    }
//    LOGE("setSurface1111");
//    this->pSurface = env->NewGlobalRef(object);
//    //************注意pVideo->pJniCall->jniEnv这个是主线程的
//    //获取窗体
//    LOGE("setSurface22222");
//   pJniCall->javaVM->DetachCurrentThread();
//    LOGE("setSurface33333");

    if(pJniCall != NULL){
        LOGE("setSurface33333");
        this->pSurface = pJniCall->jniEnv->NewGlobalRef(object);
        LOGE("setSurface444");
        LOGE("DZVideop pSurface = %p ", this->pSurface);
    }
}

double DZVideo::getFrameSleepTime(AVFrame *pFrame) {
    double times = pFrame->pts * av_q2d(timeBase); //转换成秒
    LOGE("DZVideo getFrameSleepTime times %lf", times);
    if(times > currentTime){
        currentTime = times;
    }

    //视频快了就慢一点，视频慢了就快一点
    //但是尽量的把时间控制在视频的帧率时间范围左右 1秒24帧，1/24 = 0.04  1/30 = 0.033

    //相差多少秒
    double diffTime = pAudio->currentTime - currentTime;


    if(diffTime > 0.016 || diffTime < -0.016){
        //第一次控制0.016s -- -0.016s
        if(diffTime > 0.016){
            delayTime = delayTime *2 / 3;
        } else if(diffTime < -0.016){
            delayTime = delayTime * 3/2;
        }

        //第二次控制
        if(delayTime < defaultDelayTime/2){
            delayTime = defaultDelayTime * 2/3;
        } else if(delayTime > defaultDelayTime * 2){
            delayTime = defaultDelayTime * 3/2;
        }
    }

    //第三次控制，异常情况，防止越界
    if(diffTime >= 0.25){
        delayTime = 0;
    } else if(diffTime <= -0.25){
        delayTime = defaultDelayTime * 2;
    }

    return delayTime;
}
