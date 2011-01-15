package com.thomasokken.free42;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.StringTokenizer;

import android.app.Dialog;
import android.content.Context;
import android.view.Gravity;
import android.view.View;
import android.widget.ArrayAdapter;
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
	private String currentPath;
	
	public FileSelectionDialog(Context ctx) {
		super(ctx);
		view = new FileSelView(ctx);
		setContentView(view);
		setTitle("Select File");
		setPath("/");
	}
	
	public interface OkListener {
		public void okPressed();
	}

	public void setOkListener(OkListener okListener) {
		this.okListener = okListener;
	}
	
	public void setPath(String path) {
		String fileName = "";
		File f = new File(path);
		if (!f.exists() || f.isFile()) {
			int p = path.lastIndexOf("/");
			if (p == -1) {
				fileName = path;
				path = "/";
			} else {
				fileName = path.substring(p + 1);
				path = path.substring(0, p);
				if (path.length() == 0)
					path = "/";
			}
		}
		StringTokenizer tok = new StringTokenizer(path, "/");
		List<String> pathComps = new ArrayList<String>();
		pathComps.add("/");
		StringBuffer pathBuf = new StringBuffer();
		pathBuf.append("/");
		while (tok.hasMoreTokens()) {
			String t = tok.nextToken();
			pathComps.add(t);
			pathBuf.append(t);
			pathBuf.append("/");
		}
		ArrayAdapter<String> aa = new ArrayAdapter<String>(getContext(), android.R.layout.simple_spinner_item, pathComps);
		dirListSpinner.setAdapter(aa);
		fileNameView.setText(fileName);
		currentPath = pathBuf.toString();
	}

	private void doUp() {
		if (currentPath.length() > 1) {
			int n = currentPath.lastIndexOf("/", currentPath.length() - 2);
			setPath(currentPath.substring(0, n + 1) + fileNameView.getText());
		}
	}
	
	private void doMkDir() {
		// TODO
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
			upButton.setOnClickListener(new OnClickListener() {
				public void onClick(View view) {
					doUp();
				}
			});
			lp = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
			lp.gravity = Gravity.CENTER_HORIZONTAL;
			ll2.addView(upButton, lp);
			mkdirButton = new Button(ctx);
			mkdirButton.setText("MkDir");
			mkdirButton.setOnClickListener(new OnClickListener() {
				public void onClick(View view) {
					doMkDir();
				}
			});
			ll2.addView(mkdirButton, lp);
			okButton = new Button(ctx);
			okButton.setText("OK");
			okButton.setOnClickListener(new OnClickListener() {
				public void onClick(View view) {
					if (okListener != null)
						okListener.okPressed();
					FileSelectionDialog.this.hide();
				}
			});
			ll2.addView(okButton, lp);
			cancelButton = new Button(ctx);
			cancelButton.setText("Cancel");
			cancelButton.setOnClickListener(new OnClickListener() {
				public void onClick(View view) {
					FileSelectionDialog.this.hide();
				}
			});
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
