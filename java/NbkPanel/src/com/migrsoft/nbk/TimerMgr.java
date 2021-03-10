package com.migrsoft.nbk;

import java.util.Timer;
import java.util.TimerTask;

public class TimerMgr {
	
	private final int MAX_TIMERS = 256;
	
	private NbkPanel mShell;
	
	private Timer mTimer;
	
	private boolean[] mStopStatus;
	
	public TimerMgr(NbkPanel shell) {
		mShell = shell;
		mTimer = new Timer();
		mStopStatus = new boolean[MAX_TIMERS];
	}
	
	public synchronized void start(int id, int delay, int interval) {
		mStopStatus[id] = false;
		TimerJob job = new TimerJob(id);
		mTimer.schedule(job, delay, interval);
	}
	
	public synchronized void stop(int id) {
		mStopStatus[id] = true;
	}
	
	private synchronized boolean isStop(int id) {
		return mStopStatus[id];
	}
	
	private class TimerJob extends TimerTask {
		
		private int mId;
		
		public TimerJob(int id) {
			mId = id;
		}

		@Override
		public void run() {
			if (isStop(mId)) {
				cancel();
				return;
			}
			
			NbkEvent timer = new NbkEvent(NbkEvent.EVENT_TIMER);
			timer.setPageId(mId);
			mShell.postEventToCore(timer);
		}
	}
	
	public native void register();
}
