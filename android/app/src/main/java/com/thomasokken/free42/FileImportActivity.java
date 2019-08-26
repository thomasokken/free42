package com.thomasokken.free42;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.MediaStore;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URL;

public class FileImportActivity extends Activity {
    private String importedState;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        importedState = null;

        Intent intent = getIntent();
        String action = intent.getAction();
        System.err.println("FileImportActivity action = " + action);
        Uri uri = intent.getData();
        System.err.println("FileImportActivity uri = " + uri);

        // If attachment, some contortions to try and get the original file name
        String baseName = null;
        String scheme = uri.getScheme();
        if (scheme.equals("content")) {
            Cursor cursor = getContentResolver().query(uri, new String[]{MediaStore.MediaColumns.DISPLAY_NAME}, null, null, null);
            cursor.moveToFirst();
            int nameIndex = cursor.getColumnIndex(MediaStore.MediaColumns.DISPLAY_NAME);
            if (nameIndex >= 0) {
                baseName = cursor.getString(nameIndex);
            }
        } else {
            baseName = uri.getLastPathSegment();
        }
        if (baseName == null || baseName.equals(""))
            baseName = "Imported State";
        else if (baseName.toLowerCase().endsWith(".f42"))
            baseName = baseName.substring(0, baseName.length() - 4);

        int n = 0;
        String name;
        String baseDir = getFilesDir().getAbsolutePath();
        while (true) {
            n++;
            importedState = baseName;
            if (n > 1)
                importedState += " " + n;
            name = baseDir + "/" + importedState + ".f42";
            if (!new File(name).exists())
                break;
        }

        // Network access not allowed on main thread, so...
        if (scheme.equals("http") || scheme.equals("https")) {
            new NetworkLoader(uri.toString(), name).start();
            return;
        }

        InputStream is = null;
        OutputStream os = null;
        try {
            is = getContentResolver().openInputStream(uri);
            os = new FileOutputStream(name);
            byte[] buf = new byte[8192];
            // Check magic number first; don't bother importing if this is wrong
            n = is.read(buf, 0, 4);
            if (n != 4 || buf[0] != '2' || buf[1] != '4' || buf[2] != 'k' || buf[3] != 'F')
                throw new FormatException();
            os.write(buf, 0, 4);
            while ((n = is.read(buf)) >= 0)
                os.write(buf, 0, n);
            wrapUp(null);
        } catch (Exception e) {
            importedState = null;
            wrapUp(e);
        } finally {
            if (is != null)
                try {
                    is.close();
                } catch (IOException e) {}
            if (os != null)
                try {
                    os.close();
                } catch (IOException e) {}
        }
    }

    private void wrapUp(Exception e) {
        if (e == null) {
            alert("State imported.");
        } else if (e instanceof FormatException) {
            alert("Invalid state format.");
        } else {
            e.printStackTrace();
            alert("State import failed.");
        }
    }

    private void alert(String message) {
        runOnUiThread(new Alerter(message));
    }

    private class Alerter implements Runnable {
        private String message;
        public Alerter(String message) {
            this.message = message;
        }
        public void run() {
            AlertDialog.Builder builder = new AlertDialog.Builder(FileImportActivity.this);
            builder.setMessage(message);
            builder.setPositiveButton("OK", null);
            AlertDialog dialog = builder.create();
            dialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
                @Override
                public void onDismiss(DialogInterface dialogInterface) {
                    FileImportActivity.this.finish();
                    if (importedState != null) {
                        Free42Activity f42instance = Free42Activity.instance;
                        Intent i = new Intent(Intent.ACTION_MAIN);
                        i.addCategory(Intent.CATEGORY_LAUNCHER);
                        i.setClassName("com.thomasokken.free42", "com.thomasokken.free42.Free42Activity");
                        if (f42instance != null)
                            f42instance.importedState = importedState;
                        else
                            i.putExtra("importedState", importedState);
                        startActivity(i);
                    }
                }
            });
            dialog.show();
        }
    }
    private class NetworkLoader extends Thread {
        private String srcUrl, dstFile;
        private Exception ex;
        public NetworkLoader(String srcUrl, String dstFile) {
            this.srcUrl = srcUrl;
            this.dstFile = dstFile;
        }
        public void run() {
            InputStream is = null;
            OutputStream os = null;
            try {
                is = new URL(srcUrl).openStream();
                os = new FileOutputStream(dstFile);
                byte[] buf = new byte[8192];
                // Check magic number first; don't bother importing if this is wrong
                int n = is.read(buf, 0, 4);
                if (n != 4 || buf[0] != '2' || buf[1] != '4' || buf[2] != 'k' || buf[3] != 'F')
                    throw new FormatException();
                os.write(buf, 0, 4);
                while ((n = is.read(buf)) >= 0)
                    os.write(buf, 0, n);
            } catch (Exception e) {
                importedState = null;
                ex = e;
            } finally {
                if (is != null)
                    try {
                        is.close();
                    } catch (IOException e) {}
                if (os != null)
                    try {
                        os.close();
                    } catch (IOException e) {}
            }
            runOnUiThread(new Runnable() {
                public void run() {
                    wrapUp(ex);
                }
            });
        }
    }

    private static class FormatException extends Exception {
        public FormatException() {}
    }
}
