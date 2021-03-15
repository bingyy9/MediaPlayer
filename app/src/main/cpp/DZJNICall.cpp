//
// Created by bingy on 2021/3/14.
//

#include "DZJNICall.h"
#include "DZConstDefine.h"

DZJNICall::DZJNICall(JavaVM *javaVM, JNIEnv *jniEnv, jobject obj) {
    this->javaVM = javaVM;
    this->jniEnv = jniEnv;
    this->jDarrenPlayObj = obj;

//    jclass playerClz = jniEnv->FindClass("kim/hsl/ffmpeg/DarrenPlayer");
    jclass playerClz = jniEnv->GetObjectClass(jDarrenPlayObj);
    jDarrenPlayErrorMid= jniEnv->GetMethodID(playerClz, "onError", "(ILjava/lang/String;)V");
    LOGE("jDarrenPlayErrorMid = ");
    initCreateAudioTrack();
}

DZJNICall::~DZJNICall() {
    jniEnv->DeleteLocalRef(jAudioTrackObj);

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

void DZJNICall::callAudioTrackWrite(jbyteArray audioData, int offsetInBytes, int sizeInBytes) {
    jniEnv->CallIntMethod(jAudioTrackObj, jAdudioTrackWriteMid, audioData, offsetInBytes, sizeInBytes);
}

void DZJNICall::onPlayError(int code, char *msg) {
    LOGE("onPlayError code %d, msg %s ", code, msg);
    jstring jmsg = jniEnv->NewStringUTF(msg);
    jniEnv->CallVoidMethod(jDarrenPlayObj, jDarrenPlayErrorMid, code, jmsg);
    jniEnv->DeleteLocalRef(jmsg);
}
