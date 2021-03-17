//
// Created by bingy on 2021/3/14.
//

#ifndef INC_011_FFMPEG_MASTER_DZJNICALL_H
#define INC_011_FFMPEG_MASTER_DZJNICALL_H

#include <jni.h>

enum ThreadMode{
    THREAD_CHILD,THREAD_MAIN
};

class DZJNICall{
public:
    jobject jAudioTrackObj;
    jmethodID jAdudioTrackWriteMid;
    jmethodID jDarrenPlayErrorMid;
    jmethodID jDarrenPlayOnPreparedMid;
    JavaVM *javaVM;
    JNIEnv *jniEnv;
    jobject jDarrenPlayObj;

    void onPlayError(ThreadMode threadMode, int code, char *msg);

public:
    DZJNICall(JavaVM *javaVM, JNIEnv *jniEnv, jobject obj);
    ~DZJNICall();
    void callAudioTrackWrite(ThreadMode threadMode, jbyteArray audioData, int offsetInBytes, int sizeInBytes);
    void onPrepared(ThreadMode threadMode);

private:
    void initCreateAudioTrack();

};

#endif //INC_011_FFMPEG_MASTER_DZJNICALL_H
