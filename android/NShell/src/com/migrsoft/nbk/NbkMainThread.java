package com.migrsoft.nbk;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

public class NbkMainThread extends Thread {
	
	private Context mContext;
	
	private NbkCore mNbkCore;
	private LoaderMgr mLoaderMgr;
	private TimerMgr mTimerMgr;
	private Graphic mGraphic;
	private ImageMgr mImageMgr;
	
	private Handler mNbkHandler;
	private Handler mGuiHandler;
	
	private boolean mReady = false;
	
	private int mScreenWidth;
	private int mScreenHeight;
	
	private class NbkGlue implements INbkGlue {

		@Override
		public int getScreenWidth() {
			return mScreenWidth;
		}

		@Override
		public int getScreenHeight() {
			return mScreenHeight;
		}

		@Override
		public Canvas getCanvas() {
			return mGraphic.getCanvas();
		}

		@Override
		public void sendNbkEventToCore(NbkEvent e) {
			sendEventToCore(e);
		}

		@Override
		public void sendNbkEventToShell(NbkEvent e) {
			sendEventToGui(e);
		}
	}
	
	private NbkGlue mGlue = new NbkGlue();
	
	public NbkMainThread(Context ctx) {
		
		setName("ENGINE");
		
		mContext = ctx;
		
		mNbkCore = new NbkCore(mGlue);
		mLoaderMgr = new LoaderMgr(mGlue, mContext);
		mTimerMgr = new TimerMgr(mGlue);
		mGraphic = new Graphic();
		mImageMgr = new ImageMgr(mGlue);
	}
	
	public void setGuiHandler(Handler handler) {
		mGuiHandler = handler;
	}

	@Override
	public void run() {
		
		Looper.prepare();
		
		mNbkHandler = new Handler() {

			@Override
			public void handleMessage(Message msg) {
				handleCoreEvent((NbkEvent) msg.obj);
			}
		};
		
//		sendEventToCore(new NbkEvent(NbkEvent.EVENT_INIT));
		
		Looper.loop();
	}

	private void handleCoreEvent(NbkEvent e) {
		
//		Log.i("NbkCoreLoop", Thread.currentThread().getName() + " event " + e.getType());
		
		switch (e.getType()) {
		case NbkEvent.EVENT_QUIT:
			mNbkCore.destroy();
			break;
			
		case NbkEvent.EVENT_SCREEN_CHANGED:
			mScreenWidth = e.getWidth();
			mScreenHeight = e.getHeight();
			
			if (!mReady) {
				mReady = true;
				mNbkCore.init();
				mLoaderMgr.register();
				mTimerMgr.register();
				mGraphic.register();
				mImageMgr.register();
				sendEventToGui(new NbkEvent(NbkEvent.EVENT_INIT));
			}
			
			mNbkCore.setPageWidth(mScreenWidth);
			mGraphic.createCanvas(mScreenWidth, mScreenHeight);
			break;
			
		case NbkEvent.EVENT_LOAD_URL:
			mNbkCore.loadUrl(e.getUrl());
			break;
			
		case NbkEvent.EVENT_GO_BACK:
			mNbkCore.goBack();
			break;
			
		case NbkEvent.EVENT_GO_FORWARD:
			mNbkCore.goForward();
			break;
			
		case NbkEvent.EVENT_RECEIVE_HEADER:
		case NbkEvent.EVENT_RECEIVE_DATA:
		case NbkEvent.EVENT_RECEIVE_COMPLETE:
		case NbkEvent.EVENT_RECEIVE_ERROR:
			mLoaderMgr.onLoaderResponse(e);
			break;
			
		case NbkEvent.EVENT_NBK_EVENT:
			mNbkCore.handleNbkEvent();
			break;
			
		case NbkEvent.EVENT_NBK_CALL:
			mNbkCore.handleNbkCall();
			break;
			
		case NbkEvent.EVENT_TIMER:
			mNbkCore.onTimer(e.getPageId());
			break;
			
		case NbkEvent.EVENT_MOUSE_LEFT:
			mNbkCore.handleEvent(e);
			break;
			
		case NbkEvent.EVENT_SCROLL:
			mNbkCore.scrollTo(e.getX(), e.getY());
			break;
		
		case NbkEvent.EVENT_IMAGE_DECODE:
			mImageMgr.onImageDecode(e);
			break;
		}
	}
	
	public void sendEventToCore(NbkEvent event) {
		if (mNbkHandler == null) return;
		
		Message msg = mNbkHandler.obtainMessage();
		msg.obj = event;
		mNbkHandler.sendMessage(msg);
	}
	
	private void sendEventToGui(NbkEvent event) {
		if (mGuiHandler == null) return;
		
		Message msg = mGuiHandler.obtainMessage();
		msg.obj = event;
		mGuiHandler.sendMessage(msg);
	}
	
	public boolean isReady() {
		return mReady;
	}
	
	public synchronized Bitmap getBitmap() {
		return mGraphic.getBitmap();
	}
}
