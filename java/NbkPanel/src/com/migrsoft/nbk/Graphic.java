package com.migrsoft.nbk;

import java.awt.Color;
import java.awt.Font;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.Image;
import java.awt.Rectangle;
import java.awt.font.FontRenderContext;
import java.awt.font.LineMetrics;
import java.awt.geom.Rectangle2D;
import java.awt.image.BufferedImage;

public class Graphic {
	
	private NbkPanel mShell;
	
	private final int MAX_FONTS = 32;
	
	private Color mColor;
	private Color mBgColor;
	
	private Rectangle mClip;
	
	private class FontInfo {
		private Font mFont;
		private int mPixel;
		private boolean mBold;
		private boolean mItalic;
		private int mHeight;
		private int mAscent;
		
		public FontInfo(int pixel, boolean bold, boolean italic) {
			int style = 0;
			if (bold)
				style += Font.BOLD;
			if (italic)
				style += Font.ITALIC;
			mFont = new Font("新宋体", style, pixel);
			mPixel = pixel;
			mBold = bold;
			mItalic = italic;
			
			BufferedImage img = mShell.getScreen();
			Graphics2D g2 = (Graphics2D)img.getGraphics();
			FontRenderContext frc = g2.getFontRenderContext();
			LineMetrics lm = mFont.getLineMetrics("汉", frc);
			mHeight = (int) lm.getHeight();
			mAscent = (int) lm.getAscent();
		}
		
		public Font getFont() {
			return mFont;
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
		
		public int getStringWidth(String c) {
			BufferedImage img = mShell.getScreen();
			Graphics2D g2 = (Graphics2D)img.getGraphics();
			FontRenderContext frc = g2.getFontRenderContext();
			Rectangle2D r = mFont.getStringBounds(c, frc);
			return (int) r.getWidth();
		}
	}
	
	private FontInfo[] mFonts;
	private int mCurFont;
	
	public Graphic(NbkPanel shell) {
		mShell = shell;
		mFonts = new FontInfo[MAX_FONTS];
		mCurFont = -1;
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
		mColor = new Color(r, g, b, a);
	}
	
	public void setBgColor(int r, int g, int b, int a) {
		mBgColor = new Color(r, g, b, a);
	}
	
	public void drawText(String text, int x, int y, int decor) {
		if (mCurFont == -1 || mFonts[mCurFont] == null)
			return;
		
		BufferedImage img = mShell.getScreen();
		Graphics2D g2 = (Graphics2D) img.getGraphics();
		g2.setClip(mClip);
		g2.setPaint(mColor);
		g2.setFont(mFonts[mCurFont].getFont());
		g2.drawString(text, x, y + mFonts[mCurFont].getAscent());
	}
	
	public void drawRect(int x, int y, int w, int h) {
		BufferedImage img = mShell.getScreen();
		Graphics2D g2 = (Graphics2D) img.getGraphics();
		g2.setClip(mClip);
		g2.setPaint(mColor);
		g2.drawRect(x, y, w - 1, h - 1);
	}
	
	public void fillRect(int x, int y, int w, int h) {
		BufferedImage img = mShell.getScreen();
		Graphics2D g2 = (Graphics2D) img.getGraphics();
		g2.setClip(mClip);
		g2.setPaint(mColor);
		g2.fillRect(x, y, w, h);
	}
	
	public void setClip(int x, int y, int w, int h) {
		mClip = new Rectangle(x, y, w, h);
	}
	
	public void cancelClip() {
		int w = mShell.getScreenWidth();
		int h = mShell.getScreenHeight();
		int pw = mShell.getPageWidth();
		mClip = new Rectangle(0, 0, Math.min(w, pw), h);
	}
	
	public Rectangle getClip() {
		return mClip;
	}
	
	public void drawOval(int x, int y, int w, int h) {
		BufferedImage img = mShell.getScreen();
		Graphics g = img.getGraphics();
		g.setClip(mClip);
		g.drawOval(x, y, w, h);
	}
}
