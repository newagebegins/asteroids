#include <jni.h>
#include <android/log.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "../../win32/mygame.h"

#define  LOG_TAG    "MyGame"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define platformLog(buf) LOGI("%s", buf)

#include <sys/time.h>

static long
_getTime(void)
{
    struct timeval now;
    gettimeofday(&now, 0);
    return (long)(now.tv_sec*1000 + now.tv_usec/1000);
}

static int OldCount;
static int CurrCount;

bool setupGraphics(int screenWidth, int screenHeight) {
    if (!initGame()) {
        platformLog("Failed to initGame()\n");
        return false;
    }
    setViewport(screenWidth, screenHeight);
    OldCount = _getTime();
    return true;
}

void renderFrame(float *touches) {
    float dt = 0.0f;
    float targetDt = 1.0f / 60.0f;

    OldCount = CurrCount;
    CurrCount = _getTime();
    dt = (float)(CurrCount - OldCount) / 1000.0f;
    if (dt > targetDt) {
        dt = targetDt;
    }

    gameUpdateAndRender(dt, touches);
}

extern "C" {
    JNIEXPORT void JNICALL Java_com_android_mygame_MyGameNativeLib_init(JNIEnv * env, jobject obj,  jint width, jint height);
    JNIEXPORT void JNICALL Java_com_android_mygame_MyGameNativeLib_step(JNIEnv * env, jobject obj, jfloatArray touches);
};

JNIEXPORT void JNICALL Java_com_android_mygame_MyGameNativeLib_init(JNIEnv * env, jobject obj,  jint width, jint height)
{
    setupGraphics(width, height);
}

JNIEXPORT void JNICALL Java_com_android_mygame_MyGameNativeLib_step(JNIEnv * env, jobject obj, jfloatArray touches)
{
    jfloat* touches_ = env->GetFloatArrayElements(touches, 0);
    renderFrame(touches_);
    env->ReleaseFloatArrayElements(touches, touches_, 0);
}
