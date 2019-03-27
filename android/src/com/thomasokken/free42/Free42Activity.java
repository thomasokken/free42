/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2019  Thomas Okken
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

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.RandomAccessFile;
import java.io.UnsupportedEncodingException;
import java.lang.reflect.Method;
import java.nio.IntBuffer;
import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.util.ArrayList;
import java.util.List;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.location.Criteria;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.media.AudioManager;
import android.media.SoundPool;
import android.os.BatteryManager;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.os.Vibrator;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.text.SpannableString;
import android.text.method.LinkMovementMethod;
import android.text.util.Linkify;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.TextView;

/**
 * This Activity class contains most of the Free42 'shell' functionality;
 * the skin-specific code is separated into the SkinLayout class.
 * This class works in conjunction with free42glue.cc, which is the JNI-
 * based interface to the Free42 'core' functionality (the core is
 * C++ and porting it to Java is not practical, hence the use of JNI).
 */
public class Free42Activity extends Activity {

    private static final String[] builtinSkinNames = new String[] { "Standard", "Landscape" };
    
    private static final int SHELL_VERSION = 13;
    
    private static final int PRINT_BACKGROUND_COLOR = Color.LTGRAY;
    
    private static final int MY_PERMISSIONS_REQUEST_ACCESS_FINE_LOCATION = 1;
    
    public static Free42Activity instance;
    
    public static final String MY_STORAGE_DIR = Environment.getExternalStorageDirectory() + "/Android/data/com.thomasokken.free42";
    
    static {
        System.loadLibrary("free42");
    }
    
    private CalcView calcView;
    private SkinLayout skin;
    private PrintView printView;
    private ScrollView printScrollView;
    private boolean printViewShowing;
    private PreferencesDialog preferencesDialog;
    private AlertDialog mainMenuDialog;
    private Handler mainHandler;
    private boolean alwaysOn;
    
    private SoundPool soundPool;
    private int[] soundIds;
    
    // Streams for reading and writing the state file
    private InputStream stateFileInputStream;
    private OutputStream stateFileOutputStream;
    
    // Streams for program import and export
    private InputStream programsInputStream;
    private OutputStream programsOutputStream;
    
    // Stuff to run core_keydown() on a background thread
    private CoreThread coreThread;
    private boolean coreWantsCpu;
    
    private int ckey;
    private boolean timeout3_active;
    
    private boolean low_battery;
    private BroadcastReceiver lowBatteryReceiver;

    // Persistent state
    private int orientation = 0; // 0=portrait, 1=landscape
    private String[] skinName = new String[] { builtinSkinNames[0], builtinSkinNames[0] };
    private String[] externalSkinName = new String[2];
    private boolean[] skinSmoothing = new boolean[2];
    private boolean[] displaySmoothing = new boolean[2];
    private boolean[] maintainSkinAspect = new boolean[2];

    private boolean alwaysRepaintFullDisplay = false;
    private boolean keyClicksEnabled = true;
    private boolean keyVibrationEnabled = false;
    private int preferredOrientation = ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED;
    private int style = 0;
    
    private final Runnable repeaterCaller = new Runnable() { public void run() { repeater(); } };
    private final Runnable timeout1Caller = new Runnable() { public void run() { timeout1(); } };
    private final Runnable timeout2Caller = new Runnable() { public void run() { timeout2(); } };
    private final Runnable timeout3Caller = new Runnable() { public void run() { timeout3(); } };
    
    ///////////////////////////////////////////////////////
    ///// Top-level code to interface with Android UI /////
    ///////////////////////////////////////////////////////
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        instance = this;
        
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
        setAlwaysRepaintFullDisplay(alwaysRepaintFullDisplay);
        if (alwaysOn)
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        if (style == 1)
            setTheme(android.R.style.Theme_NoTitleBar_Fullscreen);
        else if (style == 2) {
            try {
                Method m = View.class.getMethod("setSystemUiVisibility", int.class);
                m.invoke(getWindow().getDecorView(), PreferencesDialog.immersiveModeFlags);
            } catch (Exception e) {}
        }
        
        Configuration conf = getResources().getConfiguration();
        orientation = conf.orientation == Configuration.ORIENTATION_LANDSCAPE ? 1 : 0;
        
        mainHandler = new Handler();
        calcView = new CalcView(this);
        setContentView(calcView);
        printView = new PrintView(this);
        printScrollView = new ScrollView(this);
        printScrollView.setBackgroundColor(PRINT_BACKGROUND_COLOR);
        printScrollView.addView(printView);
        
        skin = null;
        if (skinName[orientation].length() == 0 && externalSkinName[orientation].length() > 0) {
            try {
                skin = new SkinLayout(externalSkinName[orientation], skinSmoothing[orientation], displaySmoothing[orientation], maintainSkinAspect[orientation]);
            } catch (IllegalArgumentException e) {}
        }
        if (skin == null) {
            try {
                skin = new SkinLayout(skinName[orientation], skinSmoothing[orientation], displaySmoothing[orientation], maintainSkinAspect[orientation]);
            } catch (IllegalArgumentException e) {}
        }
        if (skin == null) {
            try {
                skin = new SkinLayout(builtinSkinNames[0], skinSmoothing[orientation], displaySmoothing[orientation], maintainSkinAspect[orientation]);
            } catch (IllegalArgumentException e) {
                // This one should never fail; we're loading a built-in skin.
            }
        }
        calcView.updateScale();

        nativeInit();
        core_init(init_mode, version.value);
        if (stateFileInputStream != null) {
            try {
                stateFileInputStream.close();
            } catch (IOException e) {}
            stateFileInputStream = null;
        }
        
        lowBatteryReceiver = new BroadcastReceiver() {
            public void onReceive(Context ctx, Intent intent) {
                low_battery = intent.getAction().equals(Intent.ACTION_BATTERY_LOW);
                Rect inval = skin.update_annunciators(-1, -1, -1, -1, low_battery ? 1 : 0, -1, -1);
                if (inval != null)
                    calcView.postInvalidateScaled(inval.left, inval.top, inval.right, inval.bottom);
            }
        };
        IntentFilter iff = new IntentFilter();
        iff.addAction(Intent.ACTION_BATTERY_LOW);
        iff.addAction(Intent.ACTION_BATTERY_OKAY);
        registerReceiver(lowBatteryReceiver, iff);
        
        if (preferredOrientation != this.getRequestedOrientation())
            setRequestedOrientation(preferredOrientation);

        soundPool = new SoundPool(1, AudioManager.STREAM_SYSTEM, 0);
        int[] soundResourceIds = { R.raw.tone0, R.raw.tone1, R.raw.tone2, R.raw.tone3, R.raw.tone4, R.raw.tone5, R.raw.tone6, R.raw.tone7, R.raw.tone8, R.raw.tone9, R.raw.squeak, R.raw.click };
        soundIds = new int[soundResourceIds.length];
        for (int i = 0; i < soundResourceIds.length; i++)
            soundIds[i] = soundPool.load(this, soundResourceIds[i], 1);
    }
    
    @Override
    protected void onResume() {
        super.onResume();
        
        // Check battery level -- this is necessary because the ACTTON_BATTERY_LOW
        // and ACTION_BATTERY_OKAY intents are not "sticky", i.e., we get those
        // notifications only when that status *changes*; we don't get any indication
        // of what that status *is* when the app is launched (or resumed?).
        IntentFilter ifilter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
        Intent batteryStatus = registerReceiver(null, ifilter);
        int level = batteryStatus.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
        int scale = batteryStatus.getIntExtra(BatteryManager.EXTRA_SCALE, -1);
        if (low_battery)
            low_battery = level * 100 < scale * 20;
        else
            low_battery = level * 100 <= scale * 15;
        Rect inval = skin.update_annunciators(-1, -1, -1, -1, low_battery ? 1 : 0, -1, -1);
        if (inval != null)
            calcView.postInvalidateScaled(inval.left, inval.top, inval.right, inval.bottom);

        if (core_powercycle())
            start_core_keydown();
    }
    
    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (style == 2) {
            try {
                Method m = View.class.getMethod("setSystemUiVisibility", int.class);
                m.invoke(getWindow().getDecorView(), PreferencesDialog.immersiveModeFlags);
            } catch (Exception e) {}
        }
    }

    @Override
    protected void onPause() {
        end_core_keydown();
        // Write state file
        File filesDir = getFilesDir();
        File stateFile = null;
        try {
            stateFile = File.createTempFile("state.", ".new", filesDir);
            stateFileOutputStream = new FileOutputStream(stateFile, true);
        } catch (IOException e) {
            stateFileOutputStream = null;
        }
        if (stateFileOutputStream != null) {
            write_shell_state();
            core_enter_background();
        }
        if (stateFileOutputStream != null) {
            try {
                stateFileOutputStream.close();
            } catch (IOException e) {
                stateFileOutputStream = null;
            }
        }
        if (stateFileOutputStream != null) {
            // Writing state file succeeded; rename state.new to state
            stateFile.renameTo(new File(filesDir, "state"));
            stateFileOutputStream = null;
        } else {
            // Writing state file failed; delete state.new, if it even exists
            if (stateFile != null)
                stateFile.delete();
        }
        printView.dump();
        if (printTxtStream != null) {
            try {
                printTxtStream.close();
            } catch (IOException e) {}
            printTxtStream = null;
        }
        if (printGifFile != null) {
            try {
                ShellSpool.shell_finish_gif(printGifFile);
            } catch (IOException e) {}
            try {
                printGifFile.close();
            } catch (IOException e) {}
            printGifFile = null;
        }
        super.onPause();
    }
    
    @Override
    protected void onDestroy() {
        // N.B. In the Android build, core_quit() does not write the
        // core state; we assume that onPause() has been called previously,
        // and its core_enter_background() call takes care of saving state.
        // All this core_quit() call does it free up memory.
        core_quit();
        if (lowBatteryReceiver != null) {
            unregisterReceiver(lowBatteryReceiver);
            lowBatteryReceiver = null;
        }
        super.onDestroy();
    }
    
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (printViewShowing && keyCode == KeyEvent.KEYCODE_BACK) {
            doFlipCalcPrintout();
            return true;
        } else {
            return super.onKeyDown(keyCode, event);
        }
    }
    
    @Override
    public void onConfigurationChanged(Configuration newConf) {
        super.onConfigurationChanged(newConf);
        orientation = newConf.orientation == Configuration.ORIENTATION_LANDSCAPE ? 1 : 0;
        boolean[] ann_state = skin.getAnnunciators();
        SkinLayout newSkin = null;
        if (skinName[orientation].length() == 0 && externalSkinName[orientation].length() > 0) {
            try {
                newSkin = new SkinLayout(externalSkinName[orientation], skinSmoothing[orientation], displaySmoothing[orientation], maintainSkinAspect[orientation], ann_state);
            } catch (IllegalArgumentException e) {}
        }
        if (newSkin == null) {
            try {
                newSkin = new SkinLayout(skinName[orientation], skinSmoothing[orientation], displaySmoothing[orientation], maintainSkinAspect[orientation], ann_state);
            } catch (IllegalArgumentException e) {}
        }
        if (newSkin == null) {
            try {
                newSkin = new SkinLayout(builtinSkinNames[0], skinSmoothing[orientation], displaySmoothing[orientation], maintainSkinAspect[orientation], ann_state);
            } catch (IllegalArgumentException e) {
                // This one should never fail; we're loading a built-in skin.
            }
        }
        if (newSkin != null)
            skin = newSkin;
        calcView.updateScale();
        calcView.invalidate();
        core_repaint_display();
    }
    
    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        // ignore
    }
    
    private void cancelRepeaterAndTimeouts1And2() {
        mainHandler.removeCallbacks(repeaterCaller);
        mainHandler.removeCallbacks(timeout1Caller);
        mainHandler.removeCallbacks(timeout2Caller);
    }
    
    private void cancelTimeout3() {
        mainHandler.removeCallbacks(timeout3Caller);
        timeout3_active = false;
    }
    
    private void postMainMenu() {
        if (mainMenuDialog == null) {
            AlertDialog.Builder builder = new AlertDialog.Builder(this);
            builder.setTitle("Main Menu");
            List<String> itemsList = new ArrayList<String>();
            itemsList.add("Copy");
            itemsList.add("Paste");
            itemsList.add("Preferences");
            itemsList.add("Show Print-Out");
            itemsList.add("Clear Print-Out");
            itemsList.add("About Free42");
            itemsList.add("Import Programs");
            itemsList.add("Export Programs");
            for (int i = 0; i < builtinSkinNames.length; i++)
                itemsList.add("Skin: \"" + builtinSkinNames[i] + "\"");
            itemsList.add("Skin: Other...");
            itemsList.add("Cancel");
            builder.setItems(itemsList.toArray(new String[itemsList.size()]),
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                            mainMenuItemSelected(which);
                        }
                    });
            mainMenuDialog = builder.create();
        }
        mainMenuDialog.show();
    }
    
    private void mainMenuItemSelected(int which) {
        switch (which) {
        case 0:
            doCopy();
            return;
        case 1:
            doPaste();
            return;
        case 2:
            doPreferences();
            return;
        case 3:
            doFlipCalcPrintout();
            return;
        case 4:
            doClearPrintout();
            return;
        case 5:
            doAbout();
            return;
        case 6:
            doImport();
            return;
        case 7:
            doExport();
            return;
        default:
            int index = which - 8;
            if (index >= 0 && index < builtinSkinNames.length) {
                doSelectSkin(builtinSkinNames[index]);
                return;
            } else if (index == builtinSkinNames.length) {
                if (!checkStorageAccess())
                    return;
                FileSelectionDialog fsd = new FileSelectionDialog(this, new String[] { "layout", "*" });
                if (externalSkinName[orientation].length() > 0)
                    fsd.setPath(externalSkinName[orientation] + ".layout");
                fsd.setOkListener(new FileSelectionDialog.OkListener() {
                    public void okPressed(String path) {
                        if (path.endsWith(".layout"))
                            doSelectSkin(path.substring(0, path.length() - 7));
                    }
                });
                fsd.show();
                return;
            }
        }
    }
    
    private void doCopy() {
        android.text.ClipboardManager clip = (android.text.ClipboardManager) getSystemService(CLIPBOARD_SERVICE);
        clip.setText(core_copy());
    }
    
    private void doPaste() {
        android.text.ClipboardManager clip = (android.text.ClipboardManager) getSystemService(CLIPBOARD_SERVICE);
        if (clip.hasText())
            core_paste(clip.getText().toString());
    }
    
    private void doFlipCalcPrintout() {
        printViewShowing = !printViewShowing;
        setContentView(printViewShowing ? printScrollView : calcView);
    }
    
    private void doClearPrintout() {
        printView.clear();
    }
    
    private void doImport() {
        if (!checkStorageAccess())
            return;
        FileSelectionDialog fsd = new FileSelectionDialog(this, new String[] { "raw", "*" });
        fsd.setOkListener(new FileSelectionDialog.OkListener() {
            public void okPressed(String path) {
                doImport2(path);
            }
        });
        fsd.show();
    }
    
    private void doImport2(String path) {
        try {
            programsInputStream = new FileInputStream(path);
        } catch (IOException e) {
            alert("Import failed: " + e.getMessage());
            return;
        }
        core_import_programs();
        redisplay();
        if (programsInputStream != null) {
            try {
                programsInputStream.close();
            } catch (IOException e) {}
            programsInputStream = null;
        }
    }
    
    private boolean[] selectedProgramIndexes;
    
    private void alert(String message) {
        runOnUiThread(new Alerter(message));
    }
    
    private class Alerter implements Runnable {
        private String message;
        public Alerter(String message) {
            this.message = message;
        }
        public void run() {
            AlertDialog.Builder builder = new AlertDialog.Builder(Free42Activity.this);
            builder.setMessage(message);
            builder.setPositiveButton("OK", null);
            builder.create().show();
        }
    }

    private void doExport() {
        if (!checkStorageAccess())
            return;
        String[] names = core_list_programs();
        selectedProgramIndexes = new boolean[names.length];
        
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Select Programs");
        builder.setMultiChoiceItems(names, selectedProgramIndexes, new DialogInterface.OnMultiChoiceClickListener() {
            public void onClick(DialogInterface dialog, int which, boolean isChecked) {
                // I don't have to do anything here; the only reason why
                // I create this listener is because if I pass 'null'
                // instead, the selectedProgramIndexes array never gets
                // updated.
            }
        });
        DialogInterface.OnClickListener listener = new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                doProgramSelectionClick(dialog, which);
            }
        };
        builder.setPositiveButton("OK", listener);
        builder.setNegativeButton("Cancel", null);
        builder.create().show();
    }
    
    private void doProgramSelectionClick(DialogInterface dialog, int which) {
        if (which == DialogInterface.BUTTON_POSITIVE) {
            boolean none = true;
            for (int i = 0; i < selectedProgramIndexes.length; i++)
                if (selectedProgramIndexes[i]) {
                    none = false;
                    break;
                }
            if (!none) {
                FileSelectionDialog fsd = new FileSelectionDialog(this, new String[] { "raw", "*" });
                fsd.setOkListener(new FileSelectionDialog.OkListener() {
                    public void okPressed(String path) {
                        doExport2(path);
                    }
                });
                fsd.show();
            }
        }
        dialog.dismiss();
    }
    
    private void doExport2(String path) {
        try {
            programsOutputStream = new FileOutputStream(path);
        } catch (IOException e) {
            alert("Export failed: " + e.getMessage());
            return;
        }
        
        int n = 0;
        for (int i = 0; i < selectedProgramIndexes.length; i++)
            if (selectedProgramIndexes[i])
                n++;
        int[] selection = new int[n];
        n = 0;
        for (int i = 0; i < selectedProgramIndexes.length; i++)
            if (selectedProgramIndexes[i])
                selection[n++] = i;
        core_export_programs(selection);
        
        if (programsOutputStream != null) {
            try {
                programsOutputStream.close();
            } catch (IOException e) {}
            programsOutputStream = null;
        }
    }
    
    private void doSelectSkin(String skinName) {
        try {
            boolean[] annunciators = skin.getAnnunciators();
            skin = new SkinLayout(skinName, skinSmoothing[orientation], displaySmoothing[orientation], maintainSkinAspect[orientation], annunciators);
            if (skinName.startsWith("/")) {
                externalSkinName[orientation] = skinName;
                this.skinName[orientation] = "";
            } else
                this.skinName[orientation] = skinName;
            calcView.updateScale();
            calcView.invalidate();
            core_repaint_display();
        } catch (IllegalArgumentException e) {
            shell_beeper(1835, 125);
        }
    }
    
    private void doPreferences() {
        if (preferencesDialog == null) {
            preferencesDialog = new PreferencesDialog(this);
            preferencesDialog.setOkListener(new PreferencesDialog.OkListener() {
                public void okPressed() {
                    doPreferencesOk();
                }
            });
        }
        
        CoreSettings cs = new CoreSettings();
        getCoreSettings(cs);
        preferencesDialog.setSingularMatrixError(cs.matrix_singularmatrix);
        preferencesDialog.setMatrixOutOfRange(cs.matrix_outofrange);
        preferencesDialog.setAutoRepeat(cs.auto_repeat);
        preferencesDialog.setAlwaysOn(shell_always_on(-1) != 0);
        preferencesDialog.setKeyClicks(keyClicksEnabled);
        preferencesDialog.setKeyVibration(keyVibrationEnabled);
        preferencesDialog.setOrientation(preferredOrientation);
        preferencesDialog.setStyle(style);
        preferencesDialog.setDisplayFullRepaint(alwaysRepaintFullDisplay);
        preferencesDialog.setMaintainSkinAspect(maintainSkinAspect[orientation]);
        preferencesDialog.setSkinSmoothing(skinSmoothing[orientation]);
        preferencesDialog.setDisplaySmoothing(displaySmoothing[orientation]);
        preferencesDialog.setPrintToText(ShellSpool.printToTxt);
        preferencesDialog.setPrintToTextFileName(ShellSpool.printToTxtFileName);
        preferencesDialog.setPrintToGif(ShellSpool.printToGif);
        preferencesDialog.setPrintToGifFileName(ShellSpool.printToGifFileName);
        preferencesDialog.setMaxGifHeight(ShellSpool.maxGifHeight);
        preferencesDialog.show();
    }
        
    private void doPreferencesOk() {
        CoreSettings cs = new CoreSettings();
        getCoreSettings(cs);
        cs.matrix_singularmatrix = preferencesDialog.getSingularMatrixError();
        cs.matrix_outofrange = preferencesDialog.getMatrixOutOfRange();
        cs.auto_repeat = preferencesDialog.getAutoRepeat();
        shell_always_on(preferencesDialog.getAlwaysOn() ? 1 : 0);
        keyClicksEnabled = preferencesDialog.getKeyClicks();
        keyVibrationEnabled = preferencesDialog.getKeyVibration();
        int oldOrientation = preferredOrientation;
        preferredOrientation = preferencesDialog.getOrientation();
        style = preferencesDialog.getStyle();
        alwaysRepaintFullDisplay = preferencesDialog.getDisplayFullRepaint();
        putCoreSettings(cs);
        setAlwaysRepaintFullDisplay(alwaysRepaintFullDisplay);

        ShellSpool.maxGifHeight = preferencesDialog.getMaxGifHeight();

        boolean newPrintEnabled = preferencesDialog.getPrintToText();
        String newFileName = preferencesDialog.getPrintToTextFileName();
        if (printTxtStream != null && (!newPrintEnabled || !newFileName.equals(ShellSpool.printToTxtFileName))) {
            try {
                printTxtStream.close();
            } catch (IOException e) {}
            printTxtStream = null;
        }
        ShellSpool.printToTxt = newPrintEnabled;
        ShellSpool.printToTxtFileName = newFileName;
        
        newPrintEnabled = preferencesDialog.getPrintToGif();
        newFileName = preferencesDialog.getPrintToGifFileName();
        if (printGifFile != null && (!newPrintEnabled || !newFileName.equals(ShellSpool.printToGifFileName))) {
            try {
                ShellSpool.shell_finish_gif(printGifFile);
            } catch (IOException e) {}
            try {
                printGifFile.close();
            } catch (IOException e) {}
            printGifFile = null;
            gif_seq = 0;
        }
        ShellSpool.printToGif = newPrintEnabled;
        ShellSpool.printToGifFileName = newFileName;
        
        boolean newMaintainSkinAspect = preferencesDialog.getMaintainSkinAspect();
        if (newMaintainSkinAspect != maintainSkinAspect[orientation]) {
            maintainSkinAspect[orientation] = newMaintainSkinAspect;
            skin.setMaintainSkinAspect(newMaintainSkinAspect);
            calcView.updateScale();
            calcView.invalidate();
        }
        
        boolean newSkinSmoothing = preferencesDialog.getSkinSmoothing();
        boolean newDisplaySmoothing = preferencesDialog.getDisplaySmoothing();
        if (newSkinSmoothing != skinSmoothing[orientation] || newDisplaySmoothing != displaySmoothing[orientation]) {
            skinSmoothing[orientation] = newSkinSmoothing;
            displaySmoothing[orientation] = newDisplaySmoothing;
            skin.setSmoothing(newSkinSmoothing, newDisplaySmoothing);
            calcView.invalidate();
        }
        
        if (preferredOrientation != oldOrientation)
            setRequestedOrientation(preferredOrientation);
    }
    
    private void doAbout() {
        new AboutDialog(this).show();
    }
    
    public class AboutDialog extends Dialog {
        private AboutView view;
        
        public AboutDialog(Context context) {
            super(context);
            view = new AboutView(context);
            setContentView(view);
            this.setTitle("About Free42");
        }
        
        private class AboutView extends RelativeLayout {
            public AboutView(Context context) {
                super(context);
                
                ImageView icon = new ImageView(context);
                icon.setId(1);
                icon.setImageResource(R.drawable.icon);
                addView(icon);
                
                TextView label1 = new TextView(context);
                label1.setId(2);
                String version = "";
                try {
                    version = " " + getPackageManager().getPackageInfo(getPackageName(), 0).versionName;
                } catch (NameNotFoundException e) {}
                label1.setText("Free42" + version);
                LayoutParams lp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
                lp.addRule(RelativeLayout.ALIGN_TOP, icon.getId());
                lp.addRule(RelativeLayout.RIGHT_OF, icon.getId());
                addView(label1, lp);

                TextView label2 = new TextView(context);
                label2.setId(3);
                label2.setText("(C) 2004-2019 Thomas Okken");
                lp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
                lp.addRule(RelativeLayout.ALIGN_LEFT, label1.getId());
                lp.addRule(RelativeLayout.BELOW, label1.getId());
                addView(label2, lp);

                TextView label3 = new TextView(context);
                label3.setId(4);
                SpannableString s = new SpannableString("http://thomasokken.com/free42/");
                Linkify.addLinks(s, Linkify.WEB_URLS);
                label3.setText(s);
                label3.setMovementMethod(LinkMovementMethod.getInstance());
                lp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
                lp.addRule(RelativeLayout.ALIGN_LEFT, label2.getId());
                lp.addRule(RelativeLayout.BELOW, label2.getId());
                addView(label3, lp);

                TextView label4 = new TextView(context);
                label4.setId(5);
                s = new SpannableString("http://thomasokken.com/free42/42s.pdf");
                Linkify.addLinks(s, Linkify.WEB_URLS);
                label4.setText(s);
                label4.setMovementMethod(LinkMovementMethod.getInstance());
                lp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
                lp.addRule(RelativeLayout.ALIGN_LEFT, label3.getId());
                lp.addRule(RelativeLayout.BELOW, label3.getId());
                addView(label4, lp);

                Button okB = new Button(context);
                okB.setId(6);
                okB.setText("   OK   ");
                okB.setOnClickListener(new OnClickListener() {
                    public void onClick(View view) {
                        AboutDialog.this.hide();
                    }
                });
                lp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
                lp.addRule(RelativeLayout.BELOW, label4.getId());
                lp.addRule(RelativeLayout.CENTER_HORIZONTAL);
                addView(okB, lp);

            }
        }
    }
    /**
     * This class is calculator view used by the Free42 Activity.
     * Note that most of the heavy lifting takes place in the
     * Activity, not here.
     */
    private class CalcView extends View {
        
        private int width, height;
        private float hScale, vScale;
        private int hOffset, vOffset;
        private boolean possibleMenuEvent = false;

        public CalcView(Context context) {
            super(context);
        }
        
        public void updateScale() {
            vScale = ((float) height) / skin.getHeight();
            hScale = ((float) width) / skin.getWidth();
            hOffset = vOffset = 0;
            if (skin.getMaintainSkinAspect()) {
                if (hScale > vScale) {
                    hScale = vScale = ((float) height) / skin.getHeight();
                    hOffset = (int) ((width - skin.getWidth() * hScale) / 2);
                } else {
                    hScale = vScale = ((float) width) / skin.getWidth();
                    vOffset = (int) ((height - skin.getHeight() * vScale) / 2);
                }
            }
        }

        @Override
        protected void onSizeChanged(int w, int h, int oldw, int oldh) {
            width = w;
            height = h;
            updateScale();
        }

        @Override
        protected void onDraw(Canvas canvas) {
            canvas.translate(hOffset, vOffset);
            canvas.scale(hScale, vScale);
            skin.repaint(canvas);
        }
        
        @SuppressLint("ClickableViewAccessibility")
        @Override
        public boolean onTouchEvent(MotionEvent e) {
            int what = e.getAction();
            if (what != MotionEvent.ACTION_DOWN && what != MotionEvent.ACTION_UP)
                return true;
            
            cancelRepeaterAndTimeouts1And2();
            
            if (what == MotionEvent.ACTION_DOWN) {
                int x = (int) ((e.getX() - hOffset) / hScale);
                int y = (int) ((e.getY() - vOffset) / vScale);
                IntHolder skeyHolder = new IntHolder();
                IntHolder ckeyHolder = new IntHolder();
                skin.find_key(core_menu(), x, y, skeyHolder, ckeyHolder);
                int skey = skeyHolder.value;
                ckey = ckeyHolder.value;
                if (ckey == 0) {
                    if (skin.in_menu_area(x, y))
                        this.possibleMenuEvent = true;
                    return true;
                }
                click();
                end_core_keydown();
                byte[] macro = skin.find_macro(ckey);
                if (timeout3_active && (macro != null || ckey != 28 /* SHIFT */)) {
                    cancelTimeout3();
                    core_timeout3(0);
                }
                Rect inval = skin.set_active_key(skey);
                if (inval != null)
                    invalidateScaled(inval);
                boolean running;
                BooleanHolder enqueued = new BooleanHolder();
                IntHolder repeat = new IntHolder();
                if (macro == null) {
                    // Plain ol' key
                    running = core_keydown(ckey, enqueued, repeat, true);
                } else {
                    boolean one_key_macro = macro.length == 1 || (macro.length == 2 && macro[0] == 28);
                    if (!one_key_macro)
                        skin.set_display_enabled(false);
                    for (int i = 0; i < macro.length - 1; i++) {
                        core_keydown(macro[i] & 255, enqueued, repeat, true);
                        if (!enqueued.value)
                            core_keyup();
                    }
                    running = core_keydown(macro[macro.length - 1] & 255, enqueued, repeat, true);
                    if (!one_key_macro)
                        skin.set_display_enabled(true);
                }
                if (running)
                    start_core_keydown();
                else {
                    if (repeat.value != 0)
                        mainHandler.postDelayed(repeaterCaller, repeat.value == 1 ? 1000 : 500);
                    else if (!enqueued.value)
                        mainHandler.postDelayed(timeout1Caller, 250);
                }
            } else {
                if (possibleMenuEvent) {
                    possibleMenuEvent = false;
                    int x = (int) ((e.getX() - hOffset) / hScale);
                    int y = (int) ((e.getY() - vOffset) / vScale);
                    if (skin.in_menu_area(x, y))
                        Free42Activity.this.postMainMenu();
                }
                ckey = 0;
                Rect inval = skin.set_active_key(-1);
                if (inval != null)
                    invalidateScaled(inval);
                end_core_keydown();
                coreWantsCpu = core_keyup();
                if (coreWantsCpu)
                    start_core_keydown();
            }
                
            return true;
        }
        
        public void postInvalidateScaled(int left, int top, int right, int bottom) {
            left = (int) Math.floor(((double) left) * hScale + hOffset);
            top = (int) Math.floor(((double) top) * vScale + vOffset);
            right = (int) Math.ceil(((double) right) * hScale+ hOffset);
            bottom = (int) Math.ceil(((double) bottom) * vScale + vOffset);
            postInvalidate(left - 1, top - 1, right + 2, bottom + 2);
        }

        private void invalidateScaled(Rect inval) {
            inval.left = (int) Math.floor(((double) inval.left) * hScale + hOffset);
            inval.top = (int) Math.floor(((double) inval.top) * vScale + vOffset);
            inval.right = (int) Math.ceil(((double) inval.right) * hScale + hOffset);
            inval.bottom = (int) Math.ceil(((double) inval.bottom) * vScale + vOffset);
            inval.inset(-1, -1);
            invalidate(inval);
        }
    }

    /**
     * This class is the print-out view used by the Free42 Activity.
     * Note that most of the heavy lifting takes place in the
     * Activity, not here.
     */
    private class PrintView extends View {
        
        private static final int BYTESPERLINE = 18;
        // Certain devices have trouble with LINES = 16384; the print-out view collapses.
        // No idea how to detect this behavior, so unclear how to work around it.
        // Playing safe by making the print-out buffer smaller.
        // private static final int LINES = 16384;
        private static final int LINES = 8192;
        
        private byte[] buffer = new byte[LINES * BYTESPERLINE];
        private int top, bottom;
        private int printHeight;
        private int scale;

        public PrintView(Context context) {
            super(context);
            InputStream printInputStream = null;
            try {
                printInputStream = openFileInput("print");
                byte[] intBuf = new byte[4];
                if (printInputStream.read(intBuf) != 4)
                    throw new IOException();
                int len = (intBuf[0] << 24) | ((intBuf[1] & 255) << 16) | ((intBuf[2] & 255) << 8) | (intBuf[3] & 255);
                int maxlen = (LINES - 1) * BYTESPERLINE;
                if (len > maxlen) {
                    printInputStream.skip(len - maxlen);
                    len = maxlen;
                }
                int n = printInputStream.read(buffer, 0, len);
                if (n != len)
                    throw new IOException();
                top = 0;
                bottom = len;
            } catch (IOException e) {
                top = bottom = 0;
            } finally {
                if (printInputStream != null)
                    try {
                        printInputStream.close();
                    } catch (IOException e2) {}
            }

            printHeight = bottom / BYTESPERLINE;
            int screenWidth = getWindowManager().getDefaultDisplay().getWidth();
            scale = screenWidth / 143;
            if (scale == 0)
                scale = 1;
        }

        @Override
        protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
            // Pretending our height is never zero, to keep the HTC Aria
            // from throwing a fit. See also the printHeight == 0 case in
            // onDraw().
            setMeasuredDimension(143 * scale, Math.max(printHeight, 1) * scale);
        }

        @SuppressLint("DrawAllocation")
        @Override
        protected void onDraw(Canvas canvas) {
            Rect clip = canvas.getClipBounds();
            
            if (printHeight == 0) {
                // onMeasure() pretends that our height isn't really zero
                // even if printHeight == 0; this is to prevent the HTC Aria
                // from freaking out. Because of this pretense, we now have
                // to paint something, even though there isn't anything to
                // paint... So we just paint the clip rectangle using the
                // scroll view's background color.
                Paint p = new Paint();
                p.setColor(PRINT_BACKGROUND_COLOR);
                p.setStyle(Paint.Style.FILL);
                canvas.drawRect(clip, p);
                return;
            }
            
            // Extend the clip rectangle so that it doesn't include any
            // fractional pixels
            clip.left = clip.left / scale * scale;
            clip.top = clip.top / scale * scale;
            clip.right = (clip.right + scale - 1) / scale * scale;
            clip.bottom = (clip.bottom + scale - 1) / scale * scale;
            
            // Construct a temporary bitmap
            int src_x = clip.left / scale;
            int src_y = clip.top / scale;
            int src_width = (clip.right - clip.left) / scale;
            int src_height = (clip.bottom - clip.top) / scale;
            Bitmap tmpBitmap = Bitmap.createBitmap(src_width, src_height, Bitmap.Config.ARGB_8888);
            IntBuffer tmpBuffer = IntBuffer.allocate(src_width * src_height);
            int[] tmpArray = tmpBuffer.array();
            for (int y = 0; y < src_height; y++) {
                int yy = y + src_y + (top / BYTESPERLINE);
                if (yy >= LINES)
                    yy -= LINES;
                for (int x = 0; x < src_width; x++) {
                    int xx = x + src_x;
                    boolean set = (buffer[yy * BYTESPERLINE + (xx >> 3)] & (1 << (xx & 7))) != 0;
                    tmpArray[y * src_width + x] = set ? Color.BLACK : Color.WHITE;
                }
            }
            tmpBitmap.copyPixelsFromBuffer(tmpBuffer);
            canvas.drawBitmap(tmpBitmap, new Rect(0, 0, src_width, src_height), clip, new Paint());
        }
        
        @SuppressLint("ClickableViewAccessibility")
        @Override
        public boolean onTouchEvent(MotionEvent e) {
            return true;
        }
        
        @Override
        public void onSizeChanged(int w, int h, int oldw, int oldh) {
            printScrollView.fullScroll(View.FOCUS_DOWN);
        }
        
        private Object invalidatePendingMonitor = new Object();
        private boolean invalidatePending;
        private Object layoutPendingMonitor = new Object();
        private boolean layoutPending;
        
        public void print(byte[] bits, int bytesperline, int x, int y, int width, int height) {
            int oldPrintHeight = printHeight;
            for (int yy = y; yy < y + height; yy++) {
                for (int xx = 0; xx < BYTESPERLINE; xx++)
                    buffer[bottom + xx] = 0;
                for (int xx = x; xx < x + width; xx++) {
                    boolean set = (bits[yy * bytesperline + (xx >> 3)] & (1 << (xx & 7))) != 0;
                    if (set)
                        buffer[bottom + (xx >> 3)] |= 1 << (xx & 7);
                }
                bottom += BYTESPERLINE;
                printHeight++;
                if (bottom >= buffer.length)
                    bottom = 0;
                if (bottom == top) {
                    top += BYTESPERLINE;
                    printHeight--;
                    if (top >= buffer.length)
                        top = 0;
                }
            }
            if (printHeight != oldPrintHeight) {
                synchronized (layoutPendingMonitor) {
                    if (!layoutPending) {
                        mainHandler.post(new Runnable() {
                            public void run() {
                                synchronized (layoutPendingMonitor) {
                                    printView.requestLayout();
                                    layoutPending = false;
                                }
                            }
                        });
                        layoutPending = true;
                    }
                }
            } else {
                synchronized (invalidatePendingMonitor) {
                    if (!invalidatePending) {
                        mainHandler.post(new Runnable() {
                            public void run() {
                                synchronized (invalidatePendingMonitor) {
                                    printScrollView.fullScroll(View.FOCUS_DOWN);
                                    printView.postInvalidate();
                                    invalidatePending = false;
                                }
                            }
                        });
                        invalidatePending = true;
                    }
                }
            }
        }
        
        public void clear() {
            top = bottom = 0;
            printHeight = 0;
            printView.requestLayout();
        }
        
        public void dump() {
            OutputStream printOutputStream = null;
            try {
                printOutputStream = openFileOutput("print", Context.MODE_PRIVATE);
                int len = bottom - top;
                if (len < 0)
                    len += buffer.length;
                byte[] intBuf = new byte[4];
                intBuf[0] = (byte) (len >> 24);
                intBuf[1] = (byte) (len >> 16);
                intBuf[2] = (byte) (len >> 8);
                intBuf[3] = (byte) len;
                printOutputStream.write(intBuf);
                if (top <= bottom)
                    printOutputStream.write(buffer, top, bottom - top);
                else {
                    printOutputStream.write(buffer, top, buffer.length - top);
                    printOutputStream.write(buffer, 0, bottom);
                }
            } catch (IOException e) {
                // Ignore
            } finally {
                if (printOutputStream != null)
                    try {
                        printOutputStream.close();
                    } catch (IOException e2) {}
            }
        }
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
            ShellSpool.printToGif = state_read_boolean();
            ShellSpool.printToGifFileName = state_read_string();
            ShellSpool.printToTxt = state_read_boolean();
            ShellSpool.printToTxtFileName = state_read_string();
            if (shell_version >= 1)
                ShellSpool.maxGifHeight = state_read_int();
            if (shell_version >= 2)
                skinName[0] = state_read_string();
            if (shell_version >= 3)
                externalSkinName[0] = state_read_string();
            if (shell_version >= 4) {
                skinName[1] = state_read_string();
                externalSkinName[1] = state_read_string();
                keyClicksEnabled = state_read_boolean();
            } else {
                skinName[1] = skinName[0];
                externalSkinName[1] = externalSkinName[0];
                keyClicksEnabled = true;
            }
            if (shell_version >= 5)
                preferredOrientation = state_read_int();
            else
                preferredOrientation = ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED;
            if (shell_version >= 6) {
                skinSmoothing[0] = state_read_boolean();
                displaySmoothing[0] = state_read_boolean();
            }
            if (shell_version >= 7) {
                skinSmoothing[1] = state_read_boolean();
                displaySmoothing[1] = state_read_boolean();
            }
            if (shell_version >= 8)
                keyVibrationEnabled = state_read_boolean();
            if (shell_version >= 9) {
                style = state_read_int();
                int maxStyle = PreferencesDialog.immersiveModeSupported ? 2 : 1;
                if (style > maxStyle)
                    style = maxStyle;
            } else
                style = 0;
            if (shell_version >= 10)
                alwaysRepaintFullDisplay = state_read_boolean();
            if (shell_version >= 11)
                alwaysOn = state_read_boolean();
            if (shell_version >= 13) {
                maintainSkinAspect[0] = state_read_boolean();
                maintainSkinAspect[1] = state_read_boolean();
            }
            init_shell_state(shell_version);
        } catch (IllegalArgumentException e) {
            return false;
        }
        return true;
    }
    
    private void init_shell_state(int shell_version) {
        switch (shell_version) {
        case -1:
            ShellSpool.printToGif = false;
            ShellSpool.printToGifFileName = "";
            ShellSpool.printToTxt = false;
            ShellSpool.printToTxtFileName = "";
            // fall through
        case 0:
            ShellSpool.maxGifHeight = 256;
            // fall through
        case 1:
            skinName[0] = "Standard";
            // fall through
        case 2:
            externalSkinName[0] = topStorageDir() + "/Free42/" + skinName[0];
            // fall through
        case 3:
            skinName[1] = "Landscape";
            externalSkinName[1] = topStorageDir() + "/Free42/" + skinName[1];
            keyClicksEnabled = true;
            // fall through
        case 4:
            preferredOrientation = ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED;
            // fall through
        case 5:
            skinSmoothing[0] = true;
            displaySmoothing[0] = false;
            // fall through
        case 6:
            skinSmoothing[1] = skinSmoothing[0];
            displaySmoothing[1] = displaySmoothing[0];
            // fall through
        case 7:
            keyVibrationEnabled = false;
            // fall through
        case 8:
            style = 0;
            // fall through
        case 9:
            alwaysRepaintFullDisplay = false;
            // fall through
        case 10:
            alwaysOn = false;
            // fall through
        case 11:
            // fall through
        case 12:
            maintainSkinAspect[0] = false;
            maintainSkinAspect[1] = false;
            // fall through
        case 13:
            // current version (SHELL_VERSION = 13),
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
            state_write_boolean(ShellSpool.printToGif);
            state_write_string(ShellSpool.printToGifFileName);
            state_write_boolean(ShellSpool.printToTxt);
            state_write_string(ShellSpool.printToTxtFileName);
            state_write_int(ShellSpool.maxGifHeight);
            state_write_string(skinName[0]);
            state_write_string(externalSkinName[0]);
            state_write_string(skinName[1]);
            state_write_string(externalSkinName[1]);
            state_write_boolean(keyClicksEnabled);
            state_write_int(preferredOrientation);
            state_write_boolean(skinSmoothing[0]);
            state_write_boolean(displaySmoothing[0]);
            state_write_boolean(skinSmoothing[1]);
            state_write_boolean(displaySmoothing[1]);
            state_write_boolean(keyVibrationEnabled);
            state_write_int(style);
            state_write_boolean(alwaysRepaintFullDisplay);
            state_write_boolean(alwaysOn);
            state_write_boolean(maintainSkinAspect[0]);
            state_write_boolean(maintainSkinAspect[1]);
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
        public boolean coreWantsCpu;
        public void run() {
            BooleanHolder enqueued = new BooleanHolder();
            IntHolder repeat = new IntHolder();
            coreWantsCpu = core_keydown(0, enqueued, repeat, false);
        }
    }
    
    private void start_core_keydown() {
        coreThread = new CoreThread();
        coreThread.start();
    }
    
    private void end_core_keydown() {
        if (coreThread != null) {
            core_keydown_finish();
            try {
                coreThread.join();
            } catch (InterruptedException e) {}
            coreWantsCpu = coreThread.coreWantsCpu;
            coreThread = null;
        } else {
            coreWantsCpu = false;
        }
    }
    
    private void repeater() {
        cancelRepeaterAndTimeouts1And2();
        if (ckey == 0)
            return;
        int repeat = core_repeat();
        if (repeat != 0)
            mainHandler.postDelayed(repeaterCaller, repeat == 1 ? 200 : 100);
        else
            mainHandler.postDelayed(timeout1Caller, 250);
    }

    private void timeout1() {
        cancelRepeaterAndTimeouts1And2();
        if (ckey != 0) {
            core_keytimeout1();
            mainHandler.postDelayed(timeout2Caller, 1750);
        }
    }

    private void timeout2() {
        cancelRepeaterAndTimeouts1And2();
        if (ckey != 0)
            core_keytimeout2();
    }

    private void timeout3() {
        cancelTimeout3();
        end_core_keydown();
        core_timeout3(1);
        // Resume program after PSE
        BooleanHolder enqueued = new BooleanHolder();
        IntHolder repeat = new IntHolder();
        boolean running = core_keydown(0, enqueued, repeat, true);
        if (running)
            start_core_keydown();
    }
    
    private void click() {
        if (keyClicksEnabled)
            playSound(11, 0);
        if (keyVibrationEnabled) {
            Vibrator v = (Vibrator) getSystemService(Context.VIBRATOR_SERVICE);
            v.vibrate(50);
        }
    }
    
    
    private void playSound(int index, int duration) {
        soundPool.play(soundIds[index], 1f, 1f, 0, 0, 1f);
    }
    
    private static String topStorageDir() {
        return Environment.getExternalStorageDirectory().getAbsolutePath();
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
    private native void core_enter_background();
    private native void core_quit();
    private native void core_repaint_display();
    private native boolean core_menu();
    //private native boolean core_alpha_menu();
    //private native boolean core_hex_menu();
    private native boolean core_keydown(int key, BooleanHolder enqueued, IntHolder repeat, boolean immediate_return);
    private native int core_repeat();
    private native void core_keytimeout1();
    private native void core_keytimeout2();
    private native boolean core_timeout3(int repaint);
    private native boolean core_keyup();
    private native boolean core_powercycle();
    private native String[] core_list_programs();
    //private native int core_program_size(int prgm_index);
    private native void core_export_programs(int[] indexes);
    private native void core_import_programs();
    private native String core_copy();
    private native void core_paste(String s);
    private native void getCoreSettings(CoreSettings settings);
    private native void putCoreSettings(CoreSettings settings);
    private native void redisplay();
    private native void setAlwaysRepaintFullDisplay(boolean alwaysRepaint);

    private static class CoreSettings {
        public boolean matrix_singularmatrix;
        public boolean matrix_outofrange;
        public boolean auto_repeat;
        @SuppressWarnings("unused") public boolean enable_ext_accel;
        @SuppressWarnings("unused") public boolean enable_ext_locat;
        @SuppressWarnings("unused") public boolean enable_ext_heading;
        @SuppressWarnings("unused") public boolean enable_ext_time;
        @SuppressWarnings("unused") public boolean enable_ext_fptest;
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
        Rect inval = skin.display_blitter(bits, bytesperline, x, y, width, height);
        calcView.postInvalidateScaled(inval.left, inval.top, inval.right, inval.bottom);
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
        int sound_number = 10;
        for (int i = 0; i < 10; i++) {
            if (frequency <= cutoff_freqs[i]) {
                sound_number = i;
                break;
            }
        }
        playSound(sound_number, sound_number == 10 ? 125 : 250);
        try {
            Thread.sleep(sound_number == 10 ? 125 : 250);
        } catch (InterruptedException e) {}
    }

    private final int[] cutoff_freqs = { 164, 220, 243, 275, 293, 324, 366, 418, 438, 550 };
    
    private PrintAnnunciatorTurnerOffer pato = null;

    private class PrintAnnunciatorTurnerOffer extends Thread {
        public void run() {
            try {
                Thread.sleep(1000);
            } catch (InterruptedException e) {
                return;
            } finally {
                if (pato == this)
                    pato = null;
                else
                    return;
            }
            // Don't invalidate if the skin is null, which could happen
            // if we're in the process of switching between portrait and
            // landscape modes.
            SkinLayout currentSkin = skin;
            if (currentSkin != null) {
                Rect inval = currentSkin.update_annunciators(-1, -1, 0, -1, -1, -1, -1);
                if (inval != null)
                    calcView.postInvalidateScaled(inval.left, inval.top, inval.right, inval.bottom);
            }
        }
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
        boolean prt_off = false;
        if (prt != -1) {
            PrintAnnunciatorTurnerOffer p = pato;
            pato = null;
            if (p != null)
                p.interrupt();
            if (prt == 0) {
                prt = -1;
                prt_off = true;
            }
        }
        Rect inval = skin.update_annunciators(updn, shf, prt, run, -1, g, rad);
        if (inval != null)
            calcView.postInvalidateScaled(inval.left, inval.top, inval.right, inval.bottom);
        if (prt_off) {
            pato = new PrintAnnunciatorTurnerOffer();
            pato.start();
        }
    }
    
    /**
     * Callback to ask the shell to call core_timeout3() after the given number of
     * milliseconds. If there are keystroke events during that time, the timeout is
     * cancelled. (Pressing 'shift' does not cancel the timeout.)
     * This function supports the delay after SHOW, MEM, and shift-VARMENU.
     */
    public void shell_request_timeout3(int delay) {
        cancelTimeout3();
        mainHandler.postDelayed(timeout3Caller, delay);
        timeout3_active = true;
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
     * battery annunciator.
     */
    public int shell_low_battery() {
        return low_battery ? 1 : 0;
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
    
    private class AlwaysOnSetter implements Runnable {
        private boolean set;
        public AlwaysOnSetter(boolean set) {
            this.set = set;
        }
        public void run() {
            if (set)
                getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
            else
                getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        }
    }
    
    /**
     * shell_always_on()
     * Callback for setting and querying the shell's Continuous On status.
     */
    public int shell_always_on(int ao) {
        int ret = alwaysOn ? 1 : 0;
        if (ao != -1) {
            alwaysOn = ao != 0;
            runOnUiThread(new AlwaysOnSetter(alwaysOn));
        }
        return ret;
    }
    
    /**
     * shell_decimal_point()
     * Returns 0 if the host's locale uses comma as the decimal separator;
     * returns 1 if it uses dot or anything else.
     * Used to initialize flag 28 on hard reset.
     */
    public int shell_decimal_point() {
        DecimalFormat df = new DecimalFormat();
        DecimalFormatSymbols dfsym = df.getDecimalFormatSymbols();
        return dfsym.getDecimalSeparator() == ',' ? 0 : 1;
    }
    
    private OutputStream printTxtStream;
    private RandomAccessFile printGifFile;
    private int gif_lines;
    private int gif_seq = 0;
    
    /**
     * shell_print()
     * Printer emulation. The first 2 parameters are the plain text version of the
     * data to be printed; the remaining 6 parameters are the bitmap version. The
     * former is used for text-mode copying and for spooling to text files; the
     * latter is used for graphics-mode copying, spooling to image files, and
     * on-screen display.
     */
    public void shell_print(byte[] text, byte[] bits, int bytesperline,
                            int x, int y, int width, int height) {
        printView.print(bits, bytesperline, x, y, width, height);

        if (ShellSpool.printToTxt) {
            try {
                if (printTxtStream == null)
                    if (new File(ShellSpool.printToTxtFileName).exists()) {
                        printTxtStream = new FileOutputStream(ShellSpool.printToTxtFileName, true);
                    } else {
                        printTxtStream = new FileOutputStream(ShellSpool.printToTxtFileName);
                        printTxtStream.write(new byte[] { (byte) 0xEF, (byte) 0xBB, (byte) 0xBF });
                    }
                ShellSpool.shell_spool_txt(text, printTxtStream);
            } catch (IOException e) {
                if (printTxtStream != null) {
                    try {
                        printTxtStream.close();
                    } catch (IOException e2) {}
                    printTxtStream = null;
                }
                ShellSpool.printToTxt = false;
                alert("An error occurred while printing to " + ShellSpool.printToTxtFileName + ": " + e.getMessage()
                        + "\nPrinting to text file disabled.");
            }
        }
        
        if (ShellSpool.printToGif) {
            if (printGifFile != null && gif_lines + height > ShellSpool.maxGifHeight) {
                try {
                    ShellSpool.shell_finish_gif(printGifFile);
                } catch (IOException e) {}
                try {
                    printGifFile.close();
                } catch (IOException e) {}
                printGifFile = null;
            }

            String name = null;
            if (printGifFile == null) {
                while (true) {
                    gif_seq = (gif_seq + 1) % 10000;
    
                    name = ShellSpool.printToGifFileName;
                    int len = name.length();

                    /* Strip ".gif" extension, if present */
                    if (len >= 4 && name.substring(len - 4).equals(".gif")) {
                        name = name.substring(0, len - 4);
                        len -= 4;
                    }
    
                    /* Strip ".[0-9]+", if present */
                    while (len > 0) {
                        char c = name.charAt(len - 1);
                        if (c >= '0' && c <= '9')
                            name = name.substring(0, --len);
                        else
                            break;
                    }
                    if (len > 0 && name.charAt(len - 1) == '.')
                        name = name.substring(0, --len);

                    String seq = "000" + gif_seq;
                    seq = seq.substring(seq.length() - 4);
                    name += "." + seq + ".gif";
    
                    if (!new File(name).exists())
                        break;
                }
            }

            try {
                if (name != null) {
                    printGifFile = new RandomAccessFile(name, "rw");
                    gif_lines = 0;
                    ShellSpool.shell_start_gif(printGifFile, ShellSpool.maxGifHeight);
                }
                ShellSpool.shell_spool_gif(bits, bytesperline, x, y, width, height, printGifFile);
                gif_lines += height;
            } catch (IOException e) {
                if (printGifFile != null) {
                    try {
                        printGifFile.close();
                    } catch (IOException e2) {}
                    printGifFile = null;
                }
                ShellSpool.printToGif = false;
                alert("An error occurred while printing to " + ShellSpool.printToGifFileName + ": " + e.getMessage()
                        + "\nPrinting to GIF file disabled.");
            }

            if (printGifFile != null && gif_lines + 9 > ShellSpool.maxGifHeight) {
                try {
                    ShellSpool.shell_finish_gif(printGifFile);
                } catch (IOException e) {}
                try {
                    printGifFile.close();
                } catch (IOException e) {}
                printGifFile = null;
            }
        }
    }
    
    /**
     * shell_write()
     *
     * Callback for core_export_programs(). Returns 0 if a problem occurred;
     * core_export_programs() should abort in that case.
     */
    public int shell_write(byte[] buf) {
        if (programsOutputStream == null)
            return 0;
        try {
            programsOutputStream.write(buf);
            return 1;
        } catch (IOException e) {
            try {
                programsOutputStream.close();
            } catch (IOException e2) {}
            programsOutputStream = null;
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
        if (programsInputStream == null)
            return -1;
        try {
            int n = programsInputStream.read(buf);
            if (n <= 0) {
                programsInputStream.close();
                programsInputStream = null;
                return 0;
            } else
                return n;
        } catch (IOException e) {
            try {
                programsInputStream.close();
            } catch (IOException e2) {}
            programsInputStream = null;
            return -1;
        }
    }
    
    private boolean accel_inited, accel_exists;
    private double accel_x, accel_y, accel_z;
    
    public int shell_get_acceleration(DoubleHolder x, DoubleHolder y, DoubleHolder z) {
        if (!accel_inited) {
            accel_inited = true;
            SensorManager sm = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
            Sensor s = sm.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
            if (s == null)
                return 0;
            boolean success = sm.registerListener(new SensorEventListener() {
                        public void onAccuracyChanged(Sensor sensor, int accuracy) {
                            // Don't care
                        }
                        public void onSensorChanged(SensorEvent event) {
                            // Transform the measurements to conform to the iPhone
                            // conventions. The conversion factor used here is the
                            // 'standard gravity'.
                            accel_x = event.values[0] / -9.80665;
                            accel_y = event.values[1] / -9.80665;
                            accel_z = event.values[2] / -9.80665;
                        }
                    }, s, SensorManager.SENSOR_DELAY_NORMAL);
            if (!success)
                return 0;
            accel_exists = true;
        }
        
        if (accel_exists) {
            x.value = accel_x;
            y.value = accel_y;
            z.value = accel_z;
            return 1;
        } else {
            return 0;
        }
    }

    private boolean locat_inited, locat_exists;
    private double locat_lat, locat_lon, locat_lat_lon_acc, locat_elev, locat_elev_acc;
    
    public int shell_get_location(DoubleHolder lat, DoubleHolder lon, DoubleHolder lat_lon_acc, DoubleHolder elev, DoubleHolder elev_acc) {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            locat_inited = false;
            ActivityCompat.requestPermissions(this, new String[] { Manifest.permission.ACCESS_FINE_LOCATION }, MY_PERMISSIONS_REQUEST_ACCESS_FINE_LOCATION);
            return 0;
        }
        if (!locat_inited) {
            locat_inited = true;
            LocationManager lm = (LocationManager) getSystemService(Context.LOCATION_SERVICE);
            Criteria cr = new Criteria();
            cr.setAccuracy(Criteria.ACCURACY_FINE);
            String provider = lm.getBestProvider(cr, true);
            if (provider == null) {
                locat_exists = false;
                return 0;
            }
            LocationListener ll = new LocationListener() {
                public void onLocationChanged(Location location) {
                    // TODO: Verify units etc.
                    locat_lat = location.getLatitude();
                    locat_lon = location.getLongitude();
                    locat_lat_lon_acc = location.getAccuracy();
                    locat_elev = location.getAltitude();
                    locat_elev_acc = location.hasAltitude() ? locat_lat_lon_acc : -1;
                }
                public void onProviderDisabled(String provider) {
                    // Ignore
                }
                public void onProviderEnabled(String provider) {
                    // Ignore
                }
                public void onStatusChanged(String provider, int status,
                        Bundle extras) {
                    // Ignore
                }
            };
            try {
                lm.requestLocationUpdates(provider, 60000, 1, ll, Looper.getMainLooper());
            } catch (IllegalArgumentException e) {
                return 0;
            } catch (SecurityException e) {
                return 0;
            }
            locat_exists = true;
        }
        
        if (locat_exists) {
            lat.value = locat_lat;
            lon.value = locat_lon;
            lat_lon_acc.value = locat_lat_lon_acc;
            elev.value = locat_elev;
            elev_acc.value = locat_elev_acc;
            return 1;
        } else
            return 0;
    }
    
    private boolean heading_inited, heading_exists;
    private double heading_mag, heading_true, heading_acc, heading_x, heading_y, heading_z;
    
    public int shell_get_heading(DoubleHolder mag_heading, DoubleHolder true_heading, DoubleHolder acc_heading, DoubleHolder x, DoubleHolder y, DoubleHolder z) {
        if (!heading_inited) {
            heading_inited = true;
            SensorManager sm = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
            Sensor s1 = sm.getDefaultSensor(Sensor.TYPE_ORIENTATION);
            Sensor s2 = sm.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD);
            if (s1 == null)
                return 0;
            SensorEventListener listener = new SensorEventListener() {
                        public void onAccuracyChanged(Sensor sensor, int accuracy) {
                            // Don't care
                        }
                        public void onSensorChanged(SensorEvent event) {
                            // TODO: Verify this on a real phone, and
                            // check if the orientation matches the iPhone.
                            // There doesn't seem to be an API to obtain true
                            // heading, so I should set true_heading to 0
                            // and heading_acc to -1; the current code just
                            // exists to let me investigate the components
                            // returned by Orientation events.
                            if (event.sensor.getType() == Sensor.TYPE_ORIENTATION) {
                                heading_mag = event.values[0];
                                heading_true = event.values[1];
                                heading_acc = event.values[2];
                            } else {
                                heading_x = event.values[0];
                                heading_y = event.values[1];
                                heading_z = event.values[2];
                            }
                        }
                    };
            boolean success = sm.registerListener(listener, s1, SensorManager.SENSOR_DELAY_UI);
            if (!success)
                return 0;
            sm.registerListener(listener, s2, SensorManager.SENSOR_DELAY_UI);
            heading_exists = true;
        }
        
        if (heading_exists) {
            mag_heading.value = heading_mag;
            true_heading.value = heading_true;
            acc_heading.value = heading_acc;
            x.value = heading_x;
            y.value = heading_y;
            z.value = heading_z;
            return 1;
        } else {
            return 0;
        }
    }
    
    public void shell_log(String s) {
        System.err.print(s);
    }
    
    public static boolean checkStorageAccess() {
        return instance.checkStorageAccess2();
    }
    
    private boolean checkStorageAccess2() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED) {
            if (android.os.Build.VERSION.SDK_INT >= 19 /* KitKat; 4.4 */)
                new File(MY_STORAGE_DIR).mkdirs();
            return true;
        }
        ActivityCompat.requestPermissions(this, new String[] { Manifest.permission.WRITE_EXTERNAL_STORAGE }, MY_PERMISSIONS_REQUEST_ACCESS_FINE_LOCATION);
        return false;
    }
}
