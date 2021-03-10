package com.migrsoft.nbk;

import java.awt.Graphics2D;
import java.awt.Rectangle;
import java.awt.image.BufferedImage;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import javax.imageio.ImageIO;

public class ImageMgr {
	
	private static final int MAX_IMAGES = 200;
	
	private  NbkPanel mShell;
	
	private ExecutorService mPool;
	
	private class Nimage implements Runnable {
		
		private int mId;
		
		private ByteArrayOutputStream mStream;
		private BufferedImage mImage;
		
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
				Rectangle clip = mShell.getGraphic().getClip();
				BufferedImage im = mShell.getScreen();
				Graphics2D g2 = (Graphics2D)im.getGraphics();
				g2.setClip(clip);
				g2.drawImage(mImage, x, y, null);
			}
		}
		
		public void drawBg(int x, int y, int w, int h, int rx, int ry) {
			if (mImage != null) {
				Rectangle clip = mShell.getGraphic().getClip();
				BufferedImage im = mShell.getScreen();
				Graphics2D g2 = (Graphics2D)im.getGraphics();
				int iw = mImage.getWidth();
				int ih = mImage.getHeight();
				g2.setClip(clip);
				for (int i = 0; i <= rx; i++) {
					for (int j = 0; j <= ry; j++) {
						g2.drawImage(mImage, x + i * iw, y + j * ih, null);
					}
				}
			}
		}

		@Override
		public void run() {
			if (mStream != null) {
				try {
					ByteArrayInputStream in = new ByteArrayInputStream(mStream.toByteArray());
					mImage = ImageIO.read(in);
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
				mShell.postEventToCore(e);
			}
		}
	}
	
	private Nimage[] mImages;
	
	public ImageMgr(NbkPanel shell) {
		mShell = shell;
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
