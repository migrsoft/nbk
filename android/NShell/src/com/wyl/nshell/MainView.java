package com.wyl.nshell;

import com.migrsoft.nbk.WebView;
import com.migrsoft.nbk.WebViewClient;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.util.Log;
import android.widget.LinearLayout;

public class MainView extends LinearLayout {
	
	private static final String TAG = "MainView";
	
//	AddressBar mAddressBar;
//	AddressBar.IUserAction mAddressAction = new AddressBar.IUserAction() {
//		
//		@Override
//		public void OnUrl(String url) {
//			if (!url.contains("://")) {
//				url = "http://" + url;
//			}
//			mWebView.requestFocus();
//			mWebView.loadUrl(url);
//		}
//	};
	
	WebView mWebView;
	
	WebViewClient mWebViewClient = new WebViewClient() {

		@Override
		public void onReady() {
			Log.i(TAG, "Loading homepage...");
			mWebView.loadUrl("file://assets/nbk_home.htm");
		}

		@Override
		public void cannotGoBack() {
			AlertDialog.Builder builder = new AlertDialog.Builder(NShellActivity.getInstance());
			builder.setMessage(R.string.alert_quit);
			builder.setPositiveButton(R.string.alert_yes, new DialogInterface.OnClickListener() {
				
				@Override
				public void onClick(DialogInterface dialog, int which) {
					NShellActivity.getInstance().finish();
				}
			});
			builder.setNegativeButton(R.string.alert_no, null);
			builder.create().show();
		}
		
	};

	public MainView(Context context) {
		super(context);
		
		setWillNotDraw(true);
		
		setOrientation(VERTICAL);
		
//		mAddressBar = new AddressBar(context);
//		mAddressBar.setUserActionListener(mAddressAction);
//		LinearLayout.LayoutParams barParams = new LinearLayout.LayoutParams(
//				LinearLayout.LayoutParams.MATCH_PARENT,
//				LinearLayout.LayoutParams.WRAP_CONTENT);
//		mAddressBar.setLayoutParams(barParams);
//		addView(mAddressBar);
		
		mWebView = new WebView(context);
		mWebView.setClient(mWebViewClient);
		LinearLayout.LayoutParams webParams = new LinearLayout.LayoutParams(
				LinearLayout.LayoutParams.MATCH_PARENT,
				LinearLayout.LayoutParams.MATCH_PARENT,
				1.0f);
		mWebView.setLayoutParams(webParams);
		mWebView.requestFocus();
		addView(mWebView);
	}

	public void destroy() {
		mWebView.abortCore();
	}
	
	public void onBackKeyPressed() {
		mWebView.onBack();
	}
	
	public void onForward() {
		mWebView.onForward();
	}
	
	public void openUrl(String url) {
		if (!url.contains("://")) {
			url = "http://" + url;
		}
		mWebView.loadUrl(url);
	}
}
