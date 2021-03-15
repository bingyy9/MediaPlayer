package kim.hsl.ffmpeg;

import android.preference.MultiSelectListPreference;
import android.text.TextUtils;
import android.util.Log;

import kim.hsl.ffmpeg.listener.MediaErrorListener;

class DarrenPlayer {
    private String url;
    private MediaErrorListener mErrorListener;

    public void setErrorListener(MediaErrorListener listener) {
        this.mErrorListener = listener;
    }

    //invoke from JNI
    private void onError(int code, String msg){
        Log.e("DarrenPlayer", "code: " + code + " msg: " + msg);
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

        play0(url);
    }

    private native void play0(String url);

}
