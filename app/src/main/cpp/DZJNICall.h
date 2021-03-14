//
// Created by bingy on 2021/3/14.
//

#ifndef INC_011_FFMPEG_MASTER_DZJNICALL_H
#define INC_011_FFMPEG_MASTER_DZJNICALL_H

#include <jni.h>

class DZJNICall{
public:
    jobject jAudioTrackObj;
    jmethodID jAdudioTrackWriteMid;
    JavaVM *javaVM;
    JNIEnv *jniEnv;

public:
    DZJNICall(JavaVM *javaVM, JNIEnv *jniEnv);
    ~DZJNICall();
    void callAudioTrackWrite(jbyteArray audioData, int offsetInBytes, int sizeInBytes);

private:
    void initCreateAudioTrack();
};

#endif //INC_011_FFMPEG_MASTER_DZJNICALL_H
