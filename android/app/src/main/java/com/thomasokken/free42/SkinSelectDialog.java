package com.thomasokken.free42;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import android.app.Dialog;
import android.content.Context;
import android.database.DataSetObserver;
import android.graphics.Typeface;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.AdapterView.OnItemClickListener;

public class SkinSelectDialog extends Dialog {
    private Button loadButton;
    private Button deleteButton;
    private Button cancelButton;
    private ListView skinsView;
    private Listener listener;

    public SkinSelectDialog(Context ctx) {
        super(ctx);
        setContentView(R.layout.skin_select);
        getWindow().setLayout(WindowManager.LayoutParams.MATCH_PARENT,
                WindowManager.LayoutParams.MATCH_PARENT);
        loadButton = (Button) findViewById(R.id.loadButton);
        deleteButton = (Button) findViewById(R.id.deleteButton);
        cancelButton = (Button) findViewById(R.id.cancelButton);
        skinsView = (ListView) findViewById(R.id.skinsView);
        skinsView.setAdapter(new SkinListAdapter(ctx.getFilesDir().getAbsolutePath()));
        loadButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                doLoad();
            }
        });
        deleteButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                doDelete();
            }
        });
        cancelButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                SkinSelectDialog.this.dismiss();
            }
        });
        skinsView.setOnItemClickListener(new OnItemClickListener() {
            public void onItemClick(AdapterView<?> view, View parent, int position, long id) {
                String item = (String) view.getAdapter().getItem(position);
                SkinSelectDialog.this.listener.skinSelected(item);
                SkinSelectDialog.this.dismiss();
            }
        });
        setTitle("Select Skin");
    }

    public void setListener(Listener listener) {
        this.listener = listener;
    }
    
    public interface Listener {
        public void skinSelected(String skinName);
    }

    private void doLoad() {

    }

    private void doDelete() {
        SkinDeleteDialog sdd = new SkinDeleteDialog(getContext());
        sdd.setListener(new SkinDeleteDialog.Listener() {
            @Override
            public void dialogDone() {
                ((SkinListAdapter) skinsView.getAdapter()).refresh();
            }
        });
        sdd.show();
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
            String[] builtins = Free42Activity.builtinSkinNames;
            for (int i = builtins.length - 1; i >= 0; i--) {
                String name = builtins[i];
                nameList.remove(name);
                nameList.add(0, name);
            }
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
            String selectedSkin = Free42Activity.getSelectedSkin();
            boolean isSelected = name.equals(selectedSkin);
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
