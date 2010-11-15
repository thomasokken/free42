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
#include <sys/time.h>
#include <time.h>
#include "shell.h"
#include "core_main.h"
#include "core_display.h"

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
Java_com_thomasokken_free42_Free42Activity_core_1init(JNIEnv *env, jobject thiz, jint read_state, jint version) {
    core_init(read_state, version);
}

extern "C" void
Java_com_thomasokken_free42_Free42Activity_core_1quit(JNIEnv *env, jobject thiz) {
    core_quit();
}

extern "C" void
Java_com_thomasokken_free42_Free42Activity_core_1repaint_1display(JNIEnv *env, jobject thiz) {
    core_repaint_display();
}

extern "C" jboolean
Java_com_thomasokken_free42_Free42Activity_core_1menu(JNIEnv *env, jobject thiz) {
    return core_menu();
}

extern "C" jboolean
Java_com_thomasokken_free42_Free42Activity_core_1alpha_1menu(JNIEnv *env, jobject thiz) {
    return core_alpha_menu();
}

extern "C" jboolean
Java_com_thomasokken_free42_Free42Activity_core_1hex_1menu(JNIEnv *env, jobject thiz) {
    return core_hex_menu();
}

extern "C" jboolean
Java_com_thomasokken_free42_Free42Activity_core_1keydown(JNIEnv *env, jobject thiz, jint key, jobject enqueued, jobject repeat) {
    int enq, rep;
    jboolean ret = core_keydown(key, &enq, &rep);
    jclass klass = env->GetObjectClass(enqueued);
    jfieldID fid = env->GetFieldID(klass, "value", "I");
    env->SetIntField(enqueued, fid, enq);
    klass = env->GetObjectClass(repeat);
    fid = env->GetFieldID(klass, "value", "I");
    env->SetIntField(repeat, fid, rep);
    return ret;
}

extern "C" jint
Java_com_thomasokken_free42_Free42Activity_core_1repeat(JNIEnv *env, jobject thiz) {
    return core_repeat();
}

extern "C" void
Java_com_thomasokken_free42_Free42Activity_core_1keytimeout1(JNIEnv *env, jobject thiz) {
    core_keytimeout1();
}

extern "C" void
Java_com_thomasokken_free42_Free42Activity_core_1keytimeout2(JNIEnv *env, jobject thiz) {
    core_keytimeout2();
}

extern "C" jboolean
Java_com_thomasokken_free42_Free42Activity_core_1timeout3(JNIEnv *env, jobject thiz, jint repaint) {
    return core_timeout3(repaint);
}

extern "C" jboolean
Java_com_thomasokken_free42_Free42Activity_core_1keyup(JNIEnv *env, jobject thiz) {
    return core_keyup();
}

extern "C" jboolean
Java_com_thomasokken_free42_Free42Activity_core_1allows_1powerdown(JNIEnv *env, jobject thiz, jobject want_cpu) {
    int wantCpu, *wantCpuPtr;
    jfieldID wantCpuFid;
    if (want_cpu == NULL) {
	wantCpuPtr = NULL;
    } else {
	jclass klass = env->GetObjectClass(want_cpu);
	wantCpuFid = env->GetFieldID(klass, "value", "I");
	wantCpu = env->GetIntField(want_cpu, wantCpuFid);
	wantCpuPtr = &wantCpu;
    }
    jboolean ret = core_allows_powerdown(wantCpuPtr);
    if (want_cpu != NULL)
	env->SetIntField(want_cpu, wantCpuFid, wantCpu);
    return ret;
}

extern "C" jboolean
Java_com_thomasokken_free42_Free42Activity_core_1powercycle(JNIEnv *env, jobject thiz) {
    return core_powercycle();
}

extern "C" jint
Java_com_thomasokken_free42_Free42Activity_core_1list_1programs(JNIEnv *env, jarray thiz, jbyteArray buf) {
    int bufsize = env->GetArrayLength(buf);
    char *cbuf = (char *) malloc(bufsize);
    int ret = core_list_programs(cbuf, bufsize);
    env->SetByteArrayRegion(buf, 0, bufsize, (const jbyte *) cbuf);
    free(cbuf);
    return ret;
}

extern "C" jint
Java_com_thomasokken_free42_Free42Activity_core_1program_1size(JNIEnv *env, jobject thiz, jint prgm_index) {
    return core_program_size(prgm_index);
}

extern "C" jboolean
Java_com_thomasokken_free42_Free42Activity_core_1export_1programs(JNIEnv *env, jobject thiz, jintArray indexes) {
    int count = env->GetArrayLength(indexes);
    int *indexes2 = (int *) malloc(count * sizeof(int));
    env->GetIntArrayRegion(indexes, 0, count, indexes2);
    jboolean ret = core_export_programs(count, indexes2, NULL);
    free(indexes2);
    return ret;
}

extern "C" void
Java_com_thomasokken_free42_Free42Activity_core_1import_1programs(JNIEnv *env, jobject thiz) {
    core_import_programs(NULL);
}

extern "C" jstring
Java_com_thomasokken_free42_Free42Activity_core_1copy(JNIEnv *env, jobject thiz) {
    char buf[100];
    core_copy(buf, 100);
    return env->NewStringUTF(buf);
}

extern "C" void
Java_com_thomasokken_free42_Free42Activity_core_1paste(JNIEnv *env, jobject thiz, jstring s) {
    const char *buf = env->GetStringUTFChars(s, NULL);
    core_paste(buf);
    env->ReleaseStringUTFChars(s, buf);
}

extern "C" void
Java_com_thomasokken_free42_Free42Activity_getCoreSettings(JNIEnv *env, jobject thiz, jobject settings) {
    jclass klass = env->GetObjectClass(settings);
    jfieldID fid = env->GetFieldID(klass, "matrix_singularmatrix", "B");
    env->SetBooleanField(settings, fid, core_settings.matrix_singularmatrix);
    fid = env->GetFieldID(klass, "matrix_outofrange", "B");
    env->SetBooleanField(settings, fid, core_settings.matrix_outofrange);
    fid = env->GetFieldID(klass, "raw_text", "B");
    env->SetBooleanField(settings, fid, core_settings.raw_text);
    fid = env->GetFieldID(klass, "auto_repeat", "B");
    env->SetBooleanField(settings, fid, core_settings.auto_repeat);
    fid = env->GetFieldID(klass, "enable_ext_copan", "B");
    env->SetBooleanField(settings, fid, core_settings.enable_ext_copan);
    fid = env->GetFieldID(klass, "enable_ext_bigstack", "B");
    env->SetBooleanField(settings, fid, core_settings.enable_ext_bigstack);
    fid = env->GetFieldID(klass, "enable_ext_accel", "B");
    env->SetBooleanField(settings, fid, core_settings.enable_ext_accel);
    fid = env->GetFieldID(klass, "enable_ext_locat", "B");
    env->SetBooleanField(settings, fid, core_settings.enable_ext_locat);
    fid = env->GetFieldID(klass, "enable_ext_heading", "B");
    env->SetBooleanField(settings, fid, core_settings.enable_ext_heading);
    fid = env->GetFieldID(klass, "enable_ext_time", "B");
    env->SetBooleanField(settings, fid, core_settings.enable_ext_time);
}

extern "C" void
Java_com_thomasokken_free42_Free42Activity_putCoreSettings(JNIEnv *env, jobject thiz, jobject settings) {
    jclass klass = env->GetObjectClass(settings);
    jfieldID fid = env->GetFieldID(klass, "matrix_singularmatrix", "B");
    core_settings.matrix_singularmatrix = env->GetBooleanField(settings, fid);
    fid = env->GetFieldID(klass, "matrix_outofrange", "B");
    core_settings.matrix_outofrange = env->GetBooleanField(settings, fid);
    fid = env->GetFieldID(klass, "raw_text", "B");
    core_settings.raw_text = env->GetBooleanField(settings, fid);
    fid = env->GetFieldID(klass, "auto_repeat", "B");
    core_settings.auto_repeat = env->GetBooleanField(settings, fid);
    fid = env->GetFieldID(klass, "enable_ext_copan", "B");
    core_settings.enable_ext_copan = env->GetBooleanField(settings, fid);
    fid = env->GetFieldID(klass, "enable_ext_bigstack", "B");
    core_settings.enable_ext_bigstack = env->GetBooleanField(settings, fid);
    fid = env->GetFieldID(klass, "enable_ext_accel", "B");
    core_settings.enable_ext_accel = env->GetBooleanField(settings, fid);
    fid = env->GetFieldID(klass, "enable_ext_locat", "B");
    core_settings.enable_ext_locat = env->GetBooleanField(settings, fid);
    fid = env->GetFieldID(klass, "enable_ext_heading", "B");
    core_settings.enable_ext_heading = env->GetBooleanField(settings, fid);
    fid = env->GetFieldID(klass, "enable_ext_time", "B");
    core_settings.enable_ext_time = env->GetBooleanField(settings, fid);
}

extern "C" void
Java_com_thomasokken_free42_Free42Activity_redisplay(JNIEnv *env, jobject thiz) {
    redisplay();
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

void shell_beeper(int frequency, int duration) {
    jclass klass = g_env->GetObjectClass(g_activity);
    jmethodID mid = g_env->GetMethodID(klass, "shell_beeper", "(II)V");
    g_env->CallVoidMethod(g_activity, mid, frequency, duration);
}

void shell_annunciators(int updn, int shf, int prt, int run, int g, int rad) {
    jclass klass = g_env->GetObjectClass(g_activity);
    jmethodID mid = g_env->GetMethodID(klass, "shell_annunciators", "(IIIIII)V");
    g_env->CallVoidMethod(g_activity, mid, updn, shf, prt, run, g, rad);
}

int shell_wants_cpu() {
    // TODO: Cheat code like the PalmOS ARM version; don't want to call
    // into the Java environment after every single program line.
    jclass klass = g_env->GetObjectClass(g_activity);
    jmethodID mid = g_env->GetMethodID(klass, "shell_wants_cpu", "()I");
    return g_env->CallIntMethod(g_activity, mid);
}

void shell_delay(int duration) {
    jclass klass = g_env->GetObjectClass(g_activity);
    jmethodID mid = g_env->GetMethodID(klass, "shell_delay", "(I)V");
    g_env->CallVoidMethod(g_activity, mid, duration);
}

void shell_request_timeout3(int delay) {
    jclass klass = g_env->GetObjectClass(g_activity);
    jmethodID mid = g_env->GetMethodID(klass, "shell_request_timeout3", "(I)V");
    g_env->CallVoidMethod(g_activity, mid, delay);
}

int shell_read_saved_state(void *buf, int4 bufsize) {
    jclass klass = g_env->GetObjectClass(g_activity);
    jmethodID mid = g_env->GetMethodID(klass, "shell_read_saved_state", "([B)I");
    jbyteArray buf2 = g_env->NewByteArray(bufsize);
    int n = g_env->CallIntMethod(g_activity, mid, buf2);
    if (n > 0)
	g_env->GetByteArrayRegion(buf2, 0, n, (jbyte *) buf);
    return n;
    // TODO: Release the array?
}

bool shell_write_saved_state(const void *buf, int4 bufsize) {
    jclass klass = g_env->GetObjectClass(g_activity);
    jmethodID mid = g_env->GetMethodID(klass, "shell_write_saved_state", "([B)I");
    jbyteArray buf2 = g_env->NewByteArray(bufsize);
    g_env->SetByteArrayRegion(buf2, 0, bufsize, (const jbyte *) buf);
    return g_env->CallIntMethod(g_activity, mid, buf2);
    // TODO: Release the array?
}

unsigned int shell_get_mem() {
    jclass klass = g_env->GetObjectClass(g_activity);
    jmethodID mid = g_env->GetMethodID(klass, "shell_get_mem", "()I");
    return g_env->CallIntMethod(g_activity, mid);
}

int shell_low_battery() {
    jclass klass = g_env->GetObjectClass(g_activity);
    jmethodID mid = g_env->GetMethodID(klass, "shell_low_battery", "()I");
    return g_env->CallIntMethod(g_activity, mid);
}

void shell_powerdown() {
    jclass klass = g_env->GetObjectClass(g_activity);
    jmethodID mid = g_env->GetMethodID(klass, "shell_powerdown", "()V");
    g_env->CallVoidMethod(g_activity, mid);
}

double shell_random_seed() {
    jclass klass = g_env->GetObjectClass(g_activity);
    jmethodID mid = g_env->GetMethodID(klass, "shell_random_seed", "()D");
    return g_env->CallDoubleMethod(g_activity, mid);
}

uint4 shell_milliseconds() {
    jclass klass = g_env->GetObjectClass(g_activity);
    jmethodID mid = g_env->GetMethodID(klass, "shell_milliseconds", "()I");
    return g_env->CallIntMethod(g_activity, mid);
}

void shell_print(const char *text, int length,
		 const char *bits, int bytesperline,
		 int x, int y, int width, int height) {
    jclass klass = g_env->GetObjectClass(g_activity);
    jmethodID mid = g_env->GetMethodID(klass, "shell_print", "([B[BIIIII)V");
    jbyteArray text2 = g_env->NewByteArray(length);
    g_env->SetByteArrayRegion(text2, 0, length, (const jbyte *) text);
    int bitmapsize = bytesperline * height;
    jbyteArray bits2 = g_env->NewByteArray(bitmapsize);
    g_env->SetByteArrayRegion(bits2, 0, bitmapsize, (const jbyte *) bits);
    g_env->CallVoidMethod(g_activity, mid, text2);
    // TODO: Release the arrays?
}

int shell_write(const char *buf, int4 bufsize) {
    jclass klass = g_env->GetObjectClass(g_activity);
    jmethodID mid = g_env->GetMethodID(klass, "shell_write", "([B)I");
    jbyteArray buf2 = g_env->NewByteArray(bufsize);
    g_env->SetByteArrayRegion(buf2, 0, bufsize, (const jbyte *) buf);
    return g_env->CallIntMethod(g_activity, mid, buf2);
    // TODO: Release the array?
}

int shell_read(char *buf, int4 bufsize) {
    jclass klass = g_env->GetObjectClass(g_activity);
    jmethodID mid = g_env->GetMethodID(klass, "shell_read", "([B)I");
    jbyteArray buf2 = g_env->NewByteArray(bufsize);
    int n = g_env->CallIntMethod(g_activity, mid, buf2);
    if (n > 0)
	g_env->GetByteArrayRegion(buf2, 0, n, (jbyte *) buf);
    return n;
    // TODO: Release the array?
}

shell_bcd_table_struct *shell_get_bcd_table() {
    return NULL;
}

shell_bcd_table_struct *shell_put_bcd_table(shell_bcd_table_struct *bcdtab,
					    uint4 size) {
    return bcdtab;
}

void shell_release_bcd_table(shell_bcd_table_struct *bcdtab) {
    free(bcdtab);
}

void shell_get_time_date(uint4 *time, uint4 *date, int *weekday) {
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
