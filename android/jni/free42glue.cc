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
    // TODO: Release the array?
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
    jmethodID mid = g_env->GetMethodID(klass, "shell_wants_cpu", "(V)I");
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
    jmethodID mid = g_env->GetMethodID(klass, "shell_get_mem", "(V)I");
    return g_env->CallIntMethod(g_activity, mid);
}

int shell_low_battery() {
    jclass klass = g_env->GetObjectClass(g_activity);
    jmethodID mid = g_env->GetMethodID(klass, "shell_low_battery", "(V)I");
    return g_env->CallIntMethod(g_activity, mid);
}

void shell_powerdown() {
    jclass klass = g_env->GetObjectClass(g_activity);
    jmethodID mid = g_env->GetMethodID(klass, "shell_powerdown", "(V)V");
    g_env->CallVoidMethod(g_activity, mid);
}

double shell_random_seed() {
    jclass klass = g_env->GetObjectClass(g_activity);
    jmethodID mid = g_env->GetMethodID(klass, "shell_random_seed", "(V)D");
    return g_env->CallDoubleMethod(g_activity, mid);
}

uint4 shell_milliseconds() {
    jclass klass = g_env->GetObjectClass(g_activity);
    jmethodID mid = g_env->GetMethodID(klass, "shell_milliseconds", "(V)I");
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
