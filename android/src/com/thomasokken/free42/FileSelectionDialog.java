package com.thomasokken.free42;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.StringTokenizer;

import android.app.Dialog;
import android.content.Context;
import android.database.DataSetObserver;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Adapter;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView.OnItemSelectedListener;

public class FileSelectionDialog extends Dialog {
	//private FileSelView view;
	private Spinner dirListSpinner;
	private ListView dirView;
	private Spinner fileTypeSpinner;
	private EditText fileNameTF;
	private Button upButton;
	private Button mkdirButton;
	private Button okButton;
	private Button cancelButton;
	private OkListener okListener;
	private String currentPath;
	
	public FileSelectionDialog(Context ctx, String[] types) {
		super(ctx);
		setContentView(R.layout.file_selection_dialog);
		dirListSpinner = (Spinner) findViewById(R.id.dirListSpinner);
		dirListSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> view, View parent, int position, long id) {
				Adapter a = view.getAdapter();
				if (position == a.getCount() - 1)
					return;
				StringBuffer pathBuf = new StringBuffer("/");
				for (int i = 1; i <= position; i++) {
					pathBuf.append(a.getItem(i));
					pathBuf.append("/");
				}
				setPath(pathBuf.toString());
			}
			public void onNothingSelected(AdapterView<?> arg0) {
				// Shouldn't happen
			}
		});
		fileTypeSpinner = (Spinner) findViewById(R.id.fileTypeSpinner);
		ArrayAdapter<String> aa = new ArrayAdapter<String>(getContext(), android.R.layout.simple_spinner_item, types);
		fileTypeSpinner.setAdapter(aa);
		fileTypeSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
			public void onItemSelected(AdapterView<?> view, View parent, int position, long id) {
				String type = (String) view.getAdapter().getItem(position);
				if (type.equals("*"))
					type = null;
				DirListAdapter dla = (DirListAdapter) dirView.getAdapter();
				if (dla != null)
					dla.setType(type);
			}
			public void onNothingSelected(AdapterView<?> arg0) {
				// Shouldn't happen
			}
		});
		fileNameTF = (EditText) findViewById(R.id.fileNameTF);
		upButton = (Button) findViewById(R.id.upButton);
		upButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View view) {
				doUp();
			}
		});
		mkdirButton = (Button) findViewById(R.id.mkdirButton);
		mkdirButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View view) {
				doMkDir();
			}
		});
		okButton = (Button) findViewById(R.id.okButton);
		okButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View view) {
				if (okListener != null)
					okListener.okPressed(currentPath + fileNameTF.getText().toString());
				FileSelectionDialog.this.dismiss();
			}
		});
		cancelButton = (Button) findViewById(R.id.cancelButton);
		cancelButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View view) {
				FileSelectionDialog.this.dismiss();
			}
		});
		dirView = (ListView) findViewById(R.id.dirView);
		dirView.setOnItemClickListener(new OnItemClickListener() {
			public void onItemClick(AdapterView<?> view, View parent, int position, long id) {
				File item = (File) view.getAdapter().getItem(position);
				if (item.isDirectory())
					setPath(item.getAbsolutePath());
				else
					fileNameTF.setText(item.getName());
			}
		});
		setTitle("Select File");
		setPath("/");
	}
	
	public interface OkListener {
		public void okPressed(String path);
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
		dirListSpinner.setSelection(pathComps.size() - 1);
		fileNameTF.setText(fileName);
		currentPath = pathBuf.toString();
		
		File[] list = new File(currentPath).listFiles();
		if (list == null)
			list = new File[0];
		Arrays.sort(list);
		String type = (String) fileTypeSpinner.getSelectedItem();
		if (type.equals("*"))
			type = null;
		dirView.setAdapter(new DirListAdapter(list, type));
	}

	private void doUp() {
		if (currentPath.length() > 1) {
			int n = currentPath.lastIndexOf("/", currentPath.length() - 2);
			setPath(currentPath.substring(0, n + 1) + fileNameTF.getText().toString());
		}
	}
	
	private void doMkDir() {
		String dirName = fileNameTF.getText().toString();
		if (dirName != null && dirName.length() > 0) {
			File newDir = new File(currentPath + dirName);
			newDir.mkdir();
			if (newDir.isDirectory())
				setPath(newDir.getAbsolutePath());
		}
	}
	
	private static class DirListAdapter implements ListAdapter {
		private File[] items;
		private String type;
		private List<DataSetObserver> observers = new ArrayList<DataSetObserver>();
		
		public DirListAdapter(File[] items, String type) {
			this.items = items;
			this.type = type;
		}
		
		public void setType(String type) {
			boolean changed = type == null ? this.type != null : !type.equals(this.type);
			this.type = type;
			if (changed) {
				DataSetObserver[] dsoArray;
				synchronized (observers) {
					dsoArray = observers.toArray(new DataSetObserver[observers.size()]);
				}
				for (DataSetObserver dso : dsoArray)
					dso.onChanged();
			}
		}

		@Override
		public int getCount() {
			return items.length;
		}

		@Override
		public Object getItem(int position) {
			return items[position];
		}

		@Override
		public long getItemId(int position) {
			return position;
		}

		@Override
		public int getItemViewType(int position) {
			return items[position].isDirectory() ? 0 : 1;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			File item = items[position];
			if (convertView == null) {
				Context context = parent.getContext();
				LayoutInflater inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
				convertView = inflater.inflate(R.layout.file_selection_dialog_row, null);
    			ImageView icon = (ImageView) convertView.findViewById(R.id.fdrowimage);
    			icon.setImageResource(item.isDirectory() ? R.drawable.folder : R.drawable.document);
			}
			TextView text = (TextView) convertView.findViewById(R.id.fdrowtext);
			text.setText(item.getName());
			return convertView;
		}

		@Override
		public int getViewTypeCount() {
			return 2;
		}

		@Override
		public boolean hasStableIds() {
			return true;
		}

		@Override
		public boolean isEmpty() {
			return items.length == 0;
		}

		@Override
		public void registerDataSetObserver(DataSetObserver observer) {
			synchronized (observers) {
				observers.add(observer);
			}
		}

		@Override
		public void unregisterDataSetObserver(DataSetObserver observer) {
			synchronized (observers) {
				observers.add(observer);
			}
		}

		@Override
		public boolean areAllItemsEnabled() {
			return false;
		}

		@Override
		public boolean isEnabled(int position) {
			File item = items[position];
			return item.isDirectory() || type == null || item.getName().endsWith("." + type);
		}
	}
}
