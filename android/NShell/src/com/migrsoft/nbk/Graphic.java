package com.migrsoft.nbk;

import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Region;
import android.graphics.Paint.Style;
import android.graphics.Rect;
import android.graphics.RectF;
import android.text.TextPaint;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.TypedValue;

public class Graphic {
	
	private static final String TAG = "NbkGraphic";
	
	private final int MAX_FONTS = 32;
	
	private int mColor;
	private int mBgColor;
	
	private Bitmap mBitmap;
	private Canvas mCanvas;
	private Rect mClip = new Rect();
	private Paint mPaint = new Paint();
	
	private DisplayMetrics mDisplayMetrics;
	
	private class FontInfo {
		private TextPaint mFont;
		private int mPixel;
		private boolean mBold;
		private boolean mItalic;
		private int mHeight;
		private int mAscent;
		
		public FontInfo(int pixel, boolean bold, boolean italic) {
			mPixel = pixel;
			mBold = bold;
			mItalic = italic;

			mFont = new TextPaint(Paint.ANTI_ALIAS_FLAG);
			
			mFont.setTextSize(TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_PX, pixel, mDisplayMetrics));
			mFont.density = mDisplayMetrics.density;
			
			if (mBold)
				mFont.setFakeBoldText(true);
			if (mItalic)
				mFont.setTextSkewX(-0.25f);
			
			Paint.FontMetricsInt fm = mFont.getFontMetricsInt();
			mHeight = -fm.ascent + fm.descent;
			mAscent = -fm.ascent;
			
//			Log.i(TAG, "font " + mPixel + " " + mFont.getTextSize() + ", " + fm);
		}
		
		public boolean equals(int pixel, boolean bold, boolean italic) {
			if (mPixel == pixel && mBold == bold && mItalic == italic)
				return true;
			return false;
		}
		
		public int getHeight() {
			return mHeight;
		}
		
		public int getAscent() {
			return mAscent;
		}
		
		public int getStringWidth(String text) {
			return (int) mFont.measureText(text);
		}
		
		public Paint getFont() {
			return mFont;
		}
	}
	
	private FontInfo[] mFonts;
	private int mCurFont;
	
	public Graphic() {
		mFonts = new FontInfo[MAX_FONTS];
		mCurFont = -1;
		
		mDisplayMetrics = Resources.getSystem().getDisplayMetrics();
	}
	
	public void createCanvas(int width, int height) {
		
		float density = mDisplayMetrics.density;
		int w = (int) (width * density);
		int h = (int) (height * density);
		
		if (mBitmap != null && (mBitmap.getWidth() == w && mBitmap.getHeight() == h)) return;
		
		mBitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
		mCanvas = new Canvas(mBitmap);
		mCanvas.scale(density, density);
	}
	
	public Bitmap getBitmap() {
		return mBitmap;
	}
	
	public Canvas getCanvas() {
		mCanvas.clipRect(mClip, Region.Op.REPLACE);
		return mCanvas;
	}

	public native void register();
	
	public void reset() {
		for (int i=0; i < MAX_FONTS; i++) {
			mFonts[i] = null;
		}
	}
	
	public int getFont(int pixel, boolean bold, boolean italic) {

		int i;
		for (i = 0; i < MAX_FONTS && mFonts[i] != null; i++) {
			FontInfo f = mFonts[i];
			if (f.equals(pixel, bold, italic))
				return i;
		}
		
		if (i == MAX_FONTS)
			return -1;
		
		mFonts[i] = new FontInfo(pixel, bold, italic);
		return i;
	}
	
	public void useFont(int fontId) {
		mCurFont = fontId;
	}
	
	public int getFontHeight(int fontId) {
		if (fontId >= 0 && fontId < MAX_FONTS) {
			return mFonts[fontId].getHeight();
		}
		return 0;
	}
	
	public int getTextWidth(int fontId, String text) {
		if (fontId >= 0 && fontId < MAX_FONTS) {
			return mFonts[fontId].getStringWidth(text);
		}
		return 0;
	}
	
	public void setColor(int r, int g, int b, int a) {
		mColor = Color.argb(a, r, g, b);
		mPaint.setColor(mColor);
	}
	
	public void setBgColor(int r, int g, int b, int a) {
		mBgColor = Color.argb(a, r, g, b);
	}
	
	public void drawText(String text, int x, int y, int decor) {
		if (mCurFont == -1 || mFonts[mCurFont] == null)
			return;
		
		mCanvas.clipRect(mClip, Region.Op.REPLACE);
		Paint p = mFonts[mCurFont].getFont();
		p.setColor(mColor);
		mCanvas.drawText(text, x, y + mFonts[mCurFont].getAscent(), p);
	}
	
	public void drawRect(int x, int y, int w, int h) {
//		Log.i(TAG, "drawRect " + x + " " + y + " " + w + " " + h);
		mCanvas.clipRect(mClip, Region.Op.REPLACE);
		mPaint.setStyle(Style.STROKE);
		mPaint.setColor(mColor);
		mCanvas.drawRect(x, y, x + w, y + h, mPaint);
	}
	
	public void fillRect(int x, int y, int w, int h) {
//		Log.i(TAG, "fillRect " + x + ", " + y + " " + w + " x " + h);
		mCanvas.clipRect(mClip, Region.Op.REPLACE);
		mPaint.setStyle(Style.FILL);
		mPaint.setColor(mBgColor);
		mCanvas.drawRect(new Rect(x, y, x + w, y + h), mPaint);
	}
	
	public void setClip(int x, int y, int w, int h) {
//		Log.i(TAG, "setClip " + x + ", " + y + " " + w + " x " + h);
		mClip.set(x, y, x + w, y + h);
	}
	
	public void cancelClip() {
		mClip.set(0, 0, mBitmap.getWidth(), mBitmap.getHeight());
	}
	
	public void drawOval(int x, int y, int w, int h) {
		mCanvas.clipRect(mClip, Region.Op.REPLACE);
		mCanvas.drawOval(new RectF(x, y, x + w, y + h), mPaint);
	}
}
