package com.migrsoft.nbk;

import android.util.Log;

public class NbkCore {
	
	private static final String TAG = "NbkCore";
	
	private INbkGlue mGlue;
	
	private int mViewX;
	private int mViewY;
	
	static {
		System.loadLibrary("nbk");
	}

	private long mTimeBegin;
	
	public NbkCore(INbkGlue glue) {
		mGlue = glue;
		mTimeBegin = System.currentTimeMillis();
		mViewX = mViewY = 0;
	}
	
	public long getTimeBegin() {
		return mTimeBegin;
	}
	
	public int getCurrentMillis() {
		return (int)(System.currentTimeMillis() - mTimeBegin);
	}
	
	public void init() {
		Log.i(TAG, "========== NBK ENGINE START ==========");
		nbkCoreInit();
	}
	
	public void destroy() {
		nbkCoreDestroy();
		Log.i(TAG, "==========  NBK ENGINE END  ==========");
	}
	
	public void setPageWidth(int width) {
		nbkSetPageWidth(width);
	}
		
	public void loadUrl(String url) {
		nbkLoadUrl(url);
	}
	
	public void goForward() {
		nbkHistoryNext();
	}
	
	public void goBack() {
		boolean can = nbkHistoryPrev();
		if (!can) {
			mGlue.sendNbkEventToShell(new NbkEvent(NbkEvent.EVENT_NO_BACK));
		}
	}
	
	public void scrollTo(int x, int y) {
		if (x != mViewX || y != mViewY) {
			mViewX = x;
			mViewY = y;
			nbkScrollTo(x, y);
		}
	}
	
	public void onTimer(int id) {
		nbkOnTimer(id);
	}
	
	public void scheduleNbkEvent() {
		mGlue.sendNbkEventToCore(new NbkEvent(NbkEvent.EVENT_NBK_EVENT));
	}
	
	public void handleNbkEvent() {
		nbkHandleEvent();
	}
	
	public void scheduleNbkCall() {
		mGlue.sendNbkEventToCore(new NbkEvent(NbkEvent.EVENT_NBK_CALL));
	}
	
	public void handleNbkCall() {
		nbkHandleCall();
	}
	
	public void handleEvent(NbkEvent e) {
		switch (e.getType()) {
		case NbkEvent.EVENT_MOUSE_LEFT:
			nbkHandleInputEvent(NbkEvent.EVENT_MOUSE_LEFT, e.getSubType(), e.getX(), e.getY());
			break;
		}
	}
	
	private void updateView(int w, int h, int x, int y) {
		NbkEvent e = new NbkEvent(NbkEvent.EVENT_UPDATE_VIEW);
		e.setSize(w, h);
		e.setPos(x, y);
		mGlue.sendNbkEventToShell(e);
	}
	
	private void notifyState(int state, int v1, int v2) {
		NbkEvent e;
		switch (state) {
		case 6: // NBK_EVENT_NEW_DOC_BEGIN
			e = new NbkEvent(NbkEvent.EVENT_NEW_DOC_BEGIN);
			mGlue.sendNbkEventToShell(e);
			break;
			
		case 7: // NBK_EVENT_GOT_DATA
			e = new NbkEvent(NbkEvent.EVENT_PROGRESS_DOC);
			e.setProgress(v1,  v2);
			mGlue.sendNbkEventToShell(e);
			break;
			
		case 11: // NBK_EVENT_DOWNLOAD_IMAGE
			e = new NbkEvent(NbkEvent.EVENT_PROGRESS_PIC);
			e.setProgress(v1,  v2);
			mGlue.sendNbkEventToShell(e);
			break;
		}
	}
	
	public int getScreenWidth() {
		return mGlue.getScreenWidth();
	}
	
	public int getScreenHeight() {
		return mGlue.getScreenHeight();
	}
	
	private native void nbkCoreInit();
	private native void nbkCoreDestroy();
	
	private native void nbkSetPageWidth(int width);

	private native void nbkHandleEvent();
	private native void nbkHandleInputEvent(int button, int stat, int x, int y);
	private native void nbkOnTimer(int id);
	private native void nbkHandleCall();
	
	private native void nbkLoadUrl(String url);
	private native boolean nbkHistoryPrev();
	private native boolean nbkHistoryNext();
	
	private native void nbkScrollTo(int x, int y);
}
