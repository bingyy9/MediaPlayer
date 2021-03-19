#include <jni.h>
#include <string>

//本地窗口 , 在 Native 层处理图像绘制
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <malloc.h>
//#include <SELS/OpenSELS.h>
//#include <SELS/OpenSLES_Android.h>

//在C++中采用C的方式编译，因为ffmpeg都是C语言写的
extern "C" {
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}
#include "DZConstDefine.h"
#include "DZJNICall.h"
#include "DZFFmpeg.h"
#include "DZAudio.h"
#include "DZMedia.h"



#include "FFMPEG.h"

//声明 FFMPEG 类
FFMPEG *ffmpeg = 0;

/**
 * 原生绘制窗口
 */
ANativeWindow * aNativeWindow;

/**
 * 同步锁 , 静态方式初始化 , 同一个进程 , 只能出现一个 , 只要该应用不销毁 , 就一直存在
 *      该锁可以不同进行释放 , 直接使用即可
 */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Java 类回调
 */
JavaPlayerCaller * javaCallHelper;

//JNI_OnLoad 中获取的 Java 虚拟机对象放在这里
JavaVM *pJavaVM;
//int JNI_OnLoad(JavaVM *vm, void *r){
//
//    javaVM = vm;
//
//    return JNI_VERSION_1_6;
//}

/**
 * 解码后的图片绘制方法
 *
 *      注意互斥问题 : 确保该方法与 Java_kim_hsl_ffmpeg_Player_native_1prepare 方法互斥 , 要加上线程锁
 *
 * @param data
 *          RGBA 像素数据
 * @param linesize
 *
 * @param width
 *          图像的宽
 * @param height
 *          图像的高
 */
void show(uint8_t *data, int linesize, int width, int height){

    //加同步锁
    pthread_mutex_lock(&mutex);

    //首先确保 ANativeWindow * aNativeWindow 绘制载体存在 , 否则直接退出
    if(!aNativeWindow){
        //解除同步锁 , 否则一直阻塞在此处
        pthread_mutex_unlock(&mutex);
        return;
    }

    //设置 ANativeWindow 绘制窗口属性
    //  传入的参数分别是 : ANativeWindow 结构体指针 , 图像的宽度 , 图像的高度 , 像素的内存格式
    ANativeWindow_setBuffersGeometry(aNativeWindow, width, height, WINDOW_FORMAT_RGBA_8888);

    //获取 ANativeWindow_Buffer , 如果获取失败 , 直接释放相关资源退出
    ANativeWindow_Buffer aNativeWindow_Buffer;

    //如果获取成功 , 可以继续向后执行 , 获取失败 , 直接退出
    if(ANativeWindow_lock(aNativeWindow, &aNativeWindow_Buffer, 0)){
        //退出操作 , 释放 aNativeWindow 结构体指针
        ANativeWindow_release(aNativeWindow);
        aNativeWindow = 0;
        return;
    }

    //向 ANativeWindow_Buffer 填充 RGBA 像素格式的图像数据
    uint8_t *dst_data = static_cast<uint8_t *>(aNativeWindow_Buffer.bits);

    //参数中的 uint8_t *data 数据中 , 每一行有 linesize 个 , 拷贝的目标也要逐行拷贝
    //  aNativeWindow_Buffer.stride 是每行的数据个数 , 每个数据都包含一套 RGBA 像素数据 ,
    //      RGBA 数据每个占1字节 , 一个 RGBA 占 4 字节
    //  每行的数据个数 * 4 代表 RGBA 数据个数
    int dst_linesize = aNativeWindow_Buffer.stride * 4;

    //获取 ANativeWindow_Buffer 中数据的地址
    //      一次拷贝一行 , 有 像素高度 行数
    for(int i = 0; i < aNativeWindow_Buffer.height; i++){

        //计算拷贝的指针地址
        //  每次拷贝的目的地址 : dst_data + ( i * dst_linesize )
        //  每次拷贝的源地址 : data + ( i * linesize )

        memcpy(dst_data + ( i * dst_linesize ), data + ( i * linesize ), dst_linesize);

    }

    //启动绘制
    ANativeWindow_unlockAndPost(aNativeWindow);

    //解除同步锁
    pthread_mutex_unlock(&mutex);

}


extern "C"
JNIEXPORT void JNICALL
Java_kim_hsl_ffmpeg_Player_native_1prepare(JNIEnv *env, jobject instance, jstring dataSource_) {

    //Java 中传入的视频直播流地址 , "rtmp://live.hkstv.hk.lxdns.com/live/hks"
    const char *dataSource = env->GetStringUTFChars(dataSource_, 0);

    //创建 Java 调用类
    javaCallHelper = new JavaPlayerCaller(pJavaVM, env, instance);

    //在 FFMPEG.cpp 中声明的构造函数
    ffmpeg = new FFMPEG(javaCallHelper, dataSource);

    //设置绘制图像回调函数
    ffmpeg->setShowFrameCallback(show);

    //播放器准备 , 调用 ffmpeg 指针指向的类对象的 prepare() 方法
    ffmpeg->prepare();

    //释放字符串
    env->ReleaseStringUTFChars(dataSource_, dataSource);
}


extern "C"
JNIEXPORT void JNICALL
Java_kim_hsl_ffmpeg_Player_native_1start(JNIEnv *env, jobject instance) {

    //调用本地 ffmpeg 播放器的 start() 方法
    if(ffmpeg) {
        ffmpeg->start();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_kim_hsl_ffmpeg_Player_native_1set_1surface(JNIEnv *env, jobject instance, jobject surface) {

    //加同步锁
    pthread_mutex_lock(&mutex);

    // 将从 Java 层传递的 Surface 对象转换成 ANativeWindow 结构体
    //      如果之前已经有了 ANativeWindow 结构体 , 那么先将原来的释放掉

    //释放原来的 ANativeWindow
    if(aNativeWindow){
        ANativeWindow_release(aNativeWindow);
    }

    //转换新的 ANativeWindow
    aNativeWindow = ANativeWindow_fromSurface(env, surface);

    //解除同步锁
    pthread_mutex_unlock(&mutex);

}

extern "C"
JNIEXPORT void JNICALL
Java_kim_hsl_ffmpeg_Player_native_1stop(JNIEnv *env, jobject thiz) {

    // 停止播放 , 在该方法中药释放相关变量 , 处理内存收尾操作
    if(ffmpeg){
        ffmpeg->stop();
    }

    if(javaCallHelper){
        delete javaCallHelper;
        javaCallHelper = 0;
    }

}


extern "C"
JNIEXPORT void JNICALL
Java_kim_hsl_ffmpeg_Player_native_1release(JNIEnv *env, jobject thiz) {

    //释放 aNativeWindow

    //加同步锁
    pthread_mutex_lock(&mutex);

    //释放原来的 ANativeWindow
    if(aNativeWindow){
        ANativeWindow_release(aNativeWindow);
        aNativeWindow = 0;
    }
    //解除同步锁
    pthread_mutex_unlock(&mutex);

}extern "C"
JNIEXPORT jint JNICALL
Java_kim_hsl_ffmpeg_Player_native_1getDuration(JNIEnv *env, jobject thiz) {
    if (ffmpeg) {
        return ffmpeg->getDuration();
    }
    return 0;
}extern "C"
JNIEXPORT void JNICALL
Java_kim_hsl_ffmpeg_Player_native_1seek(JNIEnv *env, jobject thiz, jint progress) {
    if (ffmpeg){
        ffmpeg->seek(progress);
    }
}

DZJNICall *pDZJNICall = NULL;
DZFFmpeg *pDZFFmpeg = NULL;


//重写so被加载时调用的一个方法
//去了解动态注册
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *javaVm, void *reserved){
    LOGE("JNI_OnLoad=====");
    pJavaVM = javaVm;
    JNIEnv *env;
    if(pJavaVM->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_4) != JNI_OK){
        return -1;
    }
    return JNI_VERSION_1_4;
}


extern "C" JNIEXPORT void JNICALL
Java_kim_hsl_ffmpeg_DarrenPlayer_play0(JNIEnv *env, jobject thiz, jstring url_) {
    if(pDZFFmpeg != NULL){
        pDZFFmpeg->play();
    }
}

extern "C" JNIEXPORT void JNICALL Java_kim_hsl_ffmpeg_DarrenPlayer_openSLES_1Play(JNIEnv *env, jobject thiz, jstring url_) {
    // TODO: implement openSLES_Play()
    const char* url = env->GetStringUTFChars(url_, NULL);
//    pcmFile = fopen(url, "r");
//    //sample 44.1khz, 2 channels, 16bytes
//    pcmBuffer = malloc(44100*2*2);
//    initCreateOpenSELS();
    env->ReleaseStringUTFChars(url_, url);
}

extern "C" JNIEXPORT void JNICALL Java_kim_hsl_ffmpeg_DarrenPlayer_prepare0(JNIEnv *env, jobject thiz, jstring url_) {
    if(pDZJNICall == NULL){
        pDZJNICall = new DZJNICall(pJavaVM, env, thiz);
    }

    const char* url = env->GetStringUTFChars(url_, 0);
    if(pDZFFmpeg == NULL){
        pDZFFmpeg = new DZFFmpeg(pDZJNICall, url);

    }
    pDZFFmpeg->prepare();

    pDZFFmpeg->play();
//    delete pDZJNICall;
//    delete pDZFFmpeg;
    env->ReleaseStringUTFChars(url_, url);
}

extern "C" JNIEXPORT void JNICALL Java_kim_hsl_ffmpeg_DarrenPlayer_prepareAsync0(JNIEnv *env, jobject thiz, jstring url_) {
    if(pDZJNICall == NULL){
        pDZJNICall = new DZJNICall(pJavaVM, env, thiz);
    }

    const char* url = env->GetStringUTFChars(url_, 0);
    if(pDZFFmpeg == NULL){
        pDZFFmpeg = new DZFFmpeg(pDZJNICall, url);

    }
    pDZFFmpeg->prepareAsync();

//    pDZFFmpeg->play();
//    delete pDZJNICall;
//    delete pDZFFmpeg;
    env->ReleaseStringUTFChars(url_, url);
}

extern "C" JNIEXPORT void JNICALL Java_kim_hsl_ffmpeg_DarrenPlayer_decodeVieo0(JNIEnv *env, jobject thiz, jobject surface, jstring url_) {
    LOGE("native decode video entrance");
    const char* url = env->GetStringUTFChars(url_, NULL);

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
//        onJniPlayError(threadMode, FIND_STREAM_ERROR_CODE, av_err2str(formatOpenInputRes));
        return;
    }

    int avformatFindStreamInfo = avformat_find_stream_info(pFormatContext, NULL);
    if(avformatFindStreamInfo < 0){
        LOGE("avformat_find_stream_info error: %s", av_err2str(avformatFindStreamInfo));
//        onJniPlayError(threadMode, FIND_STREAM_INFO_ERROR_CODE, av_err2str(avformatFindStreamInfo));
        return;
    }

    //查找音频流的index
    int videoStreamIndex = av_find_best_stream(pFormatContext, AVMediaType::AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if(videoStreamIndex < 0){
        LOGE("av_find_best_stream find audio stream error: %s", av_err2str(videoStreamIndex));
//        onJniPlayError(threadMode, FIND_BEST_STREAM_ERROR_CODE, av_err2str(videoStreamIndex));
        return;
    }


    pCodecParameters = pFormatContext->streams[videoStreamIndex]->codecpar;
    //查找解码
    pCodec = avcodec_find_decoder(pCodecParameters->codec_id);
    if(pCodec == NULL){
        LOGE("avCodec is null");
//        onJniPlayError(threadMode, CODED_FIND_DECODER_ERROR_CODE, "avCodec is null");
        return;
    }


    //打开解码器
    pCodecContext = avcodec_alloc_context3(pCodec);
    if(pCodecContext == NULL){
        LOGE("pCodecContext is null");
//        onJniPlayError(threadMode, ALLOCATE_CONTEXT_ERROR_CODE, "pCodecContext is null");
        return;
    }

    int avcodecParametersToContextRes = avcodec_parameters_to_context(pCodecContext, pCodecParameters);
    if(avcodecParametersToContextRes  < 0){
        LOGE("codec parameters to context error : %s ", av_err2str(avcodecParametersToContextRes));
//        onJniPlayError(threadMode, AVCODEC_PARAM_TO_CONTEXT_ERROR_CODE, av_err2str(avcodecParametersToContextRes));
        return;
    }
    int avcodecOpenRes = avcodec_open2(pCodecContext, pCodec, NULL);
    if(avcodecOpenRes != 0){
        LOGE("codec audio open fail : %s ", av_err2str(avcodecOpenRes));
//        onJniPlayError(threadMode, AVCODEC_OPEN_2_ERROR_CODE, av_err2str(avcodecOpenRes));
        return;
    }
//    avcodec_open2(pFormatContext->streams[videoStreamIndex]->codec, pCodec, NULL);

    LOGE("%d, %d", pCodecParameters->sample_rate, pCodecParameters->channels);

    //获取窗体
    ANativeWindow* pNativeWindow = ANativeWindow_fromSurface(env, surface);
    //设置缓冲区属性
    ANativeWindow_setBuffersGeometry(pNativeWindow, pCodecContext->width, pCodecContext->height, WINDOW_FORMAT_RGBA_8888);
    //缓冲区Window的buffer
    ANativeWindow_Buffer outBuffer;

    //初始化转换上下文
    SwsContext *swsContext = sws_getContext(pCodecContext->width, pCodecContext->height, pCodecContext->pix_fmt
        , pCodecContext->width, pCodecContext->height, AV_PIX_FMT_RGBA
        , SWS_BILINEAR, NULL, NULL, NULL);
    AVFrame *pRGBAFrame = av_frame_alloc();  //存储解压后转换后的数据
    int frameSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA, pCodecContext->width, pCodecContext->height, 1);
    uint8_t *frameBuffer = (uint8_t *) malloc(frameSize * sizeof(uint8_t));
    av_image_fill_arrays(pRGBAFrame->data, pRGBAFrame->linesize, frameBuffer, AV_PIX_FMT_RGBA, pCodecContext->width, pCodecContext->height, 1);

    AVPacket *pPacket = av_packet_alloc();
    AVFrame *pFrame = av_frame_alloc();  //存储原理解压后的数据
    int index = 0;
    while(av_read_frame(pFormatContext, pPacket) >= 0){
        if(pPacket->stream_index == videoStreamIndex) {
            //Packet包，压缩的数据，解码成pcm数据
            int avcodecSendPacketRes = avcodec_send_packet(pCodecContext, pPacket);
            if (avcodecSendPacketRes == 0) {
                int avcodecReceiveFrameRes = avcodec_receive_frame(pCodecContext, pFrame);
                if (avcodecReceiveFrameRes == 0) {
                    //已经把AVPacket解码成AVFrame
                    index++;
                    LOGE("解码第%d帧", index);

                    //pFrame->data一般都是YUV420P的。SurfaceView需要显示RGBA8888，因此需要转换。
                    sws_scale(swsContext, pFrame->data, pFrame->linesize, 0, pCodecContext->height
                        , pRGBAFrame->data, pRGBAFrame->linesize);
                    //拿到转换后的RGBA8888data后如何渲染？ 放入缓冲区
                    ANativeWindow_lock(pNativeWindow, &outBuffer, NULL);
                    memcpy(outBuffer.bits, frameBuffer, frameSize);
                    ANativeWindow_unlockAndPost(pNativeWindow);
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

    env->ReleaseStringUTFChars(url_, url);

}