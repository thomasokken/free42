/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2024  Thomas Okken
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see http://www.gnu.org/licenses/.
 *****************************************************************************/

package com.thomasokken.free42;

import android.app.Activity;
import android.net.Uri;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class BackgroundIO {
    public static void save(String inputFile, Uri outputUri, boolean deleteOrig, String errMsg, Runnable continuation) {
        new BackgroundSaver(inputFile, outputUri, deleteOrig, errMsg, continuation).start();
    }

    private static class BackgroundSaver extends Thread {
        private String inputFile;
        private Uri outputUri;
        private boolean deleteOrig;
        private String errMsg;
        private Runnable continuation;

        public BackgroundSaver(String inputFile, Uri outputUri, boolean deleteOrig, String errMsg, Runnable continuation) {
            this.inputFile = inputFile;
            this.outputUri = outputUri;
            this.deleteOrig = deleteOrig;
            this.errMsg = errMsg;
            this.continuation = continuation;
        }

        public void run() {
            InputStream is = null;
            OutputStream os = null;
            try {
                is = new FileInputStream(inputFile);
                os = Free42Activity.instance.getContentResolver().openOutputStream(outputUri);
                byte[] buf = new byte[1024];
                int n;
                while ((n = is.read(buf)) != -1)
                    os.write(buf, 0, n);
            } catch (IOException e) {
                if (errMsg != null)
                    Free42Activity.instance.runOnUiThread(new Runnable() { public void run() { Free42Activity.showAlert(errMsg); }});
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
            if (deleteOrig)
            new File(inputFile).delete();
            if (continuation != null)
                Free42Activity.instance.runOnUiThread(continuation);
        }
    }
}
