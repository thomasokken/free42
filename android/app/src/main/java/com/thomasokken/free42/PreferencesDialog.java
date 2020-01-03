/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2020  Thomas Okken
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

import android.app.Dialog;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.os.Vibrator;
import android.view.View;
import android.view.WindowManager;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.SeekBar;
import android.widget.Spinner;

public class PreferencesDialog extends Dialog {
    private static boolean reversePortraitSupported;
    private static final int reversePortraitConstant;
    public static boolean immersiveModeSupported;
    public static final int immersiveModeFlags;
    
    static {
        reversePortraitSupported = true;
        int tmp = 0;
        try {
            tmp = ActivityInfo.class.getField("SCREEN_ORIENTATION_REVERSE_PORTRAIT").getInt(null);
        } catch (Exception e) {
            reversePortraitSupported = false;
        }
        reversePortraitConstant = tmp;
        
        immersiveModeSupported = true;
        tmp = 0;
        try {
            String[] fields = { "SYSTEM_UI_FLAG_LAYOUT_STABLE",
                                "SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION",
                                "SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN",
                                "SYSTEM_UI_FLAG_HIDE_NAVIGATION",
                                "SYSTEM_UI_FLAG_FULLSCREEN",
                                "SYSTEM_UI_FLAG_IMMERSIVE_STICKY" };
            for (String field : fields)
                tmp |= View.class.getField(field).getInt(null);
        } catch (Exception e) {
            immersiveModeSupported = false;
        }
        immersiveModeFlags = tmp;
    }
    
    private CheckBox singularMatrixCB;
    private CheckBox matrixOutOfRangeCB;
    private CheckBox autoRepeatCB;
    private CheckBox alwaysOnCB;
    private SeekBar keyClicksSB;
    private SeekBar hapticSB;
    private Spinner orientationSP;
    private Spinner styleSP;
    private CheckBox maintainSkinAspectCB;
    private CheckBox skinSmoothingCB;
    private CheckBox displaySmoothingCB;
    private CheckBox displayFullRepaintCB;
    private CheckBox printToTextCB;
    private EditText printToTextFileNameTF;
    private CheckBox printToGifCB;
    private EditText printToGifFileNameTF;
    private EditText maxGifHeightTF;
    private OkListener okListener;
    
    public PreferencesDialog(Context context) {
        super(context);
        setContentView(R.layout.preferences_dialog);
        getWindow().setLayout(WindowManager.LayoutParams.MATCH_PARENT,
                WindowManager.LayoutParams.MATCH_PARENT);
        singularMatrixCB = (CheckBox) findViewById(R.id.singularMatrixCB);
        matrixOutOfRangeCB = (CheckBox) findViewById(R.id.matrixOutOfRangeCB);
        autoRepeatCB = (CheckBox) findViewById(R.id.autoRepeatCB);
        alwaysOnCB = (CheckBox) findViewById(R.id.alwaysOnCB);
        keyClicksSB = (SeekBar) findViewById(R.id.keyClicksSB);
        keyClicksSB.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            private int prevVal = -1;
            @Override
            public void onProgressChanged(SeekBar seekBar, int val, boolean fromUser) {
                if (fromUser) {
                    if (val != prevVal) {
                        if (val > 0)
                            Free42Activity.instance.playSound(val + 10, 0);
                        prevVal = val;
                    }
                }
            }
            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                // ignore
            }
            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                // ignore
            }
        });
        hapticSB = (SeekBar) findViewById(R.id.hapticSB);
        hapticSB.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            private int prevVal = -1;
            @Override
            public void onProgressChanged(SeekBar seekBar, int val, boolean fromUser) {
                if (fromUser) {
                    if (val != prevVal) {
                        if (val > 0) {
                            int ms = (int) (Math.pow(2, (val - 1) / 2.0) + 0.5);
                            Vibrator v = (Vibrator) PreferencesDialog.this.getContext().getSystemService(Context.VIBRATOR_SERVICE);
                            v.vibrate(ms);
                        }
                        prevVal = val;

                    }
                }
            }
            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                // ignore
            }
            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                // ignore
            }
        });
        orientationSP = (Spinner) findViewById(R.id.orientationSpinner);
        String[] values;
        if (reversePortraitSupported)
            values = new String[] { "Automatic", "Portrait", "Reverse Portrait", "Landscape" };
        else
            values = new String[] { "Automatic", "Portrait", "Landscape" };
        ArrayAdapter<String> aa = new ArrayAdapter<String>(context, android.R.layout.simple_spinner_item, values);
        orientationSP.setAdapter(aa);
        styleSP = (Spinner) findViewById(R.id.styleSpinner);
        if (immersiveModeSupported)
            values = new String[] { "Normal", "No Status", "Full Screen" };
        else
            values = new String[] { "Normal", "No Status" };
        aa = new ArrayAdapter<String>(context, android.R.layout.simple_spinner_item, values);
        styleSP.setAdapter(aa);
        maintainSkinAspectCB = (CheckBox) findViewById(R.id.maintainSkinAspectCB);
        skinSmoothingCB = (CheckBox) findViewById(R.id.skinSmoothingCB);
        displaySmoothingCB = (CheckBox) findViewById(R.id.displaySmoothingCB);
        displayFullRepaintCB = (CheckBox) findViewById(R.id.displayFullRepaintCB);
        printToTextCB = (CheckBox) findViewById(R.id.printToTextCB);
        Button browseTextB = (Button) findViewById(R.id.browseTextB);
        browseTextB.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                browseTextFileName(view.getContext());
            }
        });
        printToTextFileNameTF = (EditText) findViewById(R.id.printToTextFileNameTF);
        printToGifCB = (CheckBox) findViewById(R.id.printToGifCB);
        Button browseGifB = (Button) findViewById(R.id.browseGifB);
        browseGifB.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                browseGifFileName(view.getContext());
            }
        });
        printToGifFileNameTF = (EditText) findViewById(R.id.printToGifFileNameTF);
        maxGifHeightTF = (EditText) findViewById(R.id.maxGifHeightTF);
        Button okB = (Button) findViewById(R.id.okB);
        okB.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                if (okListener != null)
                    okListener.okPressed();
                PreferencesDialog.this.hide();
            }
        });
        Button cancelB = (Button) findViewById(R.id.cancelB);
        cancelB.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                PreferencesDialog.this.hide();
            }
        });
        setTitle("Preferences");
    }
    
    public interface OkListener {
        public void okPressed();
    }

    public void setOkListener(OkListener okListener) {
        this.okListener = okListener;
    }
    
    private void browseTextFileName(Context context) {
        if (!Free42Activity.checkStorageAccess())
            return;
        FileSelectionDialog fsd = new FileSelectionDialog(context, new String[] { "txt", "*" });
        fsd.setPath(printToTextFileNameTF.getText().toString());
        fsd.setOkListener(new FileSelectionDialog.OkListener() {
            public void okPressed(String path) {
                printToTextFileNameTF.setText(path);
            }
        });
        fsd.show();
    }
    
    private void browseGifFileName(Context context) {
        if (!Free42Activity.checkStorageAccess())
            return;
        FileSelectionDialog fsd = new FileSelectionDialog(context, new String[] { "gif", "*" });
        fsd.setPath(printToGifFileNameTF.getText().toString());
        fsd.setOkListener(new FileSelectionDialog.OkListener() {
            public void okPressed(String path) {
                printToGifFileNameTF.setText(path);
            }
        });
        fsd.show();
    }
    
    public void setSingularMatrixError(boolean b) {
        singularMatrixCB.setChecked(b);
    }
    
    public boolean getSingularMatrixError() {
        return singularMatrixCB.isChecked();
    }
    
    public void setMatrixOutOfRange(boolean b) {
        matrixOutOfRangeCB.setChecked(b);
    }
    
    public boolean getMatrixOutOfRange() {
        return matrixOutOfRangeCB.isChecked();
    }
    
    public void setAutoRepeat(boolean b) {
        autoRepeatCB.setChecked(b);
    }
    
    public boolean getAutoRepeat() {
        return autoRepeatCB.isChecked();
    }
    
    public void setAlwaysOn(boolean b) {
        alwaysOnCB.setChecked(b);
    }
    
    public boolean getAlwaysOn() {
        return alwaysOnCB.isChecked();
    }
    
    public void setKeyClicks(int v) {
        keyClicksSB.setProgress(v);
    }
    
    public int getKeyClicks() {
        return keyClicksSB.getProgress();
    }
    
    public void setKeyVibration(int ms) {
        hapticSB.setProgress(ms);
    }
    
    public int getKeyVibration() {
        return hapticSB.getProgress();
    }
    
    public void setOrientation(int orientation) {
        if (reversePortraitSupported) {
            if (orientation == ActivityInfo.SCREEN_ORIENTATION_PORTRAIT)
                orientation = 1;
            else if (orientation == reversePortraitConstant)
                orientation = 2;
            else if (orientation == ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE)
                orientation = 3;
            else
                orientation = 0;
        } else {
            if (orientation == ActivityInfo.SCREEN_ORIENTATION_PORTRAIT)
                orientation = 1;
            else if (orientation == ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE)
                orientation = 2;
            else
                orientation = 0;
        }
        orientationSP.setSelection(orientation);
    }
    
    public int getOrientation() {
        int orientation = orientationSP.getSelectedItemPosition();
        if (reversePortraitSupported)
            switch (orientation) {
                case 1: return ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
                case 2: return reversePortraitConstant;
                case 3: return ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
                case 0: default: return ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED;
            }
        else
            switch (orientation) {
                case 1: return ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
                case 2: return ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
                case 0: default: return ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED;
            }
    }
    
    public void setStyle(int style) {
        if (style == 2 && !immersiveModeSupported)
            style = 1;
        styleSP.setSelection(style);
    }
    
    public int getStyle() {
        return styleSP.getSelectedItemPosition();
    }
    
    public void setMaintainSkinAspect(boolean b) {
        maintainSkinAspectCB.setChecked(b);
    }
    
    public boolean getMaintainSkinAspect() {
        return maintainSkinAspectCB.isChecked();
    }
    
    public void setSkinSmoothing(boolean b) {
        skinSmoothingCB.setChecked(b);
    }
    
    public boolean getSkinSmoothing() {
        return skinSmoothingCB.isChecked();
    }
    
    public void setDisplaySmoothing(boolean b) {
        displaySmoothingCB.setChecked(b);
    }
    
    public boolean getDisplaySmoothing() {
        return displaySmoothingCB.isChecked();
    }
    
    public void setDisplayFullRepaint(boolean b) {
        displayFullRepaintCB.setChecked(b);
    }
    
    public boolean getDisplayFullRepaint() {
        return displayFullRepaintCB.isChecked();
    }
    
    public void setPrintToText(boolean b) {
        printToTextCB.setChecked(b);
    }
    
    public boolean getPrintToText() {
        return printToTextCB.isChecked();
    }
    
    public void setPrintToTextFileName(String s) {
        printToTextFileNameTF.setText(s);
    }
    
    public String getPrintToTextFileName() {
        return printToTextFileNameTF.getText().toString();
    }
    
    public void setPrintToGif(boolean b) {
        printToGifCB.setChecked(b);
    }
    
    public boolean getPrintToGif() {
        return printToGifCB.isChecked();
    }

    public void setPrintToGifFileName(String s) {
        printToGifFileNameTF.setText(s);
    }
    
    public String getPrintToGifFileName() {
        return printToGifFileNameTF.getText().toString();
    }

    public void setMaxGifHeight(int i) {
        maxGifHeightTF.setText(Integer.toString(i));
    }
    
    public int getMaxGifHeight() {
        try {
            int n = Integer.parseInt(maxGifHeightTF.getText().toString());
            if (n < 16)
                n = 16;
            else if (n > 32767)
                n = 32767;
            return n;
        } catch (NumberFormatException e) {
            return 256;
        }
    }
}
