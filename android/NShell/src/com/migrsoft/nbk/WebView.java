package com.migrsoft.nbk;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Rect;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.widget.FrameLayout;

public class WebView extends FrameLayout {
	
	private static final String TAG = "NbkWebView";
	
	private WebViewClient mClient;

	private int mWidth;
	private int mHeight;
	private Bitmap mBitmap;
	private Canvas mCanvas;
	private float mDensity = 1.0f;
	
	private NbkMainThread mMainThread;
	
	private int mPageWidth;
	private int mPageHeight;
	
	private int mPageX;
	private int mPageY;
	
	private Progress mProgress;
	
	public WebView(Context context) {
		super(context);
		
		setBackgroundColor(Color.GRAY);
		setClickable(true);
		setFocusable(true);
		setFocusableInTouchMode(true);
		
		mDensity = Resources.getSystem().getDisplayMetrics().density;
		
		createEngine();
	}
	
	private void createEngine() {
		
		mWidth = 0;
		mHeight = 0;

		mPageWidth = 0;
		mPageHeight = 0;
		
		mProgress = new Progress();
		
		mMainThread = new NbkMainThread(getContext());
		mMainThread.setGuiHandler(mGuiHandler);
		mMainThread.start();
	}
	
	private void createScreen(int w, int h) {
		if (w == mWidth && h == mHeight)
			return;
		
		Log.i(TAG, "create screen buffer");
		
		mWidth = w;
		mHeight = h;
		
		mBitmap = Bitmap.createBitmap(mWidth, mHeight, Bitmap.Config.ARGB_8888);
		mCanvas = new Canvas(mBitmap);
		
		mProgress.setArea(new Rect(0, 0, w, 5));
	}

	public void setClient(WebViewClient client) {
		mClient = client;
	}

	public void abortCore() {
		mMainThread.sendEventToCore(new NbkEvent(NbkEvent.EVENT_QUIT));
		
		try {
			mMainThread.join();
		}
		catch (Exception e) {
		}
	}
	
	private Handler mGuiHandler = new Handler() {

		@Override
		public void handleMessage(Message msg) {
			handleShellEvent((NbkEvent) msg.obj);
		}
	};
	
	private void handleShellEvent(NbkEvent e) {
		
//		Log.i("NbkView", "GUI event " + e.getType());
	
		switch (e.getType()) {
		case NbkEvent.EVENT_INIT:
			if (mClient != null) mClient.onReady();
			break;
			
		case NbkEvent.EVENT_NO_BACK:
			if (mClient != null)
				mClient.cannotGoBack();
			break;
			
		case NbkEvent.EVENT_NEW_DOC_BEGIN:
			onNewDocBegin();
			break;
			
		case NbkEvent.EVENT_UPDATE_VIEW:
			mPageWidth = e.getWidth();
			mPageHeight = e.getHeight();
			mPageX = e.getX();
			mPageY = e.getY();
//			Log.i(TAG, "EVENT_UPDATE_VIEW " + mPageWidth + " " + mPageHeight);
			updatePage();
			break;
			
		case NbkEvent.EVENT_PROGRESS_DOC:
			mProgress.setDocProgress(e.getProgressCurr(), e.getProgressTotal());
			invalidate();
			break;
			
		case NbkEvent.EVENT_PROGRESS_PIC:
			mProgress.setPicProgress(e.getProgressCurr(), e.getProgressTotal());
			invalidate();
			break;
		}
	}
	
	private void updatePage() {
//		Log.i(TAG, "updatePage");
//		Tools.saveBitmapToFile(mMainThread.getBitmap(), "/sdcard/nbk.png");
		mCanvas.drawBitmap(mMainThread.getBitmap(), 0, 0, null);
		invalidate();
	}
	
	@Override
	protected void onDraw(Canvas canvas) {
		if (mBitmap != null) {
			canvas.drawBitmap(mBitmap, 0, 0, null);
		}
		mProgress.draw(canvas);
	}
	
	public int getPageWidth() {
		return mPageWidth;
	}
	
	public int getPageHeight() {
		return mPageHeight;
	}
	
	public void loadUrl(String url) {
		if (!mMainThread.isReady()) {
			Log.i(TAG, "nbk in initializing");
			return;
		}
		
		NbkEvent load = new NbkEvent(NbkEvent.EVENT_LOAD_URL);
		load.setUrl(url);
		mMainThread.sendEventToCore(load);
		mProgress.reset();
		mProgress.setDocProgress(2, 10);
		invalidate();
	}

	@Override
	protected void onSizeChanged(int w, int h, int oldw, int oldh) {
		super.onSizeChanged(w, h, oldw, oldh);
		
		createScreen(w, h);
		
		NbkEvent e = new NbkEvent(NbkEvent.EVENT_SCREEN_CHANGED);
		e.setSize((int) (w / mDensity), (int) (h / mDensity));
		mMainThread.sendEventToCore(e);
	}
	
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_VOLUME_UP ) {
			pageUp();
			return true;
		}
		else if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) {
			pageDown();
			return true;
		}
		else
			return super.onKeyDown(keyCode, event);
	}

	@Override
	public boolean onTouchEvent(MotionEvent event) {
		if (!mMainThread.isReady()) {
			return super.onTouchEvent(event);
		}
		
		int x = (int) (event.getX() / mDensity);
		int y = (int) (event.getY() / mDensity);
		
		if (event.getAction() == MotionEvent.ACTION_DOWN) {
			NbkEvent mouse = new NbkEvent(NbkEvent.EVENT_MOUSE_LEFT);
			mouse.setClick(NbkEvent.MOUSE_PRESS, x, y);
			mMainThread.sendEventToCore(mouse);
		}
		else if (event.getAction() == MotionEvent.ACTION_MOVE) {
			NbkEvent mouse = new NbkEvent(NbkEvent.EVENT_MOUSE_LEFT);
			mouse.setClick(NbkEvent.MOUSE_MOVE, x, y);
			mMainThread.sendEventToCore(mouse);
		}
		else if (event.getAction() == MotionEvent.ACTION_UP) {
			NbkEvent mouse = new NbkEvent(NbkEvent.EVENT_MOUSE_LEFT);
			mouse.setClick(NbkEvent.MOUSE_RELEASE, x, y);
			mMainThread.sendEventToCore(mouse);
		}
		else {
			return super.onTouchEvent(event);
		}
		
		return true;
	}

	public void onBack() {
		mMainThread.sendEventToCore(new NbkEvent(NbkEvent.EVENT_GO_BACK));
	}
	
	public void onForward() {
		mMainThread.sendEventToCore(new NbkEvent(NbkEvent.EVENT_GO_FORWARD));
	}
	
	private void onNewDocBegin() {
		mProgress.reset();
	}
	
	private int getPageDistance() {
		return (int) (mHeight / mDensity * 0.9);
	}
	
	public void pageUp() {
		if (mPageY == 0)
			return;
		
		int y = mPageY - getPageDistance();
		if (y < 0)
			y = 0;
		NbkEvent e = new NbkEvent(NbkEvent.EVENT_SCROLL);
		e.setPos(mPageX, y);
		mMainThread.sendEventToCore(e);
	}
	
	public void pageDown() {
		int d = (int) (mHeight / mDensity);
		if (mPageHeight <= d)
			return;
		if (mPageY >= mPageHeight - d)
			return;
		
		int y = mPageY + getPageDistance();
		if (y > mPageHeight - d)
			y = mPageHeight - d;
		NbkEvent e = new NbkEvent(NbkEvent.EVENT_SCROLL);
		e.setPos(mPageX, y);
		mMainThread.sendEventToCore(e);
	}
}
