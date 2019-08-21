package com.thomasokken.free42;

import android.app.Activity;
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

public class FileImportActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Intent intent = getIntent();
        String action = intent.getAction();
        /*if (!Intent.ACTION_SEND.equals(action) && !Intent.ACTION_VIEW.equals(action))
            throw new UnsupportedOperationException();*/
        System.err.println("FileImportActivity action = " + action);
        Uri uri = intent.getData();
        System.err.println("FileImportActivity uri = " + uri);

        // If attachment, some contortions to try and get the original file name
        String baseName = null;
        if (uri.getScheme().equals("content")) {
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
            name = baseDir + "/" + baseName;
            if (n > 1)
                name += " " + n + ".f42";
            if (!new File(name).exists())
                break;
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
        } catch (IOException e) {
            // TODO
            e.printStackTrace();
        } catch (FormatException e) {
            // TODO
            e.printStackTrace();
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
        finish();
    }

    private static class FormatException extends Exception {
        public FormatException() {}
    }
}
