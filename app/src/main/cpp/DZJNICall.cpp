//
// Created by bingy on 2021/3/14.
//

#include "DZJNICall.h"
#include "DZConstDefine.h"

DZJNICall::DZJNICall(JavaVM *javaVM, JNIEnv *jniEnv, jobject obj) {
    this->javaVM = javaVM;
    this->jniEnv = jniEnv;
    //NewGlobalRef, so it can be used in child thread
    this->jDarrenPlayObj = jniEnv->NewGlobalRef(obj);

//    jclass playerClz = jniEnv->FindClass("kim/hsl/ffmpeg/DarrenPlayer");
    jclass playerClz = jniEnv->GetObjectClass(jDarrenPlayObj);
    jDarrenPlayErrorMid= jniEnv->GetMethodID(playerClz, "onError", "(ILjava/lang/String;)V");
    jDarrenPlayOnPreparedMid= jniEnv->GetMethodID(playerClz, "onPrepared", "()V");
    initCreateAudioTrack();
}

DZJNICall::~DZJNICall() {
    jniEnv->DeleteLocalRef(jAudioTrackObj);
    jniEnv->DeleteGlobalRef(jDarrenPlayObj);
}

void DZJNICall::initCreateAudioTrack() {
    jclass audioTrackClz = jniEnv->FindClass("android/media/AudioTrack");
    jmethodID jMid= jniEnv->GetMethodID(audioTrackClz, "<init>", "(IIIIII)V");
    int channelConfig = 0x4 | 0x8;
    int audioFormat = 2; // PCM 16Bit是2个字节
    int mode = 1;
    int sampleRateInHz = AUDIO_SAMPLE_RATE;

    jmethodID getMinBufferSizeMethodId = jniEnv->GetStaticMethodID(audioTrackClz, "getMinBufferSize", "(III)I");
    int bufferSizeInBytes = jniEnv->CallStaticIntMethod(audioTrackClz, getMinBufferSizeMethodId, sampleRateInHz, channelConfig, audioFormat);

    jAudioTrackObj = jniEnv->NewObject(audioTrackClz, jMid, 3, sampleRateInHz, channelConfig, audioFormat, bufferSizeInBytes, mode);
    LOGE("bufferSizeInBytes = %d", bufferSizeInBytes);
    //调用play()方法
    jmethodID playMethodId = jniEnv->GetMethodID(audioTrackClz, "play", "()V");
    jniEnv->CallVoidMethod(jAudioTrackObj, playMethodId);

    jAdudioTrackWriteMid = jniEnv->GetMethodID(audioTrackClz, "write", "([BII)I");

}

void DZJNICall::callAudioTrackWrite(ThreadMode threadMode, jbyteArray audioData, int offsetInBytes, int sizeInBytes) {
    LOGE("callAudioTrackWrite");
    if(threadMode == THREAD_MAIN) {
        jniEnv->CallIntMethod(jAudioTrackObj, jAdudioTrackWriteMid, audioData, offsetInBytes, sizeInBytes);
    } else {
        JNIEnv *env;
        if(javaVM->AttachCurrentThread(&env, 0) != JNI_OK){
            LOGE("get child thread jniEnv error!!");
            return;
        }
        env->CallIntMethod(jAudioTrackObj, jAdudioTrackWriteMid, audioData, offsetInBytes, sizeInBytes);
        javaVM->DetachCurrentThread();
    }
    LOGE("callAudioTrackWrite end");
}

void DZJNICall::onPlayError(ThreadMode threadMode, int code, char *msg) {
    LOGE("onPlayError code %d, msg %s ", code, msg);
    if(threadMode == THREAD_MAIN){
        jstring jmsg = jniEnv->NewStringUTF(msg);
        jniEnv->CallVoidMethod(jDarrenPlayObj, jDarrenPlayErrorMid, code, jmsg);
        jniEnv->DeleteLocalRef(jmsg);
    } else if(threadMode == THREAD_CHILD){
        //获取当前线程的JNIEnv，通过JavaVM
        JNIEnv *env;
        if(javaVM->AttachCurrentThread(&env, 0) != JNI_OK){
            LOGE("get child thread jniEnv error!!");
            return;
        }
        jstring jmsg = env->NewStringUTF(msg);
        env->CallVoidMethod(jDarrenPlayObj, jDarrenPlayErrorMid, code, jmsg);
        env->DeleteLocalRef(jmsg);
        javaVM->DetachCurrentThread();
    }
}

void DZJNICall::onPrepared(ThreadMode threadMode) {
    LOGE("onPrepared");
    if(threadMode == THREAD_MAIN){
        jniEnv->CallVoidMethod(jDarrenPlayObj, jDarrenPlayOnPreparedMid);
    } else if(threadMode == THREAD_CHILD){
        //获取当前线程的JNIEnv，通过JavaVM
        JNIEnv *env;
        if(javaVM->AttachCurrentThread(&env, 0) != JNI_OK){
            LOGE("get child thread jniEnv error!!");
            return;
        }
        env->CallVoidMethod(jDarrenPlayObj, jDarrenPlayOnPreparedMid);
        javaVM->DetachCurrentThread();
    }
}

