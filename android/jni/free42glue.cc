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
#include <stdio.h>
#include <stdarg.h>
#include "shell.h"
#include "core_main.h"
#include "core_display.h"


/*
static int level = 0;
void shell_logprintf(const char *format, ...);
class Tracer {
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
};
*/


static jobject g_activity;
static JavaVM *vm;

extern "C" void
Java_com_thomasokken_free42_Free42Activity_nativeInit(JNIEnv *env, jobject thiz) {
    /* I'm hanging on to the Free42Activity pointer so I can use it to call
     * Java methods. The JNIEnv pointer is thread-local, so I don't cache it;
     * use the getJniEnv() function, defined below, to retrieve it when needed.
     */
    g_activity = thiz;
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


/**********************************************************/
/* Here followeth the stubs for the core_main.h interface */
/**********************************************************/

extern "C" void
Java_com_thomasokken_free42_Free42Activity_core_1init(JNIEnv *env, jobject thiz, jint read_state, jint version) {
    //Tracer T("core_init");
    core_init(read_state, version);
}

extern "C" void
Java_com_thomasokken_free42_Free42Activity_core_1quit(JNIEnv *env, jobject thiz) {
    //Tracer T("core_quit");
    core_quit();
}

extern "C" void
Java_com_thomasokken_free42_Free42Activity_core_1repaint_1display(JNIEnv *env, jobject thiz) {
    //Tracer T("core_repaint_display");
    core_repaint_display();
}

extern "C" jboolean
Java_com_thomasokken_free42_Free42Activity_core_1menu(JNIEnv *env, jobject thiz) {
    //Tracer T("core_menu");
    return core_menu();
}

extern "C" jboolean
Java_com_thomasokken_free42_Free42Activity_core_1alpha_1menu(JNIEnv *env, jobject thiz) {
    //Tracer T("core_alpha_menu");
    return core_alpha_menu();
}

extern "C" jboolean
Java_com_thomasokken_free42_Free42Activity_core_1hex_1menu(JNIEnv *env, jobject thiz) {
    //Tracer T("core_hex_menu");
    return core_hex_menu();
}

extern "C" jboolean
Java_com_thomasokken_free42_Free42Activity_core_1keydown(JNIEnv *env, jobject thiz, jint key, jobject enqueued, jobject repeat) {
    //Tracer T("core_keydown");
    int enq, rep;
    jboolean ret = core_keydown(key, &enq, &rep);
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
    //Tracer T("core_repeat");
    return core_repeat();
}

extern "C" void
Java_com_thomasokken_free42_Free42Activity_core_1keytimeout1(JNIEnv *env, jobject thiz) {
    //Tracer T("core_keytimeout1");
    core_keytimeout1();
}

extern "C" void
Java_com_thomasokken_free42_Free42Activity_core_1keytimeout2(JNIEnv *env, jobject thiz) {
    //Tracer T("core_keytimeout2");
    core_keytimeout2();
}

extern "C" jboolean
Java_com_thomasokken_free42_Free42Activity_core_1timeout3(JNIEnv *env, jobject thiz, jint repaint) {
    //Tracer T("core_timeout3");
    return core_timeout3(repaint);
}

extern "C" jboolean
Java_com_thomasokken_free42_Free42Activity_core_1keyup(JNIEnv *env, jobject thiz) {
    //Tracer T("core_keyup");
    return core_keyup();
}

extern "C" jboolean
Java_com_thomasokken_free42_Free42Activity_core_1allows_1powerdown(JNIEnv *env, jobject thiz, jobject want_cpu) {
    //Tracer T("core_allows_powerdown");
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
    //Tracer T("core_powercycle");
    return core_powercycle();
}

extern "C" jint
Java_com_thomasokken_free42_Free42Activity_core_1list_1programs(JNIEnv *env, jarray thiz, jbyteArray buf) {
    //Tracer T("core_list_programs");
    int bufsize = env->GetArrayLength(buf);
    char *cbuf = (char *) malloc(bufsize);
    int ret = core_list_programs(cbuf, bufsize);
    env->SetByteArrayRegion(buf, 0, bufsize, (const jbyte *) cbuf);
    free(cbuf);
    return ret;
}

extern "C" jint
Java_com_thomasokken_free42_Free42Activity_core_1program_1size(JNIEnv *env, jobject thiz, jint prgm_index) {
    //Tracer T("core_program_size");
    return core_program_size(prgm_index);
}

extern "C" jboolean
Java_com_thomasokken_free42_Free42Activity_core_1export_1programs(JNIEnv *env, jobject thiz, jintArray indexes) {
    //Tracer T("core_export_programs");
    int count = env->GetArrayLength(indexes);
    int *indexes2 = (int *) malloc(count * sizeof(int));
    env->GetIntArrayRegion(indexes, 0, count, indexes2);
    jboolean ret = core_export_programs(count, indexes2, NULL);
    free(indexes2);
    return ret;
}

extern "C" void
Java_com_thomasokken_free42_Free42Activity_core_1import_1programs(JNIEnv *env, jobject thiz) {
    //Tracer T("core_import_programs");
    core_import_programs(NULL);
}

extern "C" jstring
Java_com_thomasokken_free42_Free42Activity_core_1copy(JNIEnv *env, jobject thiz) {
    //Tracer T("core_copy");
    char buf[100];
    core_copy(buf, 100);
    return env->NewStringUTF(buf);
}

extern "C" void
Java_com_thomasokken_free42_Free42Activity_core_1paste(JNIEnv *env, jobject thiz, jstring s) {
    //Tracer T("core_paste");
    const char *buf = env->GetStringUTFChars(s, NULL);
    core_paste(buf);
    env->ReleaseStringUTFChars(s, buf);
}

extern "C" void
Java_com_thomasokken_free42_Free42Activity_getCoreSettings(JNIEnv *env, jobject thiz, jobject settings) {
    //Tracer T("getCoreSettings");
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
    //Tracer T("putCoreSettings");
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
    //Tracer T("redisplay");
    redisplay();
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
    JNIEnv *env = getJniEnv();
    jclass klass = env->GetObjectClass(g_activity);
    jmethodID mid = env->GetMethodID(klass, "shell_beeper", "(II)V");
    env->CallVoidMethod(g_activity, mid, frequency, duration);
    // Delete local references
    env->DeleteLocalRef(klass);
}

void shell_annunciators(int updn, int shf, int prt, int run, int g, int rad) {
    JNIEnv *env = getJniEnv();
    jclass klass = env->GetObjectClass(g_activity);
    jmethodID mid = env->GetMethodID(klass, "shell_annunciators", "(IIIIII)V");
    env->CallVoidMethod(g_activity, mid, updn, shf, prt, run, g, rad);
    // Delete local references
    env->DeleteLocalRef(klass);
}

int shell_wants_cpu() {
    // I'm cheating a bit here: the core calls this function after every
    // program line, and the overhead from all those JNI calls adds up.
    // By only calling the shell on every 10th call, I reduce this overhead
    // by 90%; the only downside is that after the user presses R/S, the
    // program may keep running 9 lines too far. Oh, well.
    static int n = 0;
    if (++n < 10)
	return 0;
    n = 0;

    JNIEnv *env = getJniEnv();
    jclass klass = env->GetObjectClass(g_activity);
    jmethodID mid = env->GetMethodID(klass, "shell_wants_cpu", "()I");
    int ret = env->CallIntMethod(g_activity, mid);
    // Delete local references
    env->DeleteLocalRef(klass);
    return ret;
}

void shell_delay(int duration) {
    JNIEnv *env = getJniEnv();
    jclass klass = env->GetObjectClass(g_activity);
    jmethodID mid = env->GetMethodID(klass, "shell_delay", "(I)V");
    env->CallVoidMethod(g_activity, mid, duration);
    // Delete local references
    env->DeleteLocalRef(klass);
}

void shell_request_timeout3(int delay) {
    JNIEnv *env = getJniEnv();
    jclass klass = env->GetObjectClass(g_activity);
    jmethodID mid = env->GetMethodID(klass, "shell_request_timeout3", "(I)V");
    env->CallVoidMethod(g_activity, mid, delay);
    // Delete local references
    env->DeleteLocalRef(klass);
}

int shell_read_saved_state(void *buf, int4 bufsize) {
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
    JNIEnv *env = getJniEnv();
    jclass klass = env->GetObjectClass(g_activity);
    jmethodID mid = env->GetMethodID(klass, "shell_get_mem", "()I");
    unsigned int ret = env->CallIntMethod(g_activity, mid);
    // Delete local references
    env->DeleteLocalRef(klass);
    return ret;
}

int shell_low_battery() {
    JNIEnv *env = getJniEnv();
    jclass klass = env->GetObjectClass(g_activity);
    jmethodID mid = env->GetMethodID(klass, "shell_low_battery", "()I");
    int ret = env->CallIntMethod(g_activity, mid);
    // Delete local references
    env->DeleteLocalRef(klass);
    return ret;
}

void shell_powerdown() {
    JNIEnv *env = getJniEnv();
    jclass klass = env->GetObjectClass(g_activity);
    jmethodID mid = env->GetMethodID(klass, "shell_powerdown", "()V");
    env->CallVoidMethod(g_activity, mid);
    // Delete local references
    env->DeleteLocalRef(klass);
}

double shell_random_seed() {
    JNIEnv *env = getJniEnv();
    jclass klass = env->GetObjectClass(g_activity);
    jmethodID mid = env->GetMethodID(klass, "shell_random_seed", "()D");
    double ret = env->CallDoubleMethod(g_activity, mid);
    // Delete local references
    env->DeleteLocalRef(klass);
    return ret;
}

uint4 shell_milliseconds() {
    JNIEnv *env = getJniEnv();
    jclass klass = env->GetObjectClass(g_activity);
    jmethodID mid = env->GetMethodID(klass, "shell_milliseconds", "()I");
    uint4 ret = env->CallIntMethod(g_activity, mid);
    // Delete local references
    env->DeleteLocalRef(klass);
    return ret;
}

void shell_print(const char *text, int length,
		 const char *bits, int bytesperline,
		 int x, int y, int width, int height) {
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

void shell_logprintf(const char *format, ...) {
    va_list ap;
    va_start(ap, format);

    JNIEnv *env = getJniEnv();
    jclass klass = env->GetObjectClass(g_activity);
    jmethodID mid = env->GetMethodID(klass, "shell_log", "(Ljava/lang/String;)V");
    char buf[1000];
    vsprintf(buf, format, ap);
    jstring s = env->NewStringUTF(buf);
    env->CallVoidMethod(g_activity, mid, s);
    // Delete local references
    env->DeleteLocalRef(klass);
    env->DeleteLocalRef(s);

    va_end(ap);
}
