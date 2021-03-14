#include <jni.h>
#include <string>

//本地窗口 , 在 Native 层处理图像绘制
#include <android/native_window_jni.h>

//在C++中采用C的方式编译，因为ffmpeg都是C语言写的
extern "C" {
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
}
#include "DZConstDefine.h"
#include "DZJNICall.h"
#include "DZFFmpeg.h"

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
JavaVM *javaVM;
int JNI_OnLoad(JavaVM *vm, void *r){

    javaVM = vm;

    return JNI_VERSION_1_6;
}

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
    javaCallHelper = new JavaPlayerCaller(javaVM, env, instance);

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

DZJNICall *pDZJNICall;
DZFFmpeg *pDZFFmpeg;

extern "C" JNIEXPORT void JNICALL
Java_kim_hsl_ffmpeg_DarrenPlayer_play0(JNIEnv *env, jobject thiz, jstring url_) {
    pDZJNICall = new DZJNICall(NULL, env);
    const char* url = env->GetStringUTFChars(url_, 0);
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
//        goto __av_resources_destroy;
    }

    avformatFindStreamInfo = avformat_find_stream_info(pFormatContext, NULL);
    if(avformatFindStreamInfo < 0){
        LOGE("avformat_find_stream_info error: %s", av_err2str(avformatFindStreamInfo));
//        goto __av_resources_destroy;
    }

    //查找音频流的index
    audioStreamIndex = av_find_best_stream(pFormatContext, AVMediaType::AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if(audioStreamIndex < 0){
        LOGE("av_find_best_stream find audio stream error: %s", av_err2str(audioStreamIndex));
//        goto __av_resources_destroy;
    }

    pCodecParameters = pFormatContext->streams[audioStreamIndex]->codecpar;
    //查找解码
    pCodec = avcodec_find_decoder(pCodecParameters->codec_id);
    if(pCodec == NULL){
        LOGE("avCodec is null");
//        goto __av_resources_destroy;
    }


    //打开解码器
    pCodecContext = avcodec_alloc_context3(pCodec);
    if(pCodecContext == NULL){
        LOGE("pCodecContext is null");
//        goto __av_resources_destroy;
    }

    avcodecParametersToContextRes = avcodec_parameters_to_context(pCodecContext, pCodecParameters);
    if(avcodecParametersToContextRes  < 0){
        LOGE("codec parameters to context error : %s ", av_err2str(avcodecParametersToContextRes));
//        goto __av_resources_destroy;
    }
    avcodecOpenRes = avcodec_open2(pCodecContext, pCodec, NULL);
    if(avcodecOpenRes != 0){
        LOGE("codec audio open fail : %s ", av_err2str(avcodecOpenRes));
//        goto __av_resources_destroy;
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
        //error
        return;
    }
    int swrInitRes = swr_init(swrContext);
    if(swrInitRes < 0){
        //error
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

    jPcmByteArray = env->NewByteArray(avSamplesBufferSize);
    jPcmData = env->GetByteArrayElements(jPcmByteArray, NULL);

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
                    env->ReleaseByteArrayElements(jPcmByteArray, jPcmData, JNI_COMMIT);
                    pDZJNICall->callAudioTrackWrite(jPcmByteArray, 0, avSamplesBufferSize);

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
    env->ReleaseByteArrayElements(jPcmByteArray, jPcmData, 0);
    env->DeleteLocalRef(jPcmByteArray);

    delete pDZJNICall;
__av_resources_destroy:
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

    avformat_network_deinit();
    env->ReleaseStringUTFChars(url_, url);
}

extern "C" JNIEXPORT void JNICALL
Java_kim_hsl_ffmpeg_DarrenPlayer_play1(JNIEnv *env, jobject thiz, jstring url_) {
    pDZJNICall = new DZJNICall(NULL, env);
    const char* url = env->GetStringUTFChars(url_, 0);
    pDZFFmpeg = new DZFFmpeg(pDZJNICall, url);
    pDZFFmpeg->play();
    delete pDZJNICall;
    delete pDZFFmpeg;
    env->ReleaseStringUTFChars(url_, url);
}