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

package com.thomasokken.free42;

import java.io.IOException;
import java.io.InputStream;
import java.util.Date;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.drawable.BitmapDrawable;
import android.os.Bundle;
import android.util.AttributeSet;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
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

    private Free42View view;
    private SkinLayout layout;
    private Bitmap skin;
    private Bitmap display;
    private long startTime = new Date().getTime();

    /** Called with the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        view = new Free42View(this);
        setContentView(view);
        
        InputStream is = getClass().getResourceAsStream("Ehrling42sm.layout");
        try {
        	layout = new SkinLayout(is);
        } catch (IOException e) {
        	// TODO
        }
    	is = getClass().getResourceAsStream("Ehrling42sm.gif");
    	skin = new BitmapDrawable(is).getBitmap();
    	display = Bitmap.createBitmap(131, 16, Bitmap.Config.ARGB_8888);
    	
    	nativeInit();
    	core_init(0, 0);
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

    	@Override
    	protected void onDraw(Canvas canvas) {
    		Paint p = new Paint();
    		canvas.drawBitmap(skin, 0, 0, p);
    		layout.skin_repaint_display(canvas, display);
    	}
    	
    	@Override
    	public boolean onTouchEvent(MotionEvent e) {
    		int what = e.getAction();
    		if (what != MotionEvent.ACTION_DOWN && what != MotionEvent.ACTION_UP)
    			return false;
    		int x = (int) e.getX();
    		int y = (int) e.getY();
    		IntHolder skeyHolder = new IntHolder();
    		IntHolder ckeyHolder = new IntHolder();
    		layout.skin_find_key(false, x, y, false, skeyHolder, ckeyHolder);
    		int skey = skeyHolder.value;
    		int ckey = ckeyHolder.value;
    		System.out.println((what == MotionEvent.ACTION_DOWN ? "down" : "up") + " (" + x + ", " + y + ") skey=" + skey + " ckey=" + ckey);
    		
    		if (what == MotionEvent.ACTION_DOWN) {
    			IntHolder enqueued = new IntHolder();
    			IntHolder repeat = new IntHolder();
    			core_keydown(ckey, enqueued, repeat);
    		} else
    			core_keyup();
    		return true;
    	}
    	
//    	private int width = -1;
//    	private int height = -1;
//    	
//    	@Override
//    	protected void onSizeChanged(int w, int h, int oldw, int oldh) {
//    		width = w;
//    		height = h;
//    		postInvalidate();
//    	}
    }

    
    ///////////////////////////////////////////
    ///// Stubs for shell->core interface /////
    ///////////////////////////////////////////
    
    private native void nativeInit();
    private native void core_init(int read_state, int version);
    private native void quit();
    private native void core_repaint_display();
    private native boolean core_menu();
    private native boolean core_alpha_menu();
    private native boolean core_hex_menu();
    private native boolean core_keydown(int key, IntHolder enqueued, IntHolder repeat);
    private native int core_repeat();
    private native void core_keytimeout1();
    private native void core_keytimeout2();
    private native boolean core_timeout3(int repaint);
    private native boolean core_keyup();
    private native boolean core_allows_powerdown(IntHolder want_cpu);
    private native boolean core_powercycle();
    private native int core_list_programs(byte[] buf);
    private native int core_program_size(int prgm_index);
    private native boolean core_export_programs(int[] indexes);
    private native void core_import_programs();
    private native String core_copy();
    private native void core_paste(String s);
    private native void getCoreSettings(CoreSettings settings);
    private native void putCoreSettings(CoreSettings settings);
    private native void redisplay();

    private static class CoreSettings {
		public boolean matrix_singularmatrix;
		public boolean matrix_outofrange;
		public boolean raw_text;
		public boolean auto_repeat;
		public boolean enable_ext_copan;
		public boolean enable_ext_bigstack;
		public boolean enable_ext_accel;
		public boolean enable_ext_locat;
		public boolean enable_ext_heading;
		public boolean enable_ext_time;
    }

    ///////////////////////////////////////////////////
    ///// Implementation of core->shell interface /////
    ///////////////////////////////////////////////////
    
	/**
	 * shell_blitter()
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
    	layout.skin_display_blitter(display, bits, bytesperline, x, y, width, height);
    	view.postInvalidate(); // TODO -- only invalidate as much as necessary!
    }

	/**
	 * shell_beeper()
	 * Callback invoked by the emulator core to play a sound.
	 * The first parameter is the frequency in Hz; the second is the
	 * duration in ms. The sound volume is up to the GUI to control.
	 * Sound playback should be synchronous (the beeper function should
	 * not return until the sound has finished), if possible.
	 */
	public void shell_beeper(int frequency, int duration) {
		// TODO
	}
	
	/**
	 * shell_annunciators()
	 * Callback invoked by the emulator core to change the state of the display
	 * annunciators (up/down, shift, print, run, battery, (g)rad).
	 * Every parameter can have values 0 (turn off), 1 (turn on), or -1 (leave
	 * unchanged).
	 * The battery annunciator is missing from the list; this is the only one of
	 * the lot that the emulator core does not actually have any control over, and
	 * so the shell is expected to handle that one by itself.
	 */
	public void shell_annunciators(int updn, int shf, int prt, int run, int g, int rad) {
		// TODO
	}
	
	/**
	 * shell_wants_cpu()
	 *
	 * Callback used by the emulator core to check for pending events.
	 * It calls this periodically during long operations, such as running a
	 * user program, or the solver, etc. The shell should not handle any events
	 * in this call! If there are pending events, it should return 1; the currently
	 * active invocation of core_keydown() or core_keyup() will then return
	 * immediately (with a return value of 1, to indicate that it would like to get
	 * the CPU back as soon as possible).
	 */
	public int shell_wants_cpu() {
		// TODO
		return 1;
	}
	
	/**
	 * Callback to suspend execution for the given number of milliseconds. No event
	 * processing will take place during the wait, so the core can call this
	 * without having to worry about core_keydown() etc. being re-entered.
	 */
	public void shell_delay(int duration) {
		try {
			Thread.sleep(duration);
		} catch (InterruptedException e) {}
	}
	
	/**
	 * Callback to ask the shell to call core_timeout3() after the given number of
	 * milliseconds. If there are keystroke events during that time, the timeout is
	 * cancelled. (Pressing 'shift' does not cancel the timeout.)
	 * This function supports the delay after SHOW, MEM, and shift-VARMENU.
	 */
	public void shell_request_timeout3(int delay) {
		// TODO
	}
	
	/**
	 * shell_read_saved_state()
	 *
	 * Callback to read from the saved state. The function will read up to n
	 * bytes into the buffer pointed to by buf, and return the number of bytes
	 * actually read. The function returns -1 if an error was encountered; a return
	 * value of 0 signifies the end of input.
	 * The emulator core should only call this function from core_init(), and only
	 * if core_init() was called with an argument of 1. (Nothing horrible will
	 * happen if you try to call this function during other contexts, but you will
	 * always get an error then.)
	 */
	public int shell_read_saved_state(byte[] buf) {
		// TODO
		return -1;
	}
	
	/**
	 * shell_write_saved_state()
	 * Callback to dump the saved state to persistent storage.
	 * Returns 1 on success, 0 on error.
	 * The emulator core should only call this function from core_quit(). (Nothing
	 * horrible will happen if you try to call this function during other contexts,
	 * but you will always get an error then.)
	 */
	public int shell_write_saved_state(byte[] buf) {
		// TODO
		return 0;
	}
	
	/**
	 * shell_get_mem()
	 * Callback to get the amount of free memory in bytes.
	 */
	public int shell_get_mem() {
		// TODO
		return 42;
	}
	
	/**
	 * shell_low_battery()
	 * Callback to find out if the battery is low. Used to emulate flag 49 and the
	 * battery annunciator, and also taken into account when deciding whether or
	 * not to allow a power-down -- so as long as the shell provides a functional
	 * implementation of shell_low_battery(), it can leave the decision on how to
	 * respond to sysNotifySleepRequestEvent to core_allows_powerdown().
	 */
	public int shell_low_battery() {
		// TODO
		return 0;
	}
	
	/**
	 * shell_powerdown()
	 * Callback to tell the shell that the emulator wants to power down.
	 * Only called in response to OFF (shift-EXIT or the OFF command); automatic
	 * power-off is left to the OS and/or shell.
	 */
	public void shell_powerdown() {
		// TODO
	}
	
	/**
	 * shell_random_seed()
	 * When SEED is invoked with X = 0, the random number generator should be
	 * seeded to a random value; the emulator core calls this function to obtain
	 * it. The shell should construct a double in the range [0, 1) in a random
	 * manner, using the real-time clock or some other source of randomness.
	 * Note that distribution is not very important; the value will only be used to
	 * seed the RNG. What's important that using shell_random_seed() guarantees
	 * that the RNG will be initialized to a different sequence. This matters for
	 * applications like games where you don't want the same sequence of cards
	 * dealt each time.
	 */
	public double shell_random_seed() {
		long t = new Date().getTime();
		return (t % 1000000000) / 1000000000d;
	}
	
	/**
	 * shell_milliseconds()
	 * Returns an elapsed-time value in milliseconds. The caller should make no
	 * assumptions as to what this value is relative to; it is only intended to
	 * allow the emulator core make short-term elapsed-time measurements.
	 */
	public int shell_milliseconds() {
		return (int) (new Date().getTime() - startTime);
	}
	
	/**
	 * shell_print()
	 * Printer emulation. The first 2 parameters are the plain text version of the
	 * data to be printed; the remaining 6 parameters are the bitmap version. The
	 * former is used for text-mode copying and for spooling to text files; the
	 * latter is used for graphics-mode coopying, spooling to image files, and
	 * on-screen display.
	 */
	public void shell_print(byte[] text, byte[] bits, int bytesperline,
			 int x, int y, int width, int height) {
		// TODO
	}
	
	/**
	 * shell_write()
	 *
	 * Callback for core_export_programs(). Returns 0 if a problem occurred;
	 * core_export_programs() should abort in that case.
	 */
	public int shell_write(byte[] buf) {
		// TODO
		return 0;
	}
	
	/**
	 * shell_read()
	 *
	 * Callback for core_import_programs(). Returns the number of bytes actually
	 * read. Returns -1 if an error occurred; a return value of 0 signifies end of
	 * input.
	 */
	public int shell_read(byte[] buf) {
		// TODO
		return -1;
	}
	
	public void shell_log(String s) {
		System.err.println(s);
	}
}
