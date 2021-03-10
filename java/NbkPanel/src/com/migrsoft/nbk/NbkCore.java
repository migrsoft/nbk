package com.migrsoft.nbk;

import java.awt.Color;
import java.awt.Graphics2D;
import java.awt.image.BufferedImage;

public class NbkCore {
	
	private NbkPanel mShell;
	
	private int mViewX;
	private int mViewY;
	
	static {
		System.loadLibrary("nbk");
	}

	private long mTimeBegin;
	
	public NbkCore(NbkPanel shell) {
		mShell = shell;
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
		System.out.println("nbk core init");
		nbkCoreInit();
		mShell.getLoader().register();
		mShell.getTimerMgr().register();
		mShell.getGraphic().register();
		mShell.getImageMgr().register();
	}
	
	public void destroy() {
		System.out.println("nbk core destroy");
		nbkCoreDestroy();
	}
	
	private void clearScreen() {
		BufferedImage screen = mShell.getScreen();
		Graphics2D g2 = (Graphics2D)screen.getGraphics();
		g2.setBackground(Color.WHITE);
		g2.clearRect(0, 0, screen.getWidth(), screen.getHeight());
	}
	
	public void loadUrl(String url) {
		nbkLoadUrl(url);
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
		mShell.postEventToCore(new NbkEvent(NbkEvent.EVENT_NBK_EVENT));
	}
	
	public void handleNbkEvent() {
		nbkHandleEvent();
	}
	
	public void scheduleNbkCall() {
		mShell.postEventToCore(new NbkEvent(NbkEvent.EVENT_NBK_CALL));
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
	
	private void updateView(int w, int h) {
		NbkEvent e = new NbkEvent(NbkEvent.EVENT_UPDATE_VIEW);
		e.setSize(w, h);
		mShell.postEventToShell(e);
	}
	
	private void notifyState(int state, int v1, int v2) {
		switch (state) {
		case 6: // NBK_EVENT_NEW_DOC_BEGIN
			clearScreen();
			break;
		}
	}
	
	private int getScreenWidth() {
		return mShell.getScreenWidth();
	}
	
	private int getScreenHeight() {
		return mShell.getScreenHeight();
	}
	
	private native void nbkCoreInit();
	private native void nbkCoreDestroy();

	private native void nbkHandleEvent();
	private native void nbkHandleInputEvent(int button, int stat, int x, int y);
	private native void nbkOnTimer(int id);
	private native void nbkHandleCall();
	
	private native void nbkLoadUrl(String url);
	
	private native void nbkScrollTo(int x, int y);
}
