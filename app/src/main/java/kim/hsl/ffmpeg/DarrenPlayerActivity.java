package kim.hsl.ffmpeg;

import android.Manifest;
import android.app.Activity;
import android.content.pm.PackageManager;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.Toast;

import java.io.File;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import kim.hsl.ffmpeg.listener.MediaErrorListener;
import kim.hsl.ffmpeg.listener.MediaPreparedListener;

import static java.lang.Thread.sleep;

public class DarrenPlayerActivity extends Activity implements MediaPreparedListener, MediaErrorListener {

    private static final String TAG = "DarrenPlayerActivity";

    private DZVideoView mVideoView;

//    DarrenPlayer darrenPlayer;

    @RequiresApi(api = Build.VERSION_CODES.M)
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        //全屏
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);

        setContentView(R.layout.activity_darren_player);

        mVideoView = findViewById(R.id.surface_view);
//        darrenPlayer = new DarrenPlayer();
//        darrenPlayer.setErrorListener(this);
//        darrenPlayer.setMediaPreparedListener(this);

    }

    @Override
    protected void onResume() {
        super.onResume();
        testDarrenVideoPlayer();
    }

    private void testDarrenVideoPlayer(){
//        mVideoView.play();
//        File mVideoFile = new File(Environment.getExternalStorageDirectory(), "屌丝男士.mov");
//        darrenPlayer.setDataSource(mVideoFile.getAbsolutePath());
//        darrenPlayer.decodeVideo(mVideoView.getHolder().getSurface());
    }

    private AudioTrack createAudioTrack(int sampleRateInHz, int nb_channels) {
        //固定格式的音频码流
        int audioFormat = AudioFormat.ENCODING_PCM_16BIT;
        //声道布局
        int channelConfig;
        if (nb_channels == 1) {
            channelConfig = AudioFormat.CHANNEL_OUT_MONO;
        } else {
            channelConfig = AudioFormat.CHANNEL_OUT_STEREO;
        }

        int bufferSizeInBytes = AudioTrack.getMinBufferSize(sampleRateInHz, channelConfig, audioFormat);

        AudioTrack audioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRateInHz, channelConfig, audioFormat, bufferSizeInBytes, AudioTrack.MODE_STREAM);
        return audioTrack;
    }

    @Override
    public void onError(int code, String msg) {

    }

    @Override
    public void onPrepared() {
        Log.e(TAG, "onPrepared");
//        darrenPlayer.play();
    }
}
