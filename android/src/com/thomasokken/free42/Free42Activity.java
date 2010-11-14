/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.thomasokken.free42;

import java.io.InputStream;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.os.Bundle;
import android.util.AttributeSet;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;

/**
 * This class provides a basic demonstration of how to write an Android
 * activity. Inside of its window, it places a single view: an EditText that
 * displays and edits some internal text.
 */
public class Free42Activity extends Activity {
    
    static final private int BACK_ID = Menu.FIRST;
    static final private int CLEAR_ID = Menu.FIRST + 1;

    static {
    	System.loadLibrary("free42");
    }

    private native void nativeInit();
    private native String stringFromJNI();
    private native void redisplay();
    
    private Free42View view;
    private Bitmap skin;
    private Bitmap display;

    /** Called with the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        view = new Free42View(this);
        setContentView(view);
        
    	InputStream is = getClass().getResourceAsStream("Ehrling42sm.gif");
    	skin = new BitmapDrawable(is).getBitmap();
    	display = Bitmap.createBitmap(131, 16, Bitmap.Config.ARGB_8888);
    	
		//String s = stringFromJNI();
    	nativeInit();
    }

    /**
     * Called when the activity is about to start interacting with the user.
     */
    @Override
    protected void onResume() {
        super.onResume();
    }

    /**
     * Called when your activity's options menu needs to be created.
     */
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);

        // We are going to create two menus. Note that we assign them
        // unique integer IDs, labels from our string resources, and
        // given them shortcuts.
        menu.add(0, BACK_ID, 0, R.string.back).setShortcut('0', 'b');
        menu.add(0, CLEAR_ID, 0, R.string.clear).setShortcut('1', 'c');

        return true;
    }

    /**
     * Called right before your activity's option menu is displayed.
     */
    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        super.onPrepareOptionsMenu(menu);

        // Before showing the menu, we need to decide whether the clear
        // item is enabled depending on whether there is text to clear.
        //menu.findItem(CLEAR_ID).setVisible(canvas.getText().length() > 0);

        return true;
    }

    /**
     * Called when a menu item is selected.
     */
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
        case BACK_ID:
            finish();
            return true;
        case CLEAR_ID:
        	redisplay();
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    /**
     * A call-back for when the user presses the back button.
     */
    OnClickListener mBackListener = new OnClickListener() {
        public void onClick(View v) {
            finish();
        }
    };

    /**
     * A call-back for when the user presses the clear button.
     */
    OnClickListener mClearListener = new OnClickListener() {
        public void onClick(View v) {
            redisplay();
        }
    };
    
    private class Free42View extends View {

    	public Free42View(Context context) {
    		super(context);
    	}

    	public Free42View(Context context, AttributeSet attrs) {
    		super(context, attrs);
    	}

    	public Free42View(Context context, AttributeSet attrs, int defStyle) {
    		super(context, attrs, defStyle);
    	}

    	private int width = -1;
    	private int height = -1;
    	
    	@Override
    	protected void onDraw(Canvas canvas) {
    		Paint p = new Paint();
    		canvas.drawBitmap(skin, 0, 0, p);
    		Rect src = new Rect(0, 0, 131, 16);
    		Rect dst = new Rect(29, 38, 291, 86);
    		canvas.drawBitmap(display, src, dst, p);
    	}
    	
    	@Override
    	protected void onSizeChanged(int w, int h, int oldw, int oldh) {
    		width = w;
    		height = h;
    		postInvalidate();
    	}
    }

	/** shell_blitter()
	 *
	 * Callback invoked by the emulator core to cause the display, or some portion
	 * of it, to be repainted.
	 *
	 * 'bits' is a pointer to a 1 bpp (monochrome) bitmap. The bits within a byte
	 * are laid out with left corresponding to least significant, right
	 * corresponding to most significant; this corresponds to the convention for
	 * X11 images, but it is the reverse of the convention for MacOS and its
	 * derivatives (Microsoft Windows and PalmOS).
	 * The bytes are laid out sequentially, that is, bits[0] is at the top
	 * left corner, bits[1] is to the right of bits[0], bits[2] is to the right of
	 * bits[1], and so on; this corresponds to X11, MacOS, Windows, and PalmOS
	 * usage.
	 * 'bytesperline' is the number of bytes per line of the bitmap; this means
	 * that the bits just below bits[0] are at bits[bytesperline].
	 * 'x', 'y', 'width', and 'height' define the part of the bitmap that needs to
	 * be repainted. 'x' and 'y' are 0-based coordinates, with (0, 0) being the top
	 * left corner of the bitmap, and x coordinates increasing to the right, and y
	 * coordinates increasing downwards. 'width' and 'height' are the width and
	 * height of the area to be repainted.
	 */
    public void shell_blitter(byte[] bits, int bytesperline, int x, int y, int width, int height) {
    	// TODO -- This is just to get started; obviously not terribly efficient yet
    	int hmax = x + width;
    	int vmax = y + height;
    	for (int v = y; v < vmax; v++)
    		for (int h = x; h < hmax; h++) {
    			boolean bitSet = (bits[(h >> 3) + v * bytesperline] & (1 << (h & 7))) != 0;
    			display.setPixel(h, v, bitSet ? Color.BLACK : Color.WHITE);
    		}
    	view.postInvalidate(); // TODO -- only invalidate as much as necessary!
    }

///** shell_beeper()
// * Callback invoked by the emulator core to play a sound.
// * The first parameter is the frequency in Hz; the second is the
// * duration in ms. The sound volume is up to the GUI to control.
// * Sound playback should be synchronous (the beeper function should
// * not return until the sound has finished), if possible.
// */
//void shell_beeper(int frequency, int duration) SHELL1_SECT;
//
///** shell_annunciators()
// * Callback invoked by the emulator core to change the state of the display
// * annunciators (up/down, shift, print, run, battery, (g)rad).
// * Every parameter can have values 0 (turn off), 1 (turn on), or -1 (leave
// * unchanged).
// * The battery annunciator is missing from the list; this is the only one of
// * the lot that the emulator core does not actually have any control over, and
// * so the shell is expected to handle that one by itself.
// */
//void shell_annunciators(int updn, int shf, int prt, int run, int g, int rad) SHELL1_SECT;
//
///** shell_wants_cpu()
// *
// * Callback used by the emulator core to check for pending events.
// * It calls this periodically during long operations, such as running a
// * user program, or the solver, etc. The shell should not handle any events
// * in this call! If there are pending events, it should return 1; the currently
// * active invocation of core_keydown() or core_keyup() will then return
// * immediately (with a return value of 1, to indicate that it would like to get
// * the CPU back as soon as possible).
// */
//int shell_wants_cpu() SHELL1_SECT;
//
///** Callback to suspend execution for the given number of milliseconds. No event
// * processing will take place during the wait, so the core can call this
// * without having to worry about core_keydown() etc. being re-entered.
// */
//void shell_delay(int duration) SHELL1_SECT;
//
///** Callback to ask the shell to call core_timeout3() after the given number of
// * milliseconds. If there are keystroke events during that time, the timeout is
// * cancelled. (Pressing 'shift' does not cancel the timeout.)
// * This function supports the delay after SHOW, MEM, and shift-VARMENU.
// */
//void shell_request_timeout3(int delay) SHELL1_SECT;
//
///** shell_read_saved_state()
// *
// * Callback to read from the saved state. The function will read up to n
// * bytes into the buffer pointed to by buf, and return the number of bytes
// * actually read. The function returns -1 if an error was encountered; a return
// * value of 0 signifies the end of input.
// * The emulator core should only call this function from core_init(), and only
// * if core_init() was called with an argument of 1. (Nothing horrible will
// * happen if you try to call this function during other contexts, but you will
// * always get an error then.)
// */
//int4 shell_read_saved_state(void *buf, int4 bufsize) SHELL1_SECT;
//
///** shell_write_saved_state()
// * Callback to dump the saved state to persistent storage.
// * Returns 1 on success, 0 on error.
// * The emulator core should only call this function from core_quit(). (Nothing
// * horrible will happen if you try to call this function during other contexts,
// * but you will always get an error then.)
// */
//bool shell_write_saved_state(const void *buf, int4 nbytes) SHELL1_SECT;
//
///** shell_get_mem()
// * Callback to get the amount of free memory in bytes.
// */
//uint4 shell_get_mem() SHELL1_SECT;
//
///** shell_low_battery()
// * Callback to find out if the battery is low. Used to emulate flag 49 and the
// * battery annunciator, and also taken into account when deciding whether or
// * not to allow a power-down -- so as long as the shell provides a functional
// * implementation of shell_low_battery(), it can leave the decision on how to
// * respond to sysNotifySleepRequestEvent to core_allows_powerdown().
// */
//int shell_low_battery() SHELL1_SECT;
//
///** shell_powerdown()
// * Callback to tell the shell that the emulator wants to power down.
// * Only called in response to OFF (shift-EXIT or the OFF command); automatic
// * power-off is left to the OS and/or shell.
// */
//void shell_powerdown() SHELL1_SECT;
//
///** shell_random_seed()
// * When SEED is invoked with X = 0, the random number generator should be
// * seeded to a random value; the emulator core calls this function to obtain
// * it. The shell should construct a double in the range [0, 1) in a random
// * manner, using the real-time clock or some other source of randomness.
// * Note that distribution is not very important; the value will only be used to
// * seed the RNG. What's important that using shell_random_seed() guarantees
// * that the RNG will be initialized to a different sequence. This matters for
// * applications like games where you don't want the same sequence of cards
// * dealt each time.
// */
//double shell_random_seed() SHELL1_SECT;
//
///** shell_milliseconds()
// * Returns an elapsed-time value in milliseconds. The caller should make no
// * assumptions as to what this value is relative to; it is only intended to
// * allow the emulator core make short-term elapsed-time measurements.
// */
//uint4 shell_milliseconds() SHELL1_SECT;
//
///** shell_print()
// * Printer emulation. The first 2 parameters are the plain text version of the
// * data to be printed; the remaining 6 parameters are the bitmap version. The
// * former is used for text-mode copying and for spooling to text files; the
// * latter is used for graphics-mode coopying, spooling to image files, and
// * on-screen display.
// */
//void shell_print(const char *text, int length,
//		 const char *bits, int bytesperline,
//		 int x, int y, int width, int height) SHELL1_SECT;
//
///** shell_write()
// *
// * Callback for core_export_programs(). Returns 0 if a problem occurred;
// * core_export_programs() should abort in that case.
// */
//int shell_write(const char *buf, int4 buflen) SHELL1_SECT;
//
///** shell_read()
// *
// * Callback for core_import_programs(). Returns the number of bytes actually
// * read. Returns -1 if an error occurred; a return value of 0 signifies end of
// * input.
// */
//int4 shell_read(char *buf, int4 buflen) SHELL1_SECT;
//
///** shell_get_bcd_table()
// * shell_put_bcd_table()
// * shell_release_bcd_table()
// * shell_bcd_table_struct
// *
// * On platforms where computing the BCD conversion table is slow, these
// * functions should be implemented so they save the table in persistent
// * storage. On other platforms, they can be no-ops.
// * shell_get_bcd_table() returns NULL if no BCD table image was found.
// */
//typedef struct {
//    double pos_huge_double;
//    double neg_huge_double;
//    double pos_tiny_double;
//    double neg_tiny_double;
//    int2 max_pow2;
//    int2 min_pow2;
//    uint4 pos_pow2exp_offset; /* Offsets are from the end of this struct */
//    uint4 neg_pow2mant_offset;/* pos_pow2mant_offset is implicitly 0 */
//    uint4 neg_pow2exp_offset;
//} shell_bcd_table_struct;
//
//shell_bcd_table_struct *shell_get_bcd_table() SHELL1_SECT;
//shell_bcd_table_struct *shell_put_bcd_table(shell_bcd_table_struct *bcdtab,
//					    uint4 size) SHELL1_SECT;
//void shell_release_bcd_table(shell_bcd_table_struct *bcdtab) SHELL1_SECT;
//
///** shell_get_time_date()
// *
// * Get the current time and date. The date should be provided formatted as
// * YYYYMMDD, and the time should be provided formatted as HHMMSSss (24-hour).
// * The weekday is a number from 0 to 6, with 0 being Sunday.
// */
//void shell_get_time_date(uint4 *time, uint4 *date, int *weekday) SHELL1_SECT;

}
