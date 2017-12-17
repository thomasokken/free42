/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2017  Thomas Okken
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
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include "shell.h"
#include "core_main.h"
#include "core_display.h"


#define GLUE_DEBUG 0

#if GLUE_DEBUG
static int level = 0;
void shell_logprintf(const char *format, ...);
#endif

class Tracer {
#if GLUE_DEBUG
    private:
        const char *name;
    public:
        Tracer(const char *name) {
            this->name = name;
            for (int i = 0; i < level; i++)
                shell_logprintf("  ");
            shell_logprintf("ENTERING %s\n", name);
            level++;
        }
        ~Tracer() {
            --level;
            for (int i = 0; i < level; i++)
                shell_logprintf("  ");
            shell_logprintf("EXITING  %s\n", name);
        }
#else
    public:
        Tracer(const char *) {}
#endif
};


/**********************************************/
/* Some stuff to help native->java callbacks. */
/**********************************************/

static jobject g_activity;
static JavaVM *vm;

extern "C" void
Java_com_thomasokken_free42_Free42Activity_nativeInit(JNIEnv *env, jobject thiz) {
    /* I'm hanging on to the Free42Activity pointer so I can use it to call
     * Java methods. The JNIEnv pointer is thread-local, so I don't cache it;
     * use the getJniEnv() function, defined below, to retrieve it when needed.
     */
    g_activity = env->NewGlobalRef(thiz);
    env->GetJavaVM(&vm);
}

static JNIEnv *getJniEnv() {
    JNIEnv *env;
    vm->GetEnv((void **) &env, JNI_VERSION_1_2);
    return env;
}


/*******************************************************************/
/* A couple of functions to enable the Java code to get the values */
/* of the FREE42_MAGIC and FREE42_VERSION macros.                  */
/*******************************************************************/

extern "C" jint
Java_com_thomasokken_free42_Free42Activity_FREE42_1MAGIC(JNIEnv *env, jobject thiz) {
    return FREE42_MAGIC;
}

extern "C" jint
Java_com_thomasokken_free42_Free42Activity_FREE42_1VERSION(JNIEnv *env, jobject thiz) {
    return FREE42_VERSION;
}


/*********************************************************************/
/* Since JNI calls are very expensive, I avoid polling the shell to  */
/* find out if core_keydown() should return, and instead rely on the */
/* shell to tell the core, through the following function.           */
/*********************************************************************/

static bool finish_flag = false;

extern "C" void
Java_com_thomasokken_free42_Free42Activity_core_1keydown_1finish(JNIEnv *env, jobject thiz) {
    Tracer T("core_keydown_finish");
    finish_flag = true;
}


/**********************************************************/
/* Here followeth the stubs for the core_main.h interface */
/**********************************************************/

extern "C" void
Java_com_thomasokken_free42_Free42Activity_core_1init(JNIEnv *env, jobject thiz, jint read_state, jint version) {
    Tracer T("core_init");
    core_init(read_state, version);
}

extern "C" void
Java_com_thomasokken_free42_Free42Activity_core_1enter_1background(JNIEnv *env, jobject thiz) {
    Tracer T("core_enter_background");
    core_enter_background();
}

extern "C" void
Java_com_thomasokken_free42_Free42Activity_core_1quit(JNIEnv *env, jobject thiz) {
    Tracer T("core_quit");
    core_quit();
}

extern "C" void
Java_com_thomasokken_free42_Free42Activity_core_1repaint_1display(JNIEnv *env, jobject thiz) {
    Tracer T("core_repaint_display");
    core_repaint_display();
}

extern "C" jboolean
Java_com_thomasokken_free42_Free42Activity_core_1menu(JNIEnv *env, jobject thiz) {
    Tracer T("core_menu");
    return core_menu();
}

extern "C" jboolean
Java_com_thomasokken_free42_Free42Activity_core_1alpha_1menu(JNIEnv *env, jobject thiz) {
    Tracer T("core_alpha_menu");
    return core_alpha_menu();
}

extern "C" jboolean
Java_com_thomasokken_free42_Free42Activity_core_1hex_1menu(JNIEnv *env, jobject thiz) {
    Tracer T("core_hex_menu");
    return core_hex_menu();
}

extern "C" jboolean
Java_com_thomasokken_free42_Free42Activity_core_1keydown(JNIEnv *env, jobject thiz,
                            jint key, jobject enqueued, jobject repeat, jboolean immediate_return) {
    Tracer T("core_keydown");
    finish_flag = immediate_return;
    int enq, rep;
    jboolean ret;
    do {
        ret = core_keydown(key, &enq, &rep);
    } while (ret && !finish_flag);
    jclass klass = env->GetObjectClass(enqueued);
    jfieldID fid = env->GetFieldID(klass, "value", "Z");
    env->SetBooleanField(enqueued, fid, enq);
    klass = env->GetObjectClass(repeat);
    fid = env->GetFieldID(klass, "value", "I");
    env->SetIntField(repeat, fid, rep);
    return ret;
}

extern "C" jint
Java_com_thomasokken_free42_Free42Activity_core_1repeat(JNIEnv *env, jobject thiz) {
    Tracer T("core_repeat");
    return core_repeat();
}

extern "C" void
Java_com_thomasokken_free42_Free42Activity_core_1keytimeout1(JNIEnv *env, jobject thiz) {
    Tracer T("core_keytimeout1");
    core_keytimeout1();
}

extern "C" void
Java_com_thomasokken_free42_Free42Activity_core_1keytimeout2(JNIEnv *env, jobject thiz) {
    Tracer T("core_keytimeout2");
    core_keytimeout2();
}

extern "C" jboolean
Java_com_thomasokken_free42_Free42Activity_core_1timeout3(JNIEnv *env, jobject thiz, jint repaint) {
    Tracer T("core_timeout3");
    return core_timeout3(repaint);
}

extern "C" jboolean
Java_com_thomasokken_free42_Free42Activity_core_1keyup(JNIEnv *env, jobject thiz) {
    Tracer T("core_keyup");
    return core_keyup();
}

extern "C" jboolean
Java_com_thomasokken_free42_Free42Activity_core_1powercycle(JNIEnv *env, jobject thiz) {
    Tracer T("core_powercycle");
    return core_powercycle();
}

extern "C" jint
Java_com_thomasokken_free42_Free42Activity_core_1list_1programs(JNIEnv *env, jarray thiz, jbyteArray buf) {
    Tracer T("core_list_programs");
    int bufsize = env->GetArrayLength(buf);
    char *cbuf = (char *) malloc(bufsize);
    int ret = core_list_programs(cbuf, bufsize);
    env->SetByteArrayRegion(buf, 0, bufsize, (const jbyte *) cbuf);
    free(cbuf);
    return ret;
}

extern "C" jint
Java_com_thomasokken_free42_Free42Activity_core_1program_1size(JNIEnv *env, jobject thiz, jint prgm_index) {
    Tracer T("core_program_size");
    return core_program_size(prgm_index);
}

extern "C" jboolean
Java_com_thomasokken_free42_Free42Activity_core_1export_1programs(JNIEnv *env, jobject thiz, jintArray indexes) {
    Tracer T("core_export_programs");
    int count = env->GetArrayLength(indexes);
    int *indexes2 = (int *) malloc(count * sizeof(int));
    env->GetIntArrayRegion(indexes, 0, count, indexes2);
    jboolean ret = core_export_programs(count, indexes2, NULL);
    free(indexes2);
    return ret;
}

extern "C" void
Java_com_thomasokken_free42_Free42Activity_core_1import_1programs(JNIEnv *env, jobject thiz) {
    Tracer T("core_import_programs");
    core_import_programs(NULL);
}

extern "C" jstring
Java_com_thomasokken_free42_Free42Activity_core_1copy(JNIEnv *env, jobject thiz) {
    Tracer T("core_copy");
    char *buf = core_copy();
    jstring s = env->NewStringUTF(buf);
    free(buf);
    return s;
}

extern "C" void
Java_com_thomasokken_free42_Free42Activity_core_1paste(JNIEnv *env, jobject thiz, jstring s) {
    Tracer T("core_paste");
    const char *buf = env->GetStringUTFChars(s, NULL);
    core_paste(buf);
    env->ReleaseStringUTFChars(s, buf);
}

extern "C" void
Java_com_thomasokken_free42_Free42Activity_getCoreSettings(JNIEnv *env, jobject thiz, jobject settings) {
    Tracer T("getCoreSettings");
    jclass klass = env->GetObjectClass(settings);
    jfieldID fid = env->GetFieldID(klass, "matrix_singularmatrix", "Z");
    env->SetBooleanField(settings, fid, core_settings.matrix_singularmatrix);
    fid = env->GetFieldID(klass, "matrix_outofrange", "Z");
    env->SetBooleanField(settings, fid, core_settings.matrix_outofrange);
    fid = env->GetFieldID(klass, "auto_repeat", "Z");
    env->SetBooleanField(settings, fid, core_settings.auto_repeat);
    fid = env->GetFieldID(klass, "enable_ext_accel", "Z");
    env->SetBooleanField(settings, fid, core_settings.enable_ext_accel);
    fid = env->GetFieldID(klass, "enable_ext_locat", "Z");
    env->SetBooleanField(settings, fid, core_settings.enable_ext_locat);
    fid = env->GetFieldID(klass, "enable_ext_heading", "Z");
    env->SetBooleanField(settings, fid, core_settings.enable_ext_heading);
    fid = env->GetFieldID(klass, "enable_ext_time", "Z");
    env->SetBooleanField(settings, fid, core_settings.enable_ext_time);
    fid = env->GetFieldID(klass, "enable_ext_fptest", "Z");
    env->SetBooleanField(settings, fid, core_settings.enable_ext_fptest);
}

extern "C" void
Java_com_thomasokken_free42_Free42Activity_putCoreSettings(JNIEnv *env, jobject thiz, jobject settings) {
    Tracer T("putCoreSettings");
    jclass klass = env->GetObjectClass(settings);
    jfieldID fid = env->GetFieldID(klass, "matrix_singularmatrix", "Z");
    core_settings.matrix_singularmatrix = env->GetBooleanField(settings, fid);
    fid = env->GetFieldID(klass, "matrix_outofrange", "Z");
    core_settings.matrix_outofrange = env->GetBooleanField(settings, fid);
    fid = env->GetFieldID(klass, "auto_repeat", "Z");
    core_settings.auto_repeat = env->GetBooleanField(settings, fid);
    fid = env->GetFieldID(klass, "enable_ext_accel", "Z");
    core_settings.enable_ext_accel = env->GetBooleanField(settings, fid);
    fid = env->GetFieldID(klass, "enable_ext_locat", "Z");
    core_settings.enable_ext_locat = env->GetBooleanField(settings, fid);
    fid = env->GetFieldID(klass, "enable_ext_heading", "Z");
    core_settings.enable_ext_heading = env->GetBooleanField(settings, fid);
    fid = env->GetFieldID(klass, "enable_ext_time", "Z");
    core_settings.enable_ext_time = env->GetBooleanField(settings, fid);
    fid = env->GetFieldID(klass, "enable_ext_fptest", "Z");
    core_settings.enable_ext_fptest = env->GetBooleanField(settings, fid);
}

extern "C" void
Java_com_thomasokken_free42_Free42Activity_redisplay(JNIEnv *env, jobject thiz) {
    Tracer T("redisplay");
    redisplay();
}

static bool alwaysRepaintFullDisplay = false;

extern "C" void
Java_com_thomasokken_free42_Free42Activity_setAlwaysRepaintFullDisplay(JNIEnv *env, jobject thiz, jboolean repaintFull) {
    Tracer T("setAlwaysRepaintFullDisplay");
    alwaysRepaintFullDisplay = repaintFull;
}


/***************************************************************/
/* Here followeth the implementation of the shell.h interface. */
/***************************************************************/

// Note that most of these functions call DeleteLocalRef() to get rid of local
// references they obtained; this is necessary because many of these functions
// may be called arbitrarily many times within one native call (specifically,
// one core_keydown() invocation), so not releasing local references will lead
// to a "ReferenceTable overflow" eventually while running programs.

void shell_blitter(const char *bits, int bytesperline, int x, int y,
                         int width, int height) {
    Tracer T("shell_blitter");
    if (alwaysRepaintFullDisplay) {
        x = 0;
        y = 0;
        width = 131;
        height = 16;
    }
    JNIEnv *env = getJniEnv();
    jclass klass = env->GetObjectClass(g_activity);
    jmethodID mid = env->GetMethodID(klass, "shell_blitter", "([BIIIII)V");
    int size = bytesperline * (y + height);
    jbyteArray bits2 = env->NewByteArray(size);
    env->SetByteArrayRegion(bits2, 0, size, (const jbyte *) bits);
    env->CallVoidMethod(g_activity, mid, bits2, bytesperline, x, y, width, height);
    // Delete local references
    env->DeleteLocalRef(klass);
    env->DeleteLocalRef(bits2);
}

void shell_beeper(int frequency, int duration) {
    Tracer T("shell_beeper");
    JNIEnv *env = getJniEnv();
    jclass klass = env->GetObjectClass(g_activity);
    jmethodID mid = env->GetMethodID(klass, "shell_beeper", "(II)V");
    env->CallVoidMethod(g_activity, mid, frequency, duration);
    // Delete local references
    env->DeleteLocalRef(klass);
}

void shell_annunciators(int updn, int shf, int prt, int run, int g, int rad) {
    Tracer T("shell_annunciators");
    JNIEnv *env = getJniEnv();
    jclass klass = env->GetObjectClass(g_activity);
    jmethodID mid = env->GetMethodID(klass, "shell_annunciators", "(IIIIII)V");
    env->CallVoidMethod(g_activity, mid, updn, shf, prt, run, g, rad);
    // Delete local references
    env->DeleteLocalRef(klass);
}

int shell_wants_cpu() {
    Tracer T("shell_wants_cpu");
    return finish_flag;
}

void shell_delay(int duration) {
    Tracer T("shell_delay");
    struct timespec ts;
    ts.tv_sec = duration / 1000;
    ts.tv_nsec = (duration % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

void shell_request_timeout3(int delay) {
    Tracer T("shell_request_timeout3");
    JNIEnv *env = getJniEnv();
    jclass klass = env->GetObjectClass(g_activity);
    jmethodID mid = env->GetMethodID(klass, "shell_request_timeout3", "(I)V");
    env->CallVoidMethod(g_activity, mid, delay);
    // Delete local references
    env->DeleteLocalRef(klass);
}

int shell_read_saved_state(void *buf, int4 bufsize) {
    Tracer T("shell_read_saved_state");
    JNIEnv *env = getJniEnv();
    jclass klass = env->GetObjectClass(g_activity);
    jmethodID mid = env->GetMethodID(klass, "shell_read_saved_state", "([B)I");
    jbyteArray buf2 = env->NewByteArray(bufsize);
    int n = env->CallIntMethod(g_activity, mid, buf2);
    if (n > 0)
        env->GetByteArrayRegion(buf2, 0, n, (jbyte *) buf);
    // Delete local references
    env->DeleteLocalRef(klass);
    env->DeleteLocalRef(buf2);
    return n;
}

bool shell_write_saved_state(const void *buf, int4 bufsize) {
    Tracer T("shell_write_saved_state");
    JNIEnv *env = getJniEnv();
    jclass klass = env->GetObjectClass(g_activity);
    jmethodID mid = env->GetMethodID(klass, "shell_write_saved_state", "([B)Z");
    jbyteArray buf2 = env->NewByteArray(bufsize);
    env->SetByteArrayRegion(buf2, 0, bufsize, (const jbyte *) buf);
    bool ret = env->CallBooleanMethod(g_activity, mid, buf2);
    // Delete local references
    env->DeleteLocalRef(klass);
    env->DeleteLocalRef(buf2);
    return ret;
}

unsigned int shell_get_mem() {
    Tracer T("shell_get_mem");
    JNIEnv *env = getJniEnv();
    jclass klass = env->GetObjectClass(g_activity);
    jmethodID mid = env->GetMethodID(klass, "shell_get_mem", "()I");
    unsigned int ret = env->CallIntMethod(g_activity, mid);
    // Delete local references
    env->DeleteLocalRef(klass);
    return ret;
}

int shell_low_battery() {
    Tracer T("shell_low_battery");
    JNIEnv *env = getJniEnv();
    jclass klass = env->GetObjectClass(g_activity);
    jmethodID mid = env->GetMethodID(klass, "shell_low_battery", "()I");
    int ret = env->CallIntMethod(g_activity, mid);
    // Delete local references
    env->DeleteLocalRef(klass);
    return ret;
}

void shell_powerdown() {
    Tracer T("shell_powerdown");
    JNIEnv *env = getJniEnv();
    jclass klass = env->GetObjectClass(g_activity);
    jmethodID mid = env->GetMethodID(klass, "shell_powerdown", "()V");
    env->CallVoidMethod(g_activity, mid);
    // Delete local references
    env->DeleteLocalRef(klass);
}

int8 shell_random_seed() {
    Tracer T("shell_random_seed");
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000LL + tv.tv_usec / 1000;
}

uint4 shell_milliseconds() {
    Tracer T("shell_milliseconds");
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint4) (tv.tv_sec * 1000L + tv.tv_usec / 1000);
}

int shell_decimal_point() {
    Tracer T("shell_decimal_point");
    JNIEnv *env = getJniEnv();
    jclass klass = env->GetObjectClass(g_activity);
    jmethodID mid = env->GetMethodID(klass, "shell_decimal_point", "()I");
    unsigned int ret = env->CallIntMethod(g_activity, mid);
    // Delete local references
    env->DeleteLocalRef(klass);
    return ret;
}

void shell_print(const char *text, int length,
                 const char *bits, int bytesperline,
                 int x, int y, int width, int height) {
    Tracer T("shell_print");
    JNIEnv *env = getJniEnv();
    jclass klass = env->GetObjectClass(g_activity);
    jmethodID mid = env->GetMethodID(klass, "shell_print", "([B[BIIIII)V");
    jbyteArray text2 = env->NewByteArray(length);
    env->SetByteArrayRegion(text2, 0, length, (const jbyte *) text);
    int bitmapsize = bytesperline * height;
    jbyteArray bits2 = env->NewByteArray(bitmapsize);
    env->SetByteArrayRegion(bits2, 0, bitmapsize, (const jbyte *) bits);
    env->CallVoidMethod(g_activity, mid, text2, bits2, bytesperline, x, y, width, height);
    // Delete local references
    env->DeleteLocalRef(klass);
    env->DeleteLocalRef(text2);
    env->DeleteLocalRef(bits2);
}

int shell_write(const char *buf, int4 bufsize) {
    Tracer T("shell_write");
    JNIEnv *env = getJniEnv();
    jclass klass = env->GetObjectClass(g_activity);
    jmethodID mid = env->GetMethodID(klass, "shell_write", "([B)I");
    jbyteArray buf2 = env->NewByteArray(bufsize);
    env->SetByteArrayRegion(buf2, 0, bufsize, (const jbyte *) buf);
    int ret = env->CallIntMethod(g_activity, mid, buf2);
    // Delete local references
    env->DeleteLocalRef(klass);
    env->DeleteLocalRef(buf2);
    return ret;
}

int shell_read(char *buf, int4 bufsize) {
    Tracer T("shell_read");
    JNIEnv *env = getJniEnv();
    jclass klass = env->GetObjectClass(g_activity);
    jmethodID mid = env->GetMethodID(klass, "shell_read", "([B)I");
    jbyteArray buf2 = env->NewByteArray(bufsize);
    int n = env->CallIntMethod(g_activity, mid, buf2);
    if (n > 0)
        env->GetByteArrayRegion(buf2, 0, n, (jbyte *) buf);
    // Delete local references
    env->DeleteLocalRef(klass);
    env->DeleteLocalRef(buf2);
    return n;
}

int shell_get_acceleration(double *x, double *y, double *z) {
    Tracer T("shell_get_acceleration");
    JNIEnv *env = getJniEnv();
    jclass klass1 = env->FindClass("com/thomasokken/free42/DoubleHolder");
    jmethodID mid = env->GetMethodID(klass1, "<init>", "()V");
    jobject x_h = env->NewObject(klass1, mid);
    jobject y_h = env->NewObject(klass1, mid);
    jobject z_h = env->NewObject(klass1, mid);
    jclass klass2 = env->GetObjectClass(g_activity);
    mid = env->GetMethodID(klass2, "shell_get_acceleration", "(Lcom/thomasokken/free42/DoubleHolder;Lcom/thomasokken/free42/DoubleHolder;Lcom/thomasokken/free42/DoubleHolder;)I");
    int ret = env->CallIntMethod(g_activity, mid, x_h, y_h, z_h);
    jfieldID fid = env->GetFieldID(klass1, "value", "D");
    *x = env->GetDoubleField(x_h, fid);
    *y = env->GetDoubleField(y_h, fid);
    *z = env->GetDoubleField(z_h, fid);
    env->DeleteLocalRef(klass1);
    env->DeleteLocalRef(x_h);
    env->DeleteLocalRef(y_h);
    env->DeleteLocalRef(z_h);
    env->DeleteLocalRef(klass2);
    return ret;
}

int shell_get_location(double *lat, double *lon, double *lat_lon_acc,
                                            double *elev, double *elev_acc) {
    Tracer T("shell_get_location");
    JNIEnv *env = getJniEnv();
    jclass klass1 = env->FindClass("com/thomasokken/free42/DoubleHolder");
    jmethodID mid = env->GetMethodID(klass1, "<init>", "()V");
    jobject lat_h = env->NewObject(klass1, mid);
    jobject lon_h = env->NewObject(klass1, mid);
    jobject lat_lon_acc_h = env->NewObject(klass1, mid);
    jobject elev_h = env->NewObject(klass1, mid);
    jobject elev_acc_h = env->NewObject(klass1, mid);
    jclass klass2 = env->GetObjectClass(g_activity);
    mid = env->GetMethodID(klass2, "shell_get_location", "(Lcom/thomasokken/free42/DoubleHolder;Lcom/thomasokken/free42/DoubleHolder;Lcom/thomasokken/free42/DoubleHolder;Lcom/thomasokken/free42/DoubleHolder;Lcom/thomasokken/free42/DoubleHolder;)I");
    int ret = env->CallIntMethod(g_activity, mid, lat_h, lon_h, lat_lon_acc_h, elev_h, elev_acc_h);
    jfieldID fid = env->GetFieldID(klass1, "value", "D");
    *lat = env->GetDoubleField(lat_h, fid);
    *lon = env->GetDoubleField(lon_h, fid);
    *lat_lon_acc = env->GetDoubleField(lat_lon_acc_h, fid);
    *elev = env->GetDoubleField(elev_h, fid);
    *elev_acc = env->GetDoubleField(elev_acc_h, fid);
    env->DeleteLocalRef(klass1);
    env->DeleteLocalRef(lat_h);
    env->DeleteLocalRef(lon_h);
    env->DeleteLocalRef(lat_lon_acc_h);
    env->DeleteLocalRef(elev_h);
    env->DeleteLocalRef(elev_acc_h);
    env->DeleteLocalRef(klass2);
    return ret;
}

int shell_get_heading(double *mag_heading, double *true_heading, double *acc,
                                            double *x, double *y, double *z) {
    Tracer T("shell_get_heading");
    JNIEnv *env = getJniEnv();
    jclass klass1 = env->FindClass("com/thomasokken/free42/DoubleHolder");
    jmethodID mid = env->GetMethodID(klass1, "<init>", "()V");
    jobject mag_heading_h = env->NewObject(klass1, mid);
    jobject true_heading_h = env->NewObject(klass1, mid);
    jobject acc_h = env->NewObject(klass1, mid);
    jobject x_h = env->NewObject(klass1, mid);
    jobject y_h = env->NewObject(klass1, mid);
    jobject z_h = env->NewObject(klass1, mid);
    jclass klass2 = env->GetObjectClass(g_activity);
    mid = env->GetMethodID(klass2, "shell_get_heading", "(Lcom/thomasokken/free42/DoubleHolder;Lcom/thomasokken/free42/DoubleHolder;Lcom/thomasokken/free42/DoubleHolder;Lcom/thomasokken/free42/DoubleHolder;Lcom/thomasokken/free42/DoubleHolder;Lcom/thomasokken/free42/DoubleHolder;)I");
    int ret = env->CallIntMethod(g_activity, mid, mag_heading_h, true_heading_h, acc_h, x_h, y_h, z_h);
    jfieldID fid = env->GetFieldID(klass1, "value", "D");
    *mag_heading = env->GetDoubleField(mag_heading_h, fid);
    *true_heading = env->GetDoubleField(true_heading_h, fid);
    *acc = env->GetDoubleField(acc_h, fid);
    *x = env->GetDoubleField(x_h, fid);
    *y = env->GetDoubleField(y_h, fid);
    *z = env->GetDoubleField(z_h, fid);
    env->DeleteLocalRef(klass1);
    env->DeleteLocalRef(mag_heading_h);
    env->DeleteLocalRef(true_heading_h);
    env->DeleteLocalRef(acc_h);
    env->DeleteLocalRef(x_h);
    env->DeleteLocalRef(y_h);
    env->DeleteLocalRef(z_h);
    env->DeleteLocalRef(klass2);
    return ret;
}

void shell_get_time_date(uint4 *time, uint4 *date, int *weekday) {
    Tracer T("shell_get_time_date");
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm tms;
    localtime_r(&tv.tv_sec, &tms);
    if (time != NULL)
        *time = ((tms.tm_hour * 100 + tms.tm_min) * 100 + tms.tm_sec) * 100 + tv.tv_usec / 10000;
    if (date != NULL)
        *date = ((tms.tm_year + 1900) * 100 + tms.tm_mon + 1) * 100 + tms.tm_mday;
    if (weekday != NULL)
        *weekday = tms.tm_wday;
}

void shell_log(const char *message) {
    JNIEnv *env = getJniEnv();
    jclass klass = env->GetObjectClass(g_activity);
    jmethodID mid = env->GetMethodID(klass, "shell_log", "(Ljava/lang/String;)V");
    jstring s = env->NewStringUTF(message);
    env->CallVoidMethod(g_activity, mid, s);
    // Delete local references
    env->DeleteLocalRef(klass);
    env->DeleteLocalRef(s);
}

void shell_logprintf(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    char buf[1000];
    vsprintf(buf, format, ap);
    shell_log(buf);
    va_end(ap);
}
