/*****************************************************************************
 * Free42 -- a free HP-42S calculator clone
 * Copyright (C) 2004-2006  Thomas Okken
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *****************************************************************************/

#include <stdlib.h>

#include "shell.h"
#include "shell2.h"
#include "shell_spool.h"
#include "myglue.h"
#include "filesys.h"
#include "core_main.h"
#include "shell.rcp.h"
#include "skin2prc.h"

static FileHand statefile = NULL;

static DmOpenRef printdb;
static MemHandle printrec;
static DmOpenRef bcddb;
static MemHandle bcdrec;

#if 0
static void log(const char *message) {
    char basename[FILENAMELEN];
    char *newline = "\n";
    fsa_obj *dir, *logfile;
    int err = fsa_resolve("Free42:/log.txt", &dir, 1, basename);
    err = fsa_create(dir, basename, &logfile);
    fsa_release(dir);
    err = fsa_open(logfile, FSA_MODE_READWRITE);
    err = fsa_seek(logfile, FSA_SEEK_END, 0);
    uint4 len = (uint4) StrLen(message);
    err = fsa_write(logfile, message, &len);
    len = 1;
    err = fsa_write(logfile, newline, &len);
    fsa_release(logfile);
}
#endif

static void open_printout() {
    LocalID id = DmFindDatabase(0, "Free42Print");
    if (id == 0) {
	Err err = DmCreateDatabase(0, "Free42Print", 'Fk42', 'Prnt', false);
	if (err != errNone)
	    ErrFatalDisplayIf(true, "Can't create printout database.");
	id = DmFindDatabase(0, "Free42Print");
    }
    printdb = DmOpenDatabase(0, id, dmModeReadWrite);
    if (printdb == 0)
	ErrFatalDisplayIf(true, "Can't open printout database.");
    if (DmNumRecords(printdb) == 0) {
	UInt16 index = 0;
	printrec = DmNewRecord(printdb, &index, PRINT_SIZE + 2 * sizeof(int4));
	if (printrec == 0)
	    ErrFatalDisplayIf(true, "Can't initialize printout database.");
	printout = (char *) MemHandleLock(printrec);
	printout_top = 0;
	printout_bottom = 0;
	printout_pos = 0;
    } else {
	int length;
	printrec = DmGetRecord(printdb, 0);
	printout = (char *) MemHandleLock(printrec);
	printout_top = *(int4 *) (printout + PRINT_SIZE);
	printout_bottom = *(int4 *) (printout + PRINT_SIZE + sizeof(int4));
	length = printout_bottom - printout_top;
	if (length < 0)
	    length += PRINT_LINES;
	printout_pos = length < 160 ? 0 : length - 160;
    }
}

static void close_printout() {
    if (printdb != 0) {
	if (printrec != 0) {
	    DmWrite(printout, PRINT_SIZE, &printout_top, sizeof(int4));
	    DmWrite(printout, PRINT_SIZE + sizeof(int4), &printout_bottom,
							 sizeof(int4));
	    MemHandleUnlock(printrec);
	    DmReleaseRecord(printdb, 0, false);
	}
	DmCloseDatabase(printdb);
    }
}

void shell_blitter(const char *bits, int bytesperline,
	       int x, int y, int width, int height) {
    /* Callback for emulator core; it calls this to blit stuff
     * to the display
     */
    char *mybits;
    UInt16 rowbytes;
    int X, Y, XX, YY, XXmin, XXmax, YYmin, YYmax;
    int xs = skin->display_xscale;
    int ys = skin->display_yscale;

    mybits = (char *) MyGlueBmpGetBits(disp_bitmap);
    BmpGlueGetDimensions(disp_bitmap, NULL, NULL, &rowbytes);

    YYmin = y * ys;
    YYmax = YYmin + ys;
    for (Y = y; Y < y + height; Y++) {
	XXmin = x * xs;
	XXmax = XXmin + xs;
	for (X = x; X < x + width; X++) {
	    int pix = bits[Y * bytesperline + (X >> 3)] & 1 << (X & 7);
	    UInt32 off = YYmin * rowbytes;
	    for (YY = YYmin; YY < YYmax; YY++) {
		for (XX = XXmin; XX < XXmax; XX++)
		    if (pix == 0)
			mybits[off + (XX >> 3)] &= ~(128 >> (XX & 7));
		    else
			mybits[off + (XX >> 3)] |= 128 >> (XX & 7);
		off += rowbytes;
	    }
	    XXmin += xs;
	    XXmax += xs;
	}
	YYmin += ys;
	YYmax += ys;
    }

    if (can_draw && FrmGetActiveFormID() == calcform_id) {
	set_colors(&skin->display_bg, &skin->display_fg);
	set_coord_sys();
	WinDrawBitmap(disp_bitmap, skin->display_x, skin->display_y);
	restore_colors();
	restore_coord_sys();
	if (softkey != 0)
	    draw_softkey(1);
    }
}

void shell_beeper(int frequency, int duration) {
    Err err;
    SndCommandType cmd;
    cmd.cmd = sndCmdFreqDurationAmp;
    cmd.param1 = frequency;
    cmd.param2 = duration;
    cmd.param3 = state.soundVolume;
    err = SndDoCmd(NULL, &cmd, false);
}

void shell_annunciators(int updn, int shf, int prt, int run, int g, int rad) {
    if (can_draw && FrmGetActiveFormID() == calcform_id) {
	if (updn != -1 && updn != updownAnn) {
	    updownAnn = updn;
	    repaint_annunciator(0);
	}
	if (shf != -1 && shf != shiftAnn) {
	    shiftAnn = shf;
	    repaint_annunciator(1);
	}
	if (prt != -1 && prt != printAnn) {
	    printAnn = prt;
	    repaint_annunciator(2);
	}
	if (run != -1 && run != runAnn) {
	    runAnn = run;
	    repaint_annunciator(3);
	}
	if (g != -1 && g != gAnn) {
	    gAnn = g;
	    repaint_annunciator(5);
	}
	if (rad != -1 && rad != radAnn) {
	    radAnn = rad;
	    repaint_annunciator(6);
	}
    } else {
	if (updn != -1)
	    updownAnn = updn;
	if (shf != -1)
	    shiftAnn = shf;
	if (prt != -1)
	    printAnn = prt;
	if (run != -1)
	    runAnn = run;
	if (g != -1)
	    gAnn = g;
	if (rad != -1)
	    radAnn = rad;
    }
}

int shell_wants_cpu() {
    return EvtEventAvail();
}

void shell_delay(int duration) {
    Int32 ticksToWait = ((Int32) duration) * SysTicksPerSecond() / 1000;
    SysTaskDelay(ticksToWait);
}

void shell_request_timeout3(int delay) {
    timeout3time = TimGetTicks() + ((Int32) delay) * SysTicksPerSecond() / 1000;
}

int4 shell_read_saved_state(void *buf, int4 bufsize) {
    if (statefile == NULL)
	return -1;
    else {
	Err error;
	Int32 n = FileRead(statefile, buf, 1, bufsize, &error);
	if (n != bufsize && error != errNone) {
	    FileClose(statefile);
	    statefile = NULL;
	    return -1;
	} else
	    return n;
    }
}

bool shell_write_saved_state(const void *buf, int4 nbytes) {
    if (statefile == NULL)
	return false;
    else {
	Err error;
	int4 n = FileWrite(statefile, buf, 1, nbytes, &error);
	if (n != nbytes || error != errNone) {
	    FileClose(statefile);
	    FileDelete(0, "Free42State");
	    statefile = NULL;
	    return false;
	} else
	    return true;
    }
}

Int32 shell_get_mem() {
    UInt32 freeBytes, maxBlock;
    MemHeapFreeBytes(0, &freeBytes, &maxBlock);
    return freeBytes;
}

int shell_low_battery() {
    UInt16 voltage, warningVoltage;
    Boolean pluggedIn;
    int battery;

    voltage = SysBatteryInfo(false, &warningVoltage, NULL, NULL,
			     NULL, &pluggedIn, NULL);
    battery = !pluggedIn && voltage < warningVoltage;

    if (battery != batteryAnn) {
	batteryAnn = battery;
	if (can_draw && FrmGetActiveFormID() == calcform_id)
	    repaint_annunciator(4);
    }

    return battery;
}

void shell_powerdown() {
    SysSleep(false, false);
}

double shell_random_seed() {
    UInt32 tmp = (((TimGetTicks() & 65535)) << 16) | (TimGetSeconds() & 65535);
    return tmp / 4294967296.0;
}

uint4 shell_milliseconds() {
    return (uint4) (((uint8) TimGetTicks()) * 1000 / SysTicksPerSecond());
}

void shell_print(const char *text, int length,
		 const char *bits, int bytesperline,
		 int x, int y, int width, int height) {
    int xx, yy;
    int4 oldlength, newlength;
    for (yy = 0; yy < height; yy++) {
	int4 Y = (printout_bottom + yy) % PRINT_LINES;
	char buf[PRINT_BYTESPERLINE];
	MemMove(buf, printout + Y * PRINT_BYTESPERLINE, PRINT_BYTESPERLINE);
	for (xx = 0; xx < 153; xx++) {
	    int bit;
	    if (xx < width) {
		char c = bits[(y + yy) * bytesperline + ((x + xx) >> 3)];
		bit = (c & (1 << ((x + xx) & 7))) != 0;
	    } else
		bit = 0;
	    if (bit)
		buf[xx >> 3] |= 128 >> (xx & 7);
	    else
		buf[xx >> 3] &= ~(128 >> (xx & 7));
	}
	DmWrite(printout, Y * PRINT_BYTESPERLINE, buf, PRINT_BYTESPERLINE);
    }

    oldlength = printout_bottom - printout_top;
    if (oldlength < 0)
	oldlength += PRINT_LINES;
    printout_bottom = (printout_bottom + height) % PRINT_LINES;
    newlength = oldlength + height;
    if (newlength >= PRINT_LINES) {
	printout_top = (printout_bottom + 1) % PRINT_LINES;
	newlength = PRINT_LINES - 1;
    }
    printout_pos = newlength < 160 ? 0 : newlength - 160;

    if (FrmGetActiveFormID() == printform_id) {
	FormType *form = FrmGetActiveForm();
	int index = FrmGetObjectIndex(form, printscroll_id);
	ScrollBarType *sb = (ScrollBarType *) FrmGetObjectPtr(form, index);
	SclSetScrollBar(sb, printout_pos, 0, printout_pos, 160);
	if (can_draw)
	    repaint_printout();
    }

    if (state.printerToMemo)
	print_to_memo(text, length);
    if (state.printerToTxtFile)
	print_to_txt(text, length);
    if (state.printerToGifFile)
	print_to_gif(bits, bytesperline, x, y, width, height);
}

shell_bcd_table_struct *shell_get_bcd_table() {
    LocalID id = DmFindDatabase(0, "Free42BcdConv");
    if (id == 0)
	return NULL;
    bcddb = DmOpenDatabase(0, id, dmModeReadWrite);
    if (bcddb == 0)
	return NULL;
    if (DmNumRecords(bcddb) == 0) {
	DmCloseDatabase(bcddb);
	bcddb = 0;
	return NULL;
    } else {
	bcdrec = DmGetRecord(bcddb, 0);
	return (shell_bcd_table_struct *) MemHandleLock(bcdrec);
    }
}

shell_bcd_table_struct *shell_put_bcd_table(shell_bcd_table_struct *bcdtab, uint4 size) {
    LocalID id;
    UInt16 index = 0;
    shell_bcd_table_struct *newtab;

    DmCreateDatabase(0, "Free42BcdConv", 'Fk42', 'BcdC', false);
    id = DmFindDatabase(0, "Free42BcdConv");
    if (id == 0)
	ErrFatalDisplayIf(true, "Can't create BCD conversion database.");
    bcddb = DmOpenDatabase(0, id, dmModeReadWrite);
    if (bcddb == 0)
	ErrFatalDisplayIf(true, "Can't open BCD conversion database.");
    bcdrec = DmNewRecord(bcddb, &index, size);
    if (bcdrec == 0)
	ErrFatalDisplayIf(true, "Can't initialize BCD conversion database.");
    newtab = (shell_bcd_table_struct *) MemHandleLock(bcdrec);
    DmWrite(newtab, 0, bcdtab, size);
    free(bcdtab);
    return newtab;
}

void shell_release_bcd_table(shell_bcd_table_struct *bcdtab) {
    if (bcddb != 0) {
	MemHandleUnlock(bcdrec);
	DmReleaseRecord(bcddb, 0, false);
	DmCloseDatabase(bcddb);
	bcddb = 0;
    }
}

UInt32 PilotMain(UInt16 cmd, void *pbp, UInt16 flags) {
    switch (cmd) {
	case sysAppLaunchCmdNormalLaunch: {
	    EventType event;
	    Err error;
	    UInt16 appCrd;
	    LocalID appDB;
	    DmSearchStateType searchState;
	    Int32 nextBatteryCheck = TimGetTicks();
	    UInt32 ftr;
	    int has_notification_mgr;
	    int4 version;
	    int init_mode;

	    error = FtrGet(sysFtrCreator, sysFtrNumROMVersion, &ftr);
	    if (error != errNone ||
		    ftr < sysMakeROMVersion(3, 0, 0, sysROMStageRelease, 0)) {
		ErrFatalDisplayIf(true,
			"PalmOS version too old - must be 3.0 or later.");
	    }
	    error = FtrGet(sysFtrCreator, sysFtrNumNotifyMgrVersion, &ftr);
	    has_notification_mgr = error == errNone && ftr != 0;

	    statefile = FileOpen(0, "Free42State", 'Stat', 'Fk42',
				 fileModeReadOnly, &error);
	    if (error != errNone)
		statefile = NULL;
	    if (statefile != NULL) {
		if (read_shell_state(&version))
		    init_mode = 1;
		else {
		    init_shell_state(-1);
		    init_mode = 2;
		}
	    } else {
		init_shell_state(-1);
		init_mode = 0;
	    }

	    load_skin();
	    print_bitmap = MyGlueBmpCreate(153, 160, 1, NULL, &error);

	    FrmGotoForm(calcform_id);

	    open_math_lib();

	    /* I allocate the printout buffer using a database, so it does
	     * not use any application heap space.
	     * This is necessary on 2 MB devices where the application heap
	     * is tiny -- eating up 64k here can cause allocating the BCD
	     * tables to fail, and that leads to a nasty crash.
	     */
	    open_printout();

	    core_init(init_mode, version);
	    if (statefile != NULL) {
		FileClose(statefile);
		statefile = NULL;
	    }
	    if (core_powercycle())
		want_to_run = 1;

	    /* Register for about-to-sleep and just-woke-up notifications. */
	    if (has_notification_mgr) {
		error = DmGetNextDatabaseByTypeCreator(true, &searchState,
			'appl', 'Fk42', true, &appCrd, &appDB);
		SysNotifyRegister(appCrd, appDB, sysNotifySleepRequestEvent,
				  NULL, sysNotifyNormalPriority, NULL);
		SysNotifyRegister(appCrd, appDB, sysNotifyLateWakeupEvent,
				  NULL, sysNotifyNormalPriority, NULL);
	    }

	    dbfs_init();

	    while (1) {
		Int32 now = TimGetTicks();
		Int32 eventWaitTicks = nextBatteryCheck - now;
		if (eventWaitTicks <= 0) {
		    eventWaitTicks = 60 * SysTicksPerSecond();
		    nextBatteryCheck = now + eventWaitTicks;
		    shell_low_battery();
		}
		if (timeout3time != -1) {
		    Int32 ticksLeft = timeout3time - now;
		    if (ticksLeft <= 0) {
			core_timeout3(1);
			timeout3time = -1;
		    } else {
			if (ticksLeft < eventWaitTicks)
			    eventWaitTicks = ticksLeft;
		    }
		}
		EvtGetEvent(&event, want_to_run ? 0 : eventWaitTicks);
#if 0
		{
		    char buf[100];
		    StrPrintF(buf, "type=%d chr=0x%04x", event.eType, event.data.keyDown.chr);
		    log(buf);
		}
#endif
		if (event.eType == appStopEvent)
		    break;
		else if (event.eType == nilEvent
			|| event.eType == firstUserEvent) {
		    int dummy1, dummy2;
		    want_to_run = core_keydown(0, &dummy1, &dummy2);
		} else {
		    if (!SysHandleEvent(&event))
			if (!MenuHandleEvent(NULL, &event, &error))
			    if (!handle_event(&event))
				FrmDispatchEvent(&event);

		    if (!feature_set_3_5_present()) {
			/* There's no FrmSetGadgetHandler() prior to PalmOS 3.5,
			 * so in that case, I need to invoke the calcgadget's
			 * and printgadget's handlers manually.
			 */
			if (event.eType == penDownEvent) {
			    if (FrmGetActiveFormID() == calcform_id) {
				EventType e;
				e.eType = frmGadgetEnterEvent;
				e.screenX = event.screenX;
				e.screenY = event.screenY;
				calcgadget_handler(NULL,
					    formGadgetHandleEventCmd, &e);
			    }
			} else if (event.eType == frmOpenEvent) {
			    if (event.data.frmOpen.formID == calcform_id)
				calcgadget_handler(NULL,
					    formGadgetDrawCmd, NULL);
			    else if (event.data.frmOpen.formID == printform_id)
				printgadget_handler(NULL,
					    formGadgetDrawCmd, NULL);
			}
		    }
		}
	    }

	    if (has_notification_mgr) {
		SysNotifyUnregister(appCrd, appDB, sysNotifySleepRequestEvent,
				    sysNotifyNormalPriority);
		SysNotifyUnregister(appCrd, appDB, sysNotifyLateWakeupEvent,
				    sysNotifyNormalPriority);
	    }

	    close_printout();

	    statefile = FileOpen(0, "Free42State", 'Stat', 'Fk42',
				 fileModeReadWrite, &error);

	    if (error != errNone)
		statefile = NULL;

	    if (statefile != NULL)
		write_shell_state();

	    core_quit();
	    if (statefile != NULL) {
		FileClose(statefile);
		UInt16 card = 0;
		LocalID dbid = DmFindDatabase(card, "Free42State");
		if (dbid != 0) {
		    UInt16 atts;
		    error = DmDatabaseInfo(card, dbid, NULL, &atts, NULL,
					NULL, NULL, NULL, NULL, NULL,
					NULL, NULL, NULL);
		    if (error == errNone
			    && (atts & dmHdrAttrBackup) == 0) {
			atts |= dmHdrAttrBackup;
			DmSetDatabaseInfo(card, dbid, NULL, &atts, NULL,
					NULL, NULL, NULL, NULL, NULL,
					NULL, NULL, NULL);
		    }
		}
	    }

	    close_math_lib();

	    FrmCloseAllForms();

	    if (disp_bitmap != NULL)
		MyGlueBmpDelete(disp_bitmap);
	    if (disp_bits_v3 != NULL)
		MemPtrFree(disp_bits_v3);
	    if (print_bitmap != NULL)
		MyGlueBmpDelete(print_bitmap);
	    unload_skin();
	    misc_cleanup();

	    if (state.printerToMemo)
		close_memo();
	    if (state.printerToTxtFile)
		close_txt();
	    if (state.printerToGifFile)
		close_gif(0);

	    shell_spool_exit();
	    dbfs_finish();

	    break;
	}

	case sysAppLaunchCmdNotify: {
	    SysNotifyParamType *param = (SysNotifyParamType *) pbp;
	    if ((flags & sysAppLaunchFlagSubCall) == 0)
		/* We're not the current application;
		 * no need to handle the notifications in this case.
		 * NOTE: this should never happen, as we unregister
		 * the notifications on app exit, but I put the check
		 * in just in case. The code that actually handles the
		 * notifications assumes that globals are available, so
		 * it would not be healthy to allow it to run if we're
		 * not the active application.
		 */
		break;
	    switch (param->notifyType) {
		case sysNotifySleepRequestEvent: {
		    /* The core will veto auto-off sleep requests if a program
		     * is running, or if the continuous-on flag (44) is set,
		     * but it will never veto sleep if the battery is low.
		     */
		    SleepEventParamType *sparam =
			    (SleepEventParamType *) param->notifyDetailsP;
		    int want_cpu = 0;
		    if (sparam->reason == sysSleepAutoOff
				    && !core_allows_powerdown(&want_cpu))
			sparam->deferSleep++;
		    if (want_cpu) {
			/* The core wants to run -- sounds like it was waiting
			 * in GETKEY and now wants program execution to resume.
			 * We post an event to get the event queue moving, in
			 * case EvtGetEvent() is currently waiting.
			 */
			EventType evt;
			evt.eType = firstUserEvent;
			EvtAddEventToQueue(&evt);
		    }
		    break;
		}
		case sysNotifyLateWakeupEvent:
		    /* Tell the core that a power cycle has occurred; it
		     * should respond by clearing the continuous-on flag (44),
		     * starting program execution if the autostart flag (11)
		     * is set, clearing flag 11, and whatever else is needed
		     * to simulate the effects of an off/on sequence on a real
		     * HP-42S.
		     */
		    if (core_powercycle()) {
			/* The core wants to run -- sounds like the auto-exec
			 * flag (11) was set. We post an event to get the event
			 * queue moving, in case EvtGetEvent() is currently
			 * waiting.
			 */
			EventType evt;
			evt.eType = firstUserEvent;
			EvtAddEventToQueue(&evt);
		    }
		    break;
	    }
	    break;
	}
    }

    return 0;
}
