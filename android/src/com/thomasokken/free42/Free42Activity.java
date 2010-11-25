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

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.util.Date;
import java.util.Timer;
import java.util.TimerTask;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.util.AttributeSet;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;

/**
 * This Activity class contains most of the Free42 'shell' functionality;
 * the skin-specific code is separated into the SkinLayout class.
 * This class works in conjunction with free42glue.cc, which is the JNI-
 * based interface to the Free42 'core' functionality (the core is
 * C++ and porting it to Java is not practical, hence the use of JNI).
 */
public class Free42Activity extends Activity {
    
    static final private int BACK_ID = Menu.FIRST;
    static final private int CLEAR_ID = Menu.FIRST + 1;

    private static final int SHELL_VERSION = 0;
    
    static {
    	System.loadLibrary("free42");
    }

    private Free42View view;
    private SkinLayout layout;
    private Bitmap skin;
    
    // Streams for reading and writing the state file
    private InputStream stateFileInputStream;
    private OutputStream stateFileOutputStream;
    
    // Streams for program import and export
    private InputStream importInputStream;
    private OutputStream exportOutputStream;
    
    // Stuff to run core_keydown() on a background thread
    private CoreThread coreThread;
    private Object coreThreadMonitor = new Object();
    
	private boolean enqueued;
	private int repeat;
	private boolean coreWantsCpu;
	
	private Timer timer3;
	private Object timer3monitor = new Object();

	// Persistent state
    boolean printToGif;
    String printToGifFileName = "";
    boolean printToTxt;
    String printToTxtFileName = "";

    
    ///////////////////////////////////////////////////////
    ///// Top-level code to interface with Android UI /////
    ///////////////////////////////////////////////////////
    
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        view = new Free42View(this);
        setContentView(view);
        
        InputStream is = getClass().getResourceAsStream("Ehrling42sm.layout");
        try {
        	layout = new SkinLayout(is);
        } catch (IOException e) {
        	// Won't happen -- Ehrling42sm is a built-in resource.
        }
    	is = getClass().getResourceAsStream("Ehrling42sm.gif");
    	skin = new BitmapDrawable(is).getBitmap();
    	layout.setSkinBitmap(skin);

    	int init_mode;
		IntHolder version = new IntHolder();
    	try {
    		stateFileInputStream = openFileInput("state");
    	} catch (FileNotFoundException e) {
    		stateFileInputStream = null;
    	}
    	if (stateFileInputStream != null) {
            if (read_shell_state(version))
                init_mode = 1;
            else {
                init_shell_state(-1);
                init_mode = 2;
            }
        } else {
            init_shell_state(-1);
            init_mode = 0;
        }
    	
    	nativeInit();
    	core_init(init_mode, version.value);
    }

    /**
     * Called exactly once, when the application instance is about to be
     * destroyed.
     */
    @Override
    protected void onDestroy() {
    	super.onDestroy();
    	// Write state file
    	try {
    		stateFileOutputStream = openFileOutput("state", Context.MODE_PRIVATE);
    	} catch (FileNotFoundException e) {
    		stateFileOutputStream = null;
    	}
    	if (stateFileOutputStream != null) {
    		write_shell_state();
    		core_quit();
    	}
    	if (stateFileOutputStream != null) {
    		try {
    			stateFileOutputStream.close();
    		} catch (IOException e) {}
    		stateFileOutputStream = null;
    	}
    }
    
    /**
     * Called when the activity's options menu needs to be created.
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
     * Called right before the activity's option menu is displayed.
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
    		layout.skin_repaint(canvas);
    	}
    	
    	@Override
    	public boolean onTouchEvent(MotionEvent e) {
    		int what = e.getAction();
    		if (what != MotionEvent.ACTION_DOWN && what != MotionEvent.ACTION_UP)
    			return true;
    		
    		if (what == MotionEvent.ACTION_DOWN) {
	    		int x = (int) e.getX();
	    		int y = (int) e.getY();
	    		IntHolder skeyHolder = new IntHolder();
	    		IntHolder ckeyHolder = new IntHolder();
	    		layout.skin_find_key(core_menu(), x, y, false, skeyHolder, ckeyHolder);
	    		int skey = skeyHolder.value;
	    		int ckey = ckeyHolder.value;
	    		if (ckey == 0)
	    			return true;
	    		endCoreThread();
	    		synchronized (timer3monitor) {
//		            if (timer3 != null && (macro != NULL || ckey != 28 /* SHIFT */)) {
			        if (timer3 != null && ckey != 28 /* SHIFT */) {
			        	timer3.cancel();
		                timer3 = null;
		                core_timeout3(0);
		            }
	    		}
	    		coreThread = new CoreThread(ckey);
	    		coreThread.start();
    	    } else {
	    		endCoreThread();
    			coreWantsCpu = core_keyup();
    			if (coreWantsCpu) {
    				coreThread = new CoreThread(0);
    				coreThread.start();
    			}
    	    }
    			
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

    
    ////////////////////////////////////////////////////////////////////
    ///// This section is where all the real 'shell' work is done. /////
    ////////////////////////////////////////////////////////////////////
    
    private boolean read_shell_state(IntHolder version) {
    	try {
    		if (state_read_int() != FREE42_MAGIC())
    			return false;
    		version.value = state_read_int();
    		if (version.value < 0 || version.value > FREE42_VERSION())
    			return false;
    		int shell_version = state_read_int();
    		printToGif = state_read_boolean();
    	    printToGifFileName = state_read_string();
    	    printToTxt = state_read_boolean();
    	    printToTxtFileName = state_read_string();
    		init_shell_state(shell_version);
    	} catch (IllegalArgumentException e) {
    		return false;
    	}
    	return true;
    }
    
    private void init_shell_state(int shell_version) {
    	switch (shell_version) {
    	case -1:
    	    printToGif = false;
    	    printToGifFileName = "";
    	    printToTxt = false;
    	    printToTxtFileName = "";
    	    // fall through
    	case 0:
			// current version (SHELL_VERSION = 0),
			// so nothing to do here since everything
			// was initialized from the state file.
    		;
    	}
    }

    private void write_shell_state() {
    	try {
    		state_write_int(FREE42_MAGIC());
    		state_write_int(FREE42_VERSION());
    		state_write_int(SHELL_VERSION);
    		state_write_boolean(printToGif);
    		state_write_string(printToGifFileName);
    		state_write_boolean(printToTxt);
    		state_write_string(printToTxtFileName);
    	} catch (IllegalArgumentException e) {}
    }
    
	private byte[] int_buf = new byte[4];
    private int state_read_int() throws IllegalArgumentException {
    	if (shell_read_saved_state(int_buf) != 4)
    		throw new IllegalArgumentException();
    	return (int_buf[0] << 24) | ((int_buf[1] & 255) << 16) | ((int_buf[2] & 255) << 8) | (int_buf[3] & 255);
    }
    private void state_write_int(int i) throws IllegalArgumentException {
    	int_buf[0] = (byte) (i >> 24);
    	int_buf[1] = (byte) (i >> 16);
    	int_buf[2] = (byte) (i >> 8);
    	int_buf[3] = (byte) i;
    	if (!shell_write_saved_state(int_buf))
    		throw new IllegalArgumentException();
    }
    
	private byte[] boolean_buf = new byte[1];
    private boolean state_read_boolean() throws IllegalArgumentException {
    	if (shell_read_saved_state(boolean_buf) != 1)
    		throw new IllegalArgumentException();
    	return boolean_buf[0] != 0;
    }
    private void state_write_boolean(boolean b) throws IllegalArgumentException {
    	boolean_buf[0] = (byte) (b ? 1 : 0);
    	if (!shell_write_saved_state(boolean_buf))
    		throw new IllegalArgumentException();
    }
    
    private String state_read_string() throws IllegalArgumentException {
    	int length = state_read_int();
    	byte[] buf = new byte[length];
    	if (length > 0 && shell_read_saved_state(buf) != length)
    		throw new IllegalArgumentException();
    	try {
    		return new String(buf, "UTF-8");
    	} catch (UnsupportedEncodingException e) {
    		// Won't happen; UTF-8 is always supported.
    		return null;
    	}
    }
    private void state_write_string(String s) throws IllegalArgumentException {
    	byte[] buf;
    	try {
    		buf = s.getBytes("UTF-8");
    	} catch (UnsupportedEncodingException e) {
    		// Won't happen; UTF-8 is always supported.
    		throw new IllegalArgumentException();
    	}
    	state_write_int(buf.length);
    	shell_write_saved_state(buf);
    }

    private class CoreThread extends Thread {
    	private int keycode;
    	public boolean enqueued;
    	public int repeat;
    	public boolean coreWantsCpu;
    	public CoreThread(int keycode) {
    		this.keycode = keycode;
    	}
    	public void run() {
    		BooleanHolder enqHolder = new BooleanHolder();
    		IntHolder repHolder = new IntHolder();
    		coreWantsCpu = core_keydown(keycode, enqHolder, repHolder);
    		enqueued = enqHolder.value;
    		repeat = repHolder.value;
    	}
    }
    
    private void endCoreThread() {
    	synchronized (coreThreadMonitor) {
    		if (coreThread != null) {
    			core_keydown_finish();
    			try {
    				coreThread.join();
    			} catch (InterruptedException e) {}
    			enqueued = coreThread.enqueued;
    			repeat = coreThread.repeat;
    			coreWantsCpu = coreThread.coreWantsCpu;
    			coreThread = null;
    		}
    	}
    }
    

    //////////////////////////////////////////////////////////////////////////
    ///// Stubs for accessing the FREE42_MAGIC and FREE42_VERSION macros /////
    //////////////////////////////////////////////////////////////////////////
    
    private native int FREE42_MAGIC();
    private native int FREE42_VERSION();
    
    ///////////////////////////////////////////
    ///// Stubs for shell->core interface /////
    ///////////////////////////////////////////
    
    private native void nativeInit();
    private native void core_keydown_finish();
    
    private native void core_init(int read_state, int version);
    private native void core_quit();
    private native void core_repaint_display();
    private native boolean core_menu();
    private native boolean core_alpha_menu();
    private native boolean core_hex_menu();
    private native boolean core_keydown(int key, BooleanHolder enqueued, IntHolder repeat);
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
    	Rect inval = layout.skin_display_blitter(bits, bytesperline, x, y, width, height);
    	view.postInvalidate(inval.left, inval.top, inval.right, inval.bottom);
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
		for (int i = 0; i < 10; i++) {
			if (frequency <= cutoff_freqs[i]) {
				MediaPlayer mp = MediaPlayer.create(this, sound_ids[i]);
				mp.start();
				try {
					Thread.sleep(250);
				} catch (InterruptedException e) {}
				return;
			}
		}
		MediaPlayer mp = MediaPlayer.create(this, sound_ids[10]);
		mp.start();
		try {
			Thread.sleep(125);
		} catch (InterruptedException e) {}
	}

	private final int[] cutoff_freqs = { 164, 220, 243, 275, 293, 324, 366, 418, 438, 550 };
	private final int[] sound_ids = { R.raw.tone0, R.raw.tone1, R.raw.tone2, R.raw.tone3, R.raw.tone4, R.raw.tone5, R.raw.tone6, R.raw.tone7, R.raw.tone8, R.raw.tone9, R.raw.squeak };
	
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
    	Rect inval = layout.skin_annunciators(updn, shf, prt, run, g, rad);
    	if (inval != null)
    		view.postInvalidate(inval.left, inval.top, inval.right, inval.bottom);
	}
	
	/**
	 * Callback to ask the shell to call core_timeout3() after the given number of
	 * milliseconds. If there are keystroke events during that time, the timeout is
	 * cancelled. (Pressing 'shift' does not cancel the timeout.)
	 * This function supports the delay after SHOW, MEM, and shift-VARMENU.
	 */
	public void shell_request_timeout3(int delay) {
		synchronized (timer3monitor) {
			if (timer3 != null)
				timer3.cancel();
			timer3 = new Timer();
			TimerTask task = new TimerTask() {
				public void run() {
					timeout3();
				}
			};
			Date when = new Date(new Date().getTime() + delay);
			timer3.schedule(task, when);
		}
	}
	
	private void timeout3() {
		synchronized (timer3monitor) {
			if (timer3 != null)
				timer3.cancel();
			timer3 = null;
		}
		endCoreThread();
		core_timeout3(1);
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
		if (stateFileInputStream == null)
			return -1;
		try {
			int n = stateFileInputStream.read(buf);
			if (n <= 0) {
				stateFileInputStream.close();
				stateFileInputStream = null;
				return 0;
			} else
				return n;
		} catch (IOException e) {
			try {
				stateFileInputStream.close();
			} catch (IOException e2) {}
			stateFileInputStream = null;
			return -1;
		}
	}
	
	/**
	 * shell_write_saved_state()
	 * Callback to dump the saved state to persistent storage.
	 * Returns 'true' on success, 'false' on error.
	 * The emulator core should only call this function from core_quit(). (Nothing
	 * horrible will happen if you try to call this function during other contexts,
	 * but you will always get an error then.)
	 */
	public boolean shell_write_saved_state(byte[] buf) {
		if (stateFileOutputStream == null)
			return false;
		try {
			stateFileOutputStream.write(buf);
			return true;
		} catch (IOException e) {
			try {
				stateFileOutputStream.close();
			} catch (IOException e2) {}
			stateFileOutputStream = null;
			return false;
		}
	}
	
	/**
	 * shell_get_mem()
	 * Callback to get the amount of free memory in bytes.
	 */
	public int shell_get_mem() {
		long freeMem = Runtime.getRuntime().freeMemory();
		return freeMem > Integer.MAX_VALUE ? Integer.MAX_VALUE : (int) freeMem;
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
		// TODO -- see android.os.BatteryManager
		return 0;
	}
	
	/**
	 * shell_powerdown()
	 * Callback to tell the shell that the emulator wants to power down.
	 * Only called in response to OFF (shift-EXIT or the OFF command); automatic
	 * power-off is left to the OS and/or shell.
	 */
	public void shell_powerdown() {
        finish();
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
		if (exportOutputStream == null)
			return 0;
		try {
			exportOutputStream.write(buf);
			return 1;
		} catch (IOException e) {
			try {
				exportOutputStream.close();
			} catch (IOException e2) {}
			exportOutputStream = null;
			return 0;
		}
	}
	
	/**
	 * shell_read()
	 *
	 * Callback for core_import_programs(). Returns the number of bytes actually
	 * read. Returns -1 if an error occurred; a return value of 0 signifies end of
	 * input.
	 */
	public int shell_read(byte[] buf) {
		if (importInputStream == null)
			return -1;
		try {
			int n = importInputStream.read(buf);
			if (n <= 0) {
				importInputStream.close();
				importInputStream = null;
				return 0;
			} else
				return n;
		} catch (IOException e) {
			try {
				importInputStream.close();
			} catch (IOException e2) {}
			importInputStream = null;
			return -1;
		}
	}
	
	public void shell_log(String s) {
		System.err.println(s);
	}
}
