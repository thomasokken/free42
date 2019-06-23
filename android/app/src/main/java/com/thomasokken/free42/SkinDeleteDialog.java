package com.thomasokken.free42;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import android.app.Dialog;
import android.content.Context;
import android.database.DataSetObserver;
import android.graphics.Typeface;
import android.util.SparseBooleanArray;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;

public class SkinDeleteDialog extends Dialog {
    private Button deleteButton;
    private Button doneButton;
    private ListView skinsView;
    private Listener listener;

    public SkinDeleteDialog(Context ctx) {
        super(ctx);
        setContentView(R.layout.skin_delete);
        getWindow().setLayout(WindowManager.LayoutParams.MATCH_PARENT,
                WindowManager.LayoutParams.MATCH_PARENT);
        deleteButton = (Button) findViewById(R.id.deleteButton);
        doneButton = (Button) findViewById(R.id.doneButton);
        skinsView = (ListView) findViewById(R.id.skinsView);
        skinsView.setChoiceMode(ListView.CHOICE_MODE_MULTIPLE);
        skinsView.setAdapter(new SkinListAdapter(ctx.getFilesDir().getAbsolutePath()));
        deleteButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                doDelete();
            }
        });
        doneButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                listener.dialogDone();
                SkinDeleteDialog.this.dismiss();
            }
        });
        setTitle("Delete Skins");
    }

    public void setListener(Listener listener) {
        this.listener = listener;
    }
    
    public interface Listener {
        public void dialogDone();
    }

    private void doDelete() {
        SparseBooleanArray sba = skinsView.getCheckedItemPositions();
        for (int i = 0; i < sba.size(); i++) {
            if (sba.valueAt(i)) {
                int k = sba.keyAt(i);
                String name = ((SkinListAdapter) skinsView.getAdapter()).names[k];
                // Delete skin 'name'
            }
        }
    }

    private static class SkinListAdapter extends BaseAdapter {
        private String skinDirName;
        private String[] names;
        private List<DataSetObserver> observers = new ArrayList<DataSetObserver>();

        public SkinListAdapter(String skinDirName) {
            this.skinDirName = skinDirName;
            refresh();
        }

        public void refresh() {
            ArrayList<String> nameList = new ArrayList<String>();
            File[] files = new File(skinDirName).listFiles();
            if (files != null)
                for (File file : files) {
                    if (!file.isFile())
                        continue;
                    String fn = file.getName();
                    if (!fn.endsWith(".layout"))
                        continue;
                    nameList.add(fn.substring(0, fn.length() - 7));
                }
            Collections.sort(nameList, String.CASE_INSENSITIVE_ORDER);
            names = nameList.toArray(new String[nameList.size()]);
            DataSetObserver[] dsoArray;
            synchronized (observers) {
                dsoArray = observers.toArray(new DataSetObserver[observers.size()]);
            }
            for (DataSetObserver dso : dsoArray)
                dso.onChanged();
        }

        public int getCount() {
            return names.length;
        }

        public Object getItem(int position) {
            return names[position];
        }

        public long getItemId(int position) {
            return position;
        }

        public View getView(int position, View convertView, ViewGroup parent) {
            if (convertView == null) {
                Context context = parent.getContext();
                convertView = new TextView(context);
            }
            TextView tv = (TextView) convertView;
            String name = names[position];
            tv.setText(name);
            String[] selectedSkins = Free42Activity.getSelectedSkins();
            boolean isSelected = name.equals(selectedSkins[0]) || name.equals(selectedSkins[1]);
            tv.setTextSize(30.0f);
            tv.setTypeface(null, isSelected ? Typeface.BOLD : Typeface.NORMAL);
            return tv;
        }

        public boolean hasStableIds() {
            return true;
        }

        public boolean isEmpty() {
            return names.length == 0;
        }

        public void registerDataSetObserver(DataSetObserver observer) {
            synchronized (observers) {
                observers.add(observer);
            }
        }

        public void unregisterDataSetObserver(DataSetObserver observer) {
            synchronized (observers) {
                observers.remove(observer);
            }
        }

        public boolean areAllItemsEnabled() {
            return true;
        }

        public boolean isEnabled(int position) {
            return true;
        }
    }
}
