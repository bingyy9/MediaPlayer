package kim.hsl.ffmpeg;

import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;

import kim.hsl.ffmpeg.listener.MediaErrorListener;
import kim.hsl.ffmpeg.listener.MediaPreparedListener;

class DarrenPlayer {
    private String url;
    private MediaErrorListener mErrorListener;
    private MediaPreparedListener mediaPreparedListener;

    public void setMediaPreparedListener(MediaPreparedListener listener) {
        this.mediaPreparedListener = listener;
    }

    //invoke from JNI
    private void onPrepared(){
        Log.e("DarrenPlayer onPrepared", "");
        if(mediaPreparedListener != null){
            mediaPreparedListener.onPrepared();
        }
    }

    public void setErrorListener(MediaErrorListener listener) {
        this.mErrorListener = listener;
    }

    //invoke from JNI
    private void onError(int code, String msg){
        Log.e("DarrenPlayer onError", "code: " + code + " msg: " + msg);
        if(mErrorListener != null){
            mErrorListener.onError(code, msg);
        }
    }

    public void setDataSource(String url){
        this.url = url;
    }

    public void play(){
        if(TextUtils.isEmpty(url)){
            throw new NullPointerException("url is null");
        }
        play0(url); // this is for audio play
//        decodeVieo0(url);
    }

    public void decodeVideo(Surface surface){
        decodeVieo0(surface, url);
    }

    private native void decodeVieo0(Surface surface, String url);

    private native void play0(String url);

    private native void openSLES_Play(String url);

    private native void prepare0(String url);
    private native void prepareAsync0(String url);

    public void prepare() {
        prepare0(url);
    }

    public void prepareAsync() {
        prepareAsync0(url);
    }

    public native void setSurface(Surface surface);
}
