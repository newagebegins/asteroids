package com.android.mygame;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.app.Activity;
import android.content.Context;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;

public class MyGameActivity extends Activity {

    MyGameView view;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        view = new MyGameView(this);
        setContentView(view);
    }

    @Override
    protected void onPause() {
        super.onPause();
        view.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        view.onResume();
    }
}

class TouchPointer {
    public int id = -1;
    public boolean isDown = false;
    public float x = -1;
    public float y = -1;
}

class MyGameView extends GLSurfaceView {

    public TouchPointer[] pointers;
    public static float[] touches = new float[4];

    public MyGameView(Context context) {
        super(context);
        setEGLContextClientVersion(2);
        setRenderer(new MyGameRenderer());

        pointers = new TouchPointer[2];
        for (int i = 0; i < pointers.length; ++i) {
            pointers[i] = new TouchPointer();
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        int action = event.getActionMasked();
        int pointerIndex = event.getActionIndex();
        int pointerId = event.getPointerId(pointerIndex);
        //Log.d("MyGame", actionToString(action) + "(" + pointerId + ")");
        switch (action) {
            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_POINTER_DOWN:
                for (int i = 0; i < pointers.length; i++) {
                    if (pointers[i].id < 0) {
                        pointers[i].id = pointerId;
                        pointers[i].isDown = true;
                        pointers[i].x = event.getX(pointerIndex);
                        pointers[i].y = event.getY(pointerIndex);
                        break;
                    }
                }
                break;
                
            case MotionEvent.ACTION_MOVE:
                for (int pointerIndex2 = 0; pointerIndex2 < event.getPointerCount(); pointerIndex2++) {
                    int pointerId2 = event.getPointerId(pointerIndex2);
                    for (int i = 0; i < pointers.length; i++) {
                        if (pointers[i].id == pointerId2) {
                            pointers[i].isDown = true;
                            pointers[i].x = event.getX(pointerIndex2);
                            pointers[i].y = event.getY(pointerIndex2);
                            break;
                        }
                    }
                }
                break;
                
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_POINTER_UP:
            case MotionEvent.ACTION_CANCEL:
                for (int i = 0; i < pointers.length; i++) {
                    if (pointers[i].id == pointerId) {
                        pointers[i].id = -1;
                        pointers[i].isDown = false;
                        pointers[i].x = -1;
                        pointers[i].y = -1;
                        break;
                    }
                }
                break;
        }

        int touchCoordIndex = 0;
        for (int i = 0; i < pointers.length; i++) {
            TouchPointer pointer = pointers[i];
            if (pointer.isDown) {
                touches[touchCoordIndex++] = pointer.x;
                touches[touchCoordIndex++] = pointer.y;
            } else {
                touches[touchCoordIndex++] = -1;
                touches[touchCoordIndex++] = -1;
            }
        }
        return true;
    }
    
    public static String actionToString(int action) {
        switch (action) {
            case MotionEvent.ACTION_DOWN: return "Down";
            case MotionEvent.ACTION_MOVE: return "Move";
            case MotionEvent.ACTION_POINTER_DOWN: return "Pointer Down";
            case MotionEvent.ACTION_UP: return "Up";
            case MotionEvent.ACTION_POINTER_UP: return "Pointer Up";
            case MotionEvent.ACTION_OUTSIDE: return "Outside";
            case MotionEvent.ACTION_CANCEL: return "Cancel";
        }
        return "";
    }
}

class MyGameRenderer implements GLSurfaceView.Renderer {

    public void onDrawFrame(GL10 gl) {
        MyGameNativeLib.step(MyGameView.touches);
    }

    public void onSurfaceChanged(GL10 gl, int width, int height) {
        MyGameNativeLib.init(width, height);
    }

    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
    }
}

class MyGameNativeLib {

    static {
        System.loadLibrary("mygame");
    }

    public static native void init(int width, int height);
    public static native void step(float[] touches);
}
