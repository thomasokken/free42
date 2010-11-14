/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2010  Thomas Okken
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see http://www.gnu.org/licenses/.
 *****************************************************************************/

#include <string.h>
#include <jni.h>
//#include <core_main.h>
//#include <core_display.h>
void shell_blitter(const char *bits, int bytesperline, int x, int y, int width, int height);

/* This is a trivial JNI example where we use a native method
 * to return a new VM String. See the corresponding Java source
 * file located at:
 *
 *   apps/samples/hello-jni/project/src/com/example/HelloJni/HelloJni.java
 */
extern "C" jstring
Java_com_thomasokken_free42_Free42Activity_stringFromJNI(JNIEnv *env, jobject thiz) {
    return env->NewStringUTF("Hello from JNI !");
}

static JNIEnv *g_env;
static jobject g_activity;

extern "C" void
Java_com_thomasokken_free42_Free42Activity_nativeInit(JNIEnv *env, jobject thiz) {
    /* I'm hanging on to the JNIEnv pointer so that I can use it to call
     * Java methods. Note that the env pointer is only valid within one
     * thread, so it is critical that all calls into the native code
     * be made from the same thread (which shouldn't be the main thread).
     */
    g_env = env;
    g_activity = thiz;
}

/**********************************************************/
/* Here followeth the stubs for the core_main.h interface */
/**********************************************************/

extern "C" void
Java_com_thomasokken_free42_Free42Activity_redisplay(JNIEnv *env, jobject thiz) {
    // redisplay();
    char disp[272];
    for (int i = 0; i < 272; i++)
	disp[i] = i;
    shell_blitter(disp, 17, 0, 0, 131, 16);
}

/***************************************************************/
/* Here followeth the implementation of the shell.h interface. */
/***************************************************************/

void shell_blitter(const char *bits, int bytesperline, int x, int y,
		         int width, int height) {
    jclass klass = g_env->GetObjectClass(g_activity);
    jmethodID mid = g_env->GetMethodID(klass, "shell_blitter", "([BIIIII)V");
    int size = bytesperline * height;
    jbyteArray bits2 = g_env->NewByteArray(size);
    g_env->SetByteArrayRegion(bits2, 0, size, (const jbyte *) bits);
    g_env->CallVoidMethod(g_activity, mid, bits2, bytesperline, x, y, width, height);
}
