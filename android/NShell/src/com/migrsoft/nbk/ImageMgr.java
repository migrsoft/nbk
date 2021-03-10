package com.migrsoft.nbk;

import java.io.ByteArrayOutputStream;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.Region;
import android.util.DisplayMetrics;
import android.util.Log;

public class ImageMgr {
	
	private static final String TAG = "NbkImageMgr";
	
	private static final int MAX_IMAGES = 200;
	
	private INbkGlue mGlue;
	
	private ExecutorService mPool;
	
	private class Nimage implements Runnable {
		
		private int mId;
		
		private ByteArrayOutputStream mStream;
		private Bitmap mImage;
		
		private boolean mCancel;
		
		public Nimage(int id) {
			mId = id;
			mCancel = false;
		}
		
		public void onData(byte[] data) {
			if (mStream == null)
				mStream = new ByteArrayOutputStream();
			try {
				mStream.write(data);
			}
			catch (Exception e) {
			}
		}
		
		public void onDataEnd(boolean ok) {
			try {
				mStream.close();
			}
			catch (Exception e) {				
			}
			if (!ok)
				mStream = null;
		}
		
		public void cancel() {
			mCancel = true;
		}
		
		public void draw(int x, int y, int w, int h) {
			if (mImage != null) {
				Canvas c = mGlue.getCanvas();
				Rect dst = new Rect(x, y, x + w, y + h);
				c.drawBitmap(mImage, null, dst, null);
			}
		}
		
		public void drawBg(int x, int y, int w, int h, int rx, int ry) {
			if (mImage != null) {
				int iw = mImage.getWidth();
				int ih = mImage.getHeight();
				Canvas c = mGlue.getCanvas();
				for (int i = 0; i <= rx; i++) {
					for (int j = 0; j <= ry; j++) {
						c.drawBitmap(mImage, x + i * iw, y + j * ih, null);
					}
				}
			}
		}

		@Override
		public void run() {
			if (mStream != null) {
				try {
					byte[] data = mStream.toByteArray();
					BitmapFactory.Options options = new BitmapFactory.Options();
					options.inPreferredConfig = Bitmap.Config.RGB_565;
					mImage = BitmapFactory.decodeByteArray(data, 0, data.length, options);
//					if (mImage != null)
//						Log.i(TAG, "  " + mImage.getWidth() + " x " + mImage.getHeight());
				}
				catch (Exception e) {
				}
				finally {
					mStream = null;
				}
			}
			
			if (!mCancel) {
				NbkEvent e = new NbkEvent(NbkEvent.EVENT_IMAGE_DECODE);
				e.setId(mId);
				e.setResult(mImage != null);
				if (e.getResult()) {
					e.setSize(mImage.getWidth(), mImage.getHeight());
				}
				mGlue.sendNbkEventToCore(e);
			}
		}
	}
	
	private Nimage[] mImages;
	
	public ImageMgr(INbkGlue glue) {
		mGlue = glue;
		mPool = Executors.newFixedThreadPool(1);
		mImages = new Nimage[MAX_IMAGES];
	}

	private void createImage(int id) {
		mImages[id] = new Nimage(id);
	}
	
	private void deleteImage(int id) {
		if (mImages[id] != null) {
			mImages[id].cancel();
			mImages[id] = null;
		}
	}
	
	private void onImageData(int id, byte[] data) {
		mImages[id].onData(data);
	}
	
	private void onImageEnd(int id, boolean ok) {
		mImages[id].onDataEnd(ok);
	}
	
	private void decodeImage(int id) {
		mPool.submit(mImages[id]);
	}
	
	private void drawImage(int id, int x, int y, int w, int h) {
		mImages[id].draw(x, y, w, h);
	}
	
	private void drawBgImage(int id, int x, int y, int w, int h, int rx, int ry) {
		mImages[id].drawBg(x, y, w, h, rx, ry);
	}
	
	public void onImageDecode(NbkEvent e) {
		nbkImageDecodeOver(e.getId(), e.getResult(), e.getWidth(), e.getHeight());
	}
	
	public native void register();
	
	private native void nbkImageDecodeOver(int id, boolean ok, int w, int h);
}
