package com.thomasokken.free42;

import android.app.Dialog;
import android.content.Context;
import android.view.View;
import android.view.WindowManager;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Button;
import android.widget.EditText;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;

public class SkinLoadDialog extends Dialog {
    private Button goButton;
    private Button backButton;
    private Button forwardButton;
    private EditText urlTF;
    private Button loadButton;
    private Button doneButton;
    private WebView webView;
    private Listener listener;
    private String filesDir;

    public SkinLoadDialog(Context ctx) {
        super(ctx);
        setContentView(R.layout.skin_load);
        getWindow().setLayout(WindowManager.LayoutParams.MATCH_PARENT,
                WindowManager.LayoutParams.MATCH_PARENT);
        goButton = (Button) findViewById(R.id.goB);
        backButton = (Button) findViewById(R.id.backB);
        forwardButton = (Button) findViewById(R.id.forwardB);
        loadButton = (Button) findViewById(R.id.loadB);
        doneButton = (Button) findViewById(R.id.doneB);
        urlTF = (EditText) findViewById(R.id.urlTF);
        webView = (WebView) findViewById(R.id.webView);
        goButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                doGo();
            }
        });
        backButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                webView.goBack();
            }
        });
        forwardButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                webView.goForward();
            }
        });
        loadButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                doLoad();
            }
        });
        doneButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                listener.dialogDone();
                SkinLoadDialog.this.dismiss();
            }
        });
        webView.setWebViewClient(new WebViewClient() {
            public void onPageFinished(WebView view, String url) {
                urlTF.setText(url);
            }
        });
        setTitle("Load Skins");
        filesDir = ctx.getFilesDir().getAbsolutePath();
    }

    public void setListener(Listener listener) {
        this.listener = listener;
    }

    public void setUrl(String url) {
        urlTF.setText(url);
        doGo();
    }
    
    public interface Listener {
        public void dialogDone();
    }

    private void doGo() {
        String url = urlTF.getText().toString();
        webView.loadUrl(url);
    }

    private void doLoad() {
        new Thread() {
            public void run() {
                doLoad2();
            }
        }.start();
    }

    private void doLoad2() {
        String tempGifName = filesDir + "/_temp_gif_";
        String tempLayoutName = filesDir + "/_temp_layout_";
        try {
            String url = urlTF.getText().toString();
            URL u = new URL(url);
            String path = u.getPath();
            if (path == null) {
                Free42Activity.showAlert("Invalid Skin URL");
                return;
            }
            String gifUrl = null, layoutUrl = null;
            String baseName;
            if (path.endsWith(".gif")) {
                gifUrl = url;
                baseName = path.substring(0, path.length() - 4);
                String layoutPath = baseName + ".layout";
                layoutUrl = new URL(u.getProtocol(), u.getHost(), u.getPort(), layoutPath).toString();
            } else if (path.endsWith(".layout")) {
                layoutUrl = url;
                baseName = path.substring(0, path.length() - 7);
                String gifPath = baseName + ".gif";
                gifUrl = new URL(u.getProtocol(), u.getHost(), u.getPort(), gifPath).toString();
            } else {
                Free42Activity.showAlert("Invalid Skin URL");
                return;
            }

            loadFile(gifUrl, tempGifName);
            loadFile(layoutUrl, tempLayoutName);
            int lastSlash = baseName.lastIndexOf("/");
            baseName = baseName.substring(lastSlash + 1);
            String gifName = filesDir + "/" + baseName + ".gif";
            String layoutName = filesDir + "/" + baseName + ".layout";
            new File(tempGifName).renameTo(new File(gifName));
            new File(tempLayoutName).renameTo(new File(layoutName));
        } catch (Exception e) {
            new File(tempGifName).delete();
            new File(tempLayoutName).delete();
            Free42Activity.showAlert("Loading Skin Failed");
        }
    }

    private void loadFile(String url, String fileName) throws IOException {
        InputStream is = null;
        OutputStream os = null;
        try {
            HttpURLConnection con = (HttpURLConnection) new URL(url).openConnection();
            int resCode = con.getResponseCode();
            if (resCode != 200)
                throw new IOException("Response code " + resCode);
            is = con.getInputStream();
            os = new FileOutputStream(fileName);
            byte[] buf = new byte[8192];
            int n;
            while ((n = is.read(buf)) != -1)
                os.write(buf, 0, n);
            is.close();
            os.close();
        } catch (IOException e) {
            if (is != null)
                try {
                    is.close();
                } catch (IOException e2) {}
            if (os != null)
                try {
                    os.close();
                } catch (IOException e2) {}
            new File(fileName).delete();
        }
    }
}
