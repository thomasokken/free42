package com.thomasokken.free42;

import com.thomasokken.free42.PreferencesDialog.OkListener;

import android.app.Dialog;
import android.content.Context;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.Spinner;
import android.widget.TextView;

public class FileSelectionDialog extends Dialog {
	private FileSelView view;
	private Spinner dirListSpinner;
	private ListView dirView;
	private Spinner fileTypeSpinner;
	private TextView fileNameView;
	private Button upButton;
	private Button mkdirButton;
	private Button okButton;
	private Button cancelButton;
	private OkListener okListener;
	
	public FileSelectionDialog(Context ctx) {
		super(ctx);
		view = new FileSelView(ctx);
		setContentView(view);
		setTitle("Select File");
	}
	
	public interface OkListener {
		public void okPressed();
	}

	public void setOkListener(OkListener okListener) {
		this.okListener = okListener;
	}
	
	private class FileSelView extends LinearLayout {
		public FileSelView(Context ctx) {
			super(ctx);
			setOrientation(VERTICAL);
			dirListSpinner = new Spinner(ctx);
			LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT, 0);
			lp.gravity = Gravity.LEFT;
			addView(dirListSpinner, lp);
			LinearLayout ll = new LinearLayout(ctx);
			dirView = new ListView(ctx);
			lp = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.FILL_PARENT, 1);
			ll.addView(dirView, lp);
			LinearLayout ll2 = new LinearLayout(ctx);
			ll2.setOrientation(VERTICAL);
			upButton = new Button(ctx);
			upButton.setText("Up");
			lp = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
			lp.gravity = Gravity.CENTER_HORIZONTAL;
			ll2.addView(upButton, lp);
			mkdirButton = new Button(ctx);
			mkdirButton.setText("MkDir");
			ll2.addView(mkdirButton, lp);
			okButton = new Button(ctx);
			okButton.setText("OK");
			ll2.addView(okButton, lp);
			cancelButton = new Button(ctx);
			cancelButton.setText("Cancel");
			ll2.addView(cancelButton, lp);
			lp = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
			lp.gravity = Gravity.CENTER_VERTICAL;
			ll.addView(ll2, lp);
			lp = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.FILL_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT, 1);
			addView(ll, lp);
			fileTypeSpinner = new Spinner(ctx);
			lp = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT, 0);
			addView(fileTypeSpinner, lp);
			fileNameView = new TextView(ctx);
			addView(fileNameView, lp);
		}
	}
}
