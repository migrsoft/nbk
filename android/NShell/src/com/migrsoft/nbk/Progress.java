package com.migrsoft.nbk;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.Paint.Style;

public class Progress {
	
	private Rect mArea;

	private int mDocReceived;
	private int mDocTotal;
	private int mPicReceived;
	private int mPicTotal;
	
	private boolean mInDoc;
	private boolean mInPic;
	private Paint mPaint;
	
	public Progress() {
		mPaint = new Paint();
		mPaint.setColor(Color.BLUE);
		mPaint.setStyle(Style.FILL);
		reset();
	}
	
	public void reset() {
		mDocReceived = 0;
		mDocTotal = -1;
		mPicReceived = 0;
		mPicTotal = 0;
		mInDoc = false;
		mInPic = false;
	}
	
	public void setArea(Rect area) {
		mArea = area;
	}
	
	public void setDocProgress(int curr, int total) {
		mInDoc = true;
		mDocReceived = curr;
		mDocTotal = (total == -1) ? 50000 : total;
	}
	
	public void setPicProgress(int curr, int total) {
		mInDoc = false;
		mInPic = true;
		mPicReceived = curr;
		mPicTotal = total;
	}
	
	public void draw(Canvas canvas) {
		int w;
		int p;
		Rect r = new Rect();
		if (mInDoc) {
			w = mArea.width() / 2;
			p = calcPos(mDocReceived, mDocTotal, w);
			r.set(mArea.left, mArea.top, mArea.left + p, mArea.bottom);
			canvas.drawRect(r, mPaint);
		}
		else if (mInPic) {
			if (mPicReceived < mPicTotal) {
				w = mArea.width() / 2;
				r.set(mArea.left, mArea.top, w, mArea.bottom);
				canvas.drawRect(r, mPaint);
				p = calcPos(mPicReceived, mPicTotal, w);
				r.set(mArea.left + w, mArea.top, mArea.left + w + p, mArea.bottom);
				canvas.drawRect(r, mPaint);
			}
			else
				mInPic = false;
		}
	}
	
	private int calcPos(int cur, int total, int extent) {
		if (cur > total)
			cur = total;
		return cur * extent / total;
	}
}
