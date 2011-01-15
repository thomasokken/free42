/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2011  Thomas Okken
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
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.TextView;

public class PreferencesDialog extends Dialog {
	private PrefsView view;
	private CheckBox singularMatrixCB;
	private CheckBox matrixOutOfRangeCB;
	private CheckBox autoRepeatCB;
	private CheckBox printToTextCB;
	private EditText printToTextFileNameTF;
	private CheckBox rawTextCB;
	private CheckBox printToGifCB;
	private EditText printToGifFileNameTF;
	private EditText maxGifHeightTF;
	private OkListener okListener;
	
	public PreferencesDialog(Context context) {
		super(context);
		view = new PrefsView(context);
		setContentView(view);
		this.setTitle("Preferences");
	}
	
	public interface OkListener {
		public void okPressed();
	}

	public void setOkListener(OkListener okListener) {
		this.okListener = okListener;
	}
	
	private class PrefsView extends ScrollView {
		public PrefsView(Context context) {
			super(context);
			
			RelativeLayout rl = new RelativeLayout(context);
			addView(rl);
			
			singularMatrixCB = new CheckBox(context);
			singularMatrixCB.setId(1);
			singularMatrixCB.setText("Singular Matrix Error");
			rl.addView(singularMatrixCB);
			
			matrixOutOfRangeCB = new CheckBox(context);
			matrixOutOfRangeCB.setId(2);
			matrixOutOfRangeCB.setText("Matrix Out Of Range");
			RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
			lp.addRule(RelativeLayout.BELOW, singularMatrixCB.getId());
			rl.addView(matrixOutOfRangeCB, lp);
			
			autoRepeatCB = new CheckBox(context);
			autoRepeatCB.setId(3);
			autoRepeatCB.setText("Auto-Repeat");
			lp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
			lp.addRule(RelativeLayout.BELOW, matrixOutOfRangeCB.getId());
			rl.addView(autoRepeatCB, lp);
			
			printToTextCB = new CheckBox(context);
			printToTextCB.setId(4);
			printToTextCB.setText("Print to Text:");
			lp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
			lp.addRule(RelativeLayout.BELOW, autoRepeatCB.getId());
			rl.addView(printToTextCB, lp);
			
			printToTextFileNameTF = new EditText(context);
			printToTextFileNameTF.setId(5);
			lp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.FILL_PARENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
			lp.addRule(RelativeLayout.BELOW, printToTextCB.getId());
			rl.addView(printToTextFileNameTF, lp);
			
			rawTextCB = new CheckBox(context);
			rawTextCB.setId(6);
			rawTextCB.setText("Raw Text");
			lp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
			lp.addRule(RelativeLayout.BELOW, printToTextFileNameTF.getId());
			rl.addView(rawTextCB, lp);
			
			printToGifCB = new CheckBox(context);
			printToGifCB.setId(7);
			printToGifCB.setText("Print to GIF:");
			lp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
			lp.addRule(RelativeLayout.BELOW, rawTextCB.getId());
			rl.addView(printToGifCB, lp);
			
			printToGifFileNameTF = new EditText(context);
			printToGifFileNameTF.setId(8);
			lp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.FILL_PARENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
			lp.addRule(RelativeLayout.BELOW, printToGifCB.getId());
			rl.addView(printToGifFileNameTF, lp);
			
			TextView label = new TextView(context);
			label.setId(9);
			label.setText("Max. GIF Height:");
			lp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
			lp.addRule(RelativeLayout.BELOW, printToGifFileNameTF.getId());
			rl.addView(label, lp);

			maxGifHeightTF = new EditText(context);
			maxGifHeightTF.setId(10);
			maxGifHeightTF.setText("t");
			lp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
			lp.addRule(RelativeLayout.BELOW, printToGifFileNameTF.getId());
			lp.addRule(RelativeLayout.RIGHT_OF, label.getId());
			rl.addView(maxGifHeightTF, lp);
			
			Button okB = new Button(context);
			okB.setId(11);
			okB.setText("OK");
			okB.setOnClickListener(new OnClickListener() {
				public void onClick(View view) {
					if (okListener != null)
						okListener.okPressed();
					PreferencesDialog.this.hide();
				}
			});
			lp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
			lp.addRule(RelativeLayout.BELOW, maxGifHeightTF.getId());
			rl.addView(okB, lp);

			Button cancelB = new Button(context);
			cancelB.setId(12);
			cancelB.setText("Cancel");
			cancelB.setOnClickListener(new OnClickListener() {
				public void onClick(View view) {
					PreferencesDialog.this.hide();
				}
			});
			lp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.WRAP_CONTENT, RelativeLayout.LayoutParams.WRAP_CONTENT);
			lp.addRule(RelativeLayout.BELOW, maxGifHeightTF.getId());
			lp.addRule(RelativeLayout.RIGHT_OF, okB.getId());
			rl.addView(cancelB, lp);
		}
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
	
	public void setRawText(boolean b) {
		rawTextCB.setChecked(b);
	}
	
	public boolean getRawText() {
		return rawTextCB.isChecked();
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
			if (n < 32)
				n = 32;
			else if (n > 32767)
				n = 32767;
			return n;
		} catch (NumberFormatException e) {
			return 256;
		}
	}
}
