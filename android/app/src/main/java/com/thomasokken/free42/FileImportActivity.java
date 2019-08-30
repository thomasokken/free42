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
    private String dstFile;
    private boolean importIsState;

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
        String type = "f42";
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
        if (baseName == null || baseName.equals("")) {
            baseName = "Imported State";
        } else if (baseName.toLowerCase().endsWith(".f42")) {
            baseName = baseName.substring(0, baseName.length() - 4);
            type = "f42";
        } else if (baseName.toLowerCase().endsWith(".raw")) {
            baseName = baseName.substring(0, baseName.length() - 4);
            type = "raw";
        }

        if (type.equals("f42")) {
            importIsState = true;
            int n = 0;
            String baseDir = getFilesDir().getAbsolutePath();
            while (true) {
                n++;
                importedState = baseName;
                if (n > 1)
                    importedState += " " + n;
                dstFile = baseDir + "/" + importedState + ".f42";
                if (!new File(dstFile).exists())
                    break;
            }
        } else {
            importIsState = false;
            String cacheDir = getFilesDir().getAbsolutePath() + "/cache";
            new File(cacheDir).mkdir();
            dstFile = cacheDir + "/TEMP_RAW";
        }

        // Network access not allowed on main thread, so...
        if (scheme.equals("http") || scheme.equals("https")) {
            new NetworkLoader(uri.toString()).start();
            return;
        }

        InputStream is = null;
        OutputStream os = null;
        try {
            is = getContentResolver().openInputStream(uri);
            os = new FileOutputStream(dstFile);
            byte[] buf = new byte[8192];
            int n;
            if (importIsState) {
                // Check magic number first; don't bother importing if this is wrong
                n = is.read(buf, 0, 4);
                if (n != 4 || buf[0] != '2' || buf[1] != '4' || buf[2] != 'k' || buf[3] != 'F')
                    throw new FormatException();
                os.write(buf, 0, 4);
            }
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
            finish();
            Free42Activity f42instance = Free42Activity.instance;
            Intent i = new Intent(Intent.ACTION_MAIN);
            i.addCategory(Intent.CATEGORY_LAUNCHER);
            i.setClassName("com.thomasokken.free42", "com.thomasokken.free42.Free42Activity");
            if (f42instance != null) {
                if (importIsState)
                    f42instance.importedState = importedState;
                else
                    f42instance.importedProgram = dstFile;
            } else {
                if (importIsState)
                    i.putExtra("importedState", importedState);
                else
                    i.putExtra("importedProgram", dstFile);
            }
            startActivity(i);
        } else if (e instanceof FormatException) {
            new File(dstFile).delete();
            runOnUiThread(new Alerter("Invalid state format."));
        } else {
            e.printStackTrace();
            new File(dstFile).delete();
            runOnUiThread(new Alerter("State import failed."));
        }
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
                }
            });
            dialog.show();
        }
    }

    private class NetworkLoader extends Thread {
        private String srcUrl;
        private Exception ex;
        public NetworkLoader(String srcUrl) {
            this.srcUrl = srcUrl;
        }
        public void run() {
            InputStream is = null;
            OutputStream os = null;
            try {
                is = new URL(srcUrl).openStream();
                os = new FileOutputStream(dstFile);
                byte[] buf = new byte[8192];
                int n;
                if (importIsState) {
                    // Check magic number first; don't bother importing if this is wrong
                    n = is.read(buf, 0, 4);
                    if (n != 4 || buf[0] != '2' || buf[1] != '4' || buf[2] != 'k' || buf[3] != 'F')
                        throw new FormatException();
                    os.write(buf, 0, 4);
                }
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
            wrapUp(ex);
        }
    }

    private static class FormatException extends Exception {
        public FormatException() {}
    }
}
