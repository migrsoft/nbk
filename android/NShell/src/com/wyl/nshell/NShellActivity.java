package com.wyl.nshell;

import java.util.Calendar;
import android.app.Activity;
import android.content.res.Configuration;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;

public class NShellActivity extends Activity {
	
	private static final String TAG = "NbkShell";
	
	private static NShellActivity sInstance;
	
	private MainView mMainView;
	
	private View mMask;
	private boolean mNightMode = false;
	
	public static NShellActivity getInstance() {
		return sInstance;
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		sInstance = this;
		
		Log.i(TAG, "onCreate");
		
		mMainView = new MainView(this);
		setContentView(mMainView);
		
		Calendar now = Calendar.getInstance();
		int hour = now.get(Calendar.HOUR_OF_DAY);
		boolean night = (hour > 8 && hour < 22) ? false : true;
		switchNightMode(night);
	}

	@Override
	protected void onStart() {
		super.onStart();
	}

	@Override
	protected void onDestroy() {
		super.onDestroy();
		
		Log.i(TAG, "onDestroy");
		
		mMainView.destroy();
	}

	@Override
	public boolean onKeyUp(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK) {
			mMainView.onBackKeyPressed();
			return true;
		}
		else
			return super.onKeyUp(keyCode, event);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		MenuInflater inflater = getMenuInflater();
	    inflater.inflate(R.menu.main_menu, menu);
	    return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch (item.getItemId()) {
		case R.id.quit:
			finish();
			return true;
			
		case R.id.go_forward:
			mMainView.onForward();
			return true;
			
		case R.id.night_mode:
			switchNightMode(!mNightMode);
			return true;
			
		case R.id.open:
			openUrlDialog();
			return true;
			
		default:
			return super.onOptionsItemSelected(item);
		}
	}
	
	private void switchNightMode(boolean night) {
		
		if (night == mNightMode)
			return;
		
		if (mMask == null) {
			mMask = new View(this);
			mMask.setBackgroundResource(R.color.black_overlay);
		}
		
		ViewGroup vg = (ViewGroup) getWindow().getDecorView();

		if (!night) {
			mNightMode = false;
			vg.removeView(mMask);
		}
		else {
			mNightMode = true;
			vg.addView(mMask);
		}
	}
	
	private void openUrlDialog() {
		OpenUrlDialog dlg = new OpenUrlDialog();
		dlg.setListener(new OpenUrlDialog.OpenUrlDialogListener() {
			
			@Override
			public void onOpenUrl(String url) {
				mMainView.openUrl(url);
			}
		});
		dlg.show(getFragmentManager(), "OpenUrlDialog");
	}
}
