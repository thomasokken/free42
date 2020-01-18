package com.thomasokken.free42;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import android.Manifest;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.pm.PackageManager;
import android.database.DataSetObserver;
import android.graphics.Typeface;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
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
    private String skinDirName;
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
        skinDirName = ctx.getFilesDir().getAbsolutePath();
        skinsView.setAdapter(new SkinListAdapter(skinDirName));
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
        if (ContextCompat.checkSelfPermission(Free42Activity.instance, Manifest.permission.INTERNET) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(Free42Activity.instance, new String[] { Manifest.permission.INTERNET }, 0);
            return;
        }
        SkinLoadDialog sld = new SkinLoadDialog(getContext());
        sld.setUrl("https://thomasokken.com/free42/skins/");
        sld.setListener(new SkinLoadDialog.Listener() {
            @Override
            public void dialogDone() {
                ((SkinListAdapter) skinsView.getAdapter()).refresh();
            }
        });
        sld.show();
    }

    private String[] skinNames;
    private boolean[] selectedSkinIndexes;

    private void doDelete() {
        ArrayList<String> nameList = getLoadedSkins(skinDirName);
        skinNames = nameList.toArray(new String[nameList.size()]);
        selectedSkinIndexes = new boolean[skinNames.length];

        AlertDialog.Builder builder = new AlertDialog.Builder(getContext());
        builder.setTitle("Delete Skins");
        builder.setMultiChoiceItems(skinNames, selectedSkinIndexes, new DialogInterface.OnMultiChoiceClickListener() {
            public void onClick(DialogInterface dialog, int which, boolean isChecked) {
                // I don't have to do anything here; the only reason why
                // I create this listener is because if I pass 'null'
                // instead, the selectedProgramIndexes array never gets
                // updated.
            }
        });
        DialogInterface.OnClickListener listener = new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                doSkinDeleteClick(dialog, which);
            }
        };
        builder.setPositiveButton("OK", listener);
        builder.setNegativeButton("Cancel", null);
        builder.create().show();
    }

    private void doSkinDeleteClick(DialogInterface dialog, int which) {
        if (which == DialogInterface.BUTTON_POSITIVE) {
            for (int i = 0; i < selectedSkinIndexes.length; i++)
                if (selectedSkinIndexes[i]) {
                    String baseName = skinDirName + "/" + skinNames[i];
                    new File(baseName + ".layout").delete();
                    new File(baseName + ".gif").delete();
                }
            ((SkinListAdapter) skinsView.getAdapter()).refresh();
        }
        skinNames = null;
        selectedSkinIndexes = null;
        dialog.dismiss();
    }

    private static ArrayList<String> getLoadedSkins(String skinDirName) {
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
        return nameList;
    }

    private static class SkinListAdapter extends BaseAdapter {
        private String skinDirName;
        private String[] names;
        private boolean[] enabled;
        private List<DataSetObserver> observers = new ArrayList<DataSetObserver>();

        public SkinListAdapter(String skinDirName) {
            this.skinDirName = skinDirName;
            refresh();
        }

        public void refresh() {
            ArrayList<String> nameList = getLoadedSkins(skinDirName);
            String[] builtins = Free42Activity.builtinSkinNames;
            enabled = new boolean[nameList.size() + builtins.length];
            for (int i = 0; i < builtins.length; i++) {
                String name = builtins[i];
                enabled[i] = !nameList.contains(name);
                nameList.add(i, name);
            }
            names = nameList.toArray(new String[nameList.size()]);
            for (int i = builtins.length; i < enabled.length; i++)
                enabled[i] = true;
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
            boolean isSelected = enabled[position] && name.equals(selectedSkin);
            tv.setTextSize(30.0f);
            tv.setTypeface(null, isSelected ? Typeface.ITALIC : Typeface.NORMAL);
            tv.setEnabled(enabled[position]);
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
            return false;
        }

        public boolean isEnabled(int position) {
            return enabled[position];
        }
    }
}
