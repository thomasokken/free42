package com.thomasokken.free42;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;

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
        int n = 0;
        String name;
        String baseDir = getFilesDir().getAbsolutePath();
        while (true) {
            n++;
            name = baseDir + "/Imported State " + n;
            if (!new File(name).exists())
                break;
        }
        InputStream is = null;
        OutputStream os = null;
        try {
            // Of course, reading the file is horrendously complicated.
            // https://github.com/codenameone/codenameone/issues/772
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
