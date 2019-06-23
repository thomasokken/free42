package com.thomasokken.free42;

import android.app.Dialog;
import android.content.Context;
import android.view.View;
import android.view.WindowManager;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Button;
import android.widget.EditText;

public class SkinLoadDialog extends Dialog {
    private Button goButton;
    private Button backButton;
    private Button forwardButton;
    private EditText urlTF;
    private Button loadButton;
    private Button doneButton;
    private WebView webView;
    private Listener listener;

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
        // TODO
    }
}
