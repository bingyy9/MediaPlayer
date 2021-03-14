package kim.hsl.ffmpeg.listener;

public interface MediaErrorListener {
    void onError(int code, String msg);
}
