package com.migrsoft.nbk;

import java.awt.Component;
import java.awt.Container;
import java.awt.Dimension;
import java.awt.EventQueue;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.Insets;
import java.awt.LayoutManager;
import java.awt.event.AdjustmentEvent;
import java.awt.event.AdjustmentListener;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.awt.image.BufferedImage;
import java.util.LinkedList;
import java.util.concurrent.ConcurrentLinkedQueue;

import javax.swing.JComponent;
import javax.swing.JScrollBar;

public class NbkPanel extends JComponent {

	/**
	 * 
	 */
	private static final long serialVersionUID = -4093278713226913698L;
	
	private JScrollBar mVbar;
	private JScrollBar mHbar;
	
	private int mWidth;
	private int mHeight;
	private BufferedImage[] mScreen;
	
	private LinkedList<NbkEvent> mCoreEventQueue;
	private ConcurrentLinkedQueue<NbkEvent> mShellEventQueue;
	
	private NbkCore mNbkCore;
	private LoaderMgr mLoaderMgr;
	private TimerMgr mTimerMgr;
	private Graphic mGraphic;
	private ImageMgr mImageMgr;
	private CookieMgr mCookieMgr;
	
	private Thread mCoreThread;
	
	private int mPageWidth;
	private int mPageHeight;
	
	private boolean mNbkCoreInit;
	
	private class Layout implements LayoutManager {
		
		private int mWidth;
		private int mHeight;
		
		public Layout() {
			mWidth = 10;
			mHeight = 10;
		}

		@Override
		public void addLayoutComponent(String name, Component c) {
		}

		@Override
		public void removeLayoutComponent(Component c) {
		}

		@Override
		public Dimension minimumLayoutSize(Container parent) {
			Dimension dim = new Dimension(0, 0);
			dim.width = mWidth;
			dim.height = mHeight;
			return dim;
		}

		@Override
		public Dimension preferredLayoutSize(Container parent) {
			Dimension dim = new Dimension(0, 0);
			int w = 0;
			int h = 0;
			int n = parent.getComponentCount();
			for (int i = 0; i < n; i++) {
				Component c = parent.getComponent(i);
				if (c.isVisible() && c instanceof JScrollBar) {
					Dimension d = c.getPreferredSize();
					w += d.width;
					h += d.height;
				}
			}
			
			Insets insets = parent.getInsets();
			dim.width = w + insets.left + insets.right;
			dim.height = h + insets.top + insets.bottom;
			
			mWidth = dim.width;
			mHeight = dim.height;
			
			return dim;
		}

		@Override
		public void layoutContainer(Container parent) {
			Insets insets = parent.getInsets();
			int maxWidth = parent.getWidth() - (insets.left + insets.right);
			int maxHeight = parent.getHeight() - (insets.top + insets.bottom);
			int x = insets.left;
			int y = insets.top;
			int n = parent.getComponentCount();

			JScrollBar vbar = null;
			JScrollBar hbar = null;
			for (int i = 0; i < n; i++) {
				Component c = parent.getComponent(i);
				if (c instanceof JScrollBar) {
					if (((JScrollBar)c).getOrientation() == JScrollBar.VERTICAL) {
						vbar = (JScrollBar) c;
					}
					else {
						hbar = (JScrollBar) c;
					}
				}
			}
			
			if (hbar.isVisible()) {
				Dimension dv = vbar.getPreferredSize();
				Dimension dh = hbar.getPreferredSize();
				vbar.setBounds(x + maxWidth - dv.width, y, dv.width, maxHeight - dh.height);
				hbar.setBounds(x, y + maxHeight - dh.height, maxWidth - dv.width, dh.height);
			}
			else {
				Dimension dv = vbar.getPreferredSize();
				vbar.setBounds(x + maxWidth - dv.width, y, dv.width, maxHeight);
			}
		}
	}
	
	private class ScrollbarListener implements AdjustmentListener {

		@Override
		public void adjustmentValueChanged(AdjustmentEvent e) {
			if (mNbkCoreInit == false)
				return;
			if (e.getSource() instanceof JScrollBar) {
				JScrollBar bar = (JScrollBar) e.getSource();
				NbkEvent evt = new NbkEvent(NbkEvent.EVENT_SCROLL);
				if (bar.getOrientation() == JScrollBar.VERTICAL) {
					evt.setPos(0, e.getValue());
				}
				else {
					evt.setPos(e.getValue(), 0);
				}
				postEventToCore(evt);
			}
		}
	}
	
	public NbkPanel() {
		
		mNbkCoreInit = false;
		
		setLayout(new Layout());
		
		mWidth = 320;
		mHeight = 480;
		
		mPageWidth = 0;
		mPageHeight = 0;
		
		mVbar = new JScrollBar();
		mVbar.setUnitIncrement(20);
		mVbar.setBlockIncrement(400);
		mHbar = new JScrollBar(JScrollBar.HORIZONTAL);
		mHbar.setUnitIncrement(20);
		mHbar.setBlockIncrement(400);
		
		ScrollbarListener scrollListener = new ScrollbarListener();
		mVbar.addAdjustmentListener(scrollListener);
		mHbar.addAdjustmentListener(scrollListener);
		
		updateScrollBar();
		add(mVbar);
		add(mHbar);
		
		mScreen = new BufferedImage[2];
		for (int i = 0; i < 2; i++) {
			mScreen[i] = new BufferedImage(mWidth, mHeight, BufferedImage.TYPE_INT_ARGB);
		}
		
		mCoreEventQueue = new LinkedList<NbkEvent>();
		mShellEventQueue = new ConcurrentLinkedQueue<NbkEvent>();
		
		mNbkCore = new NbkCore(this);
		mLoaderMgr = new LoaderMgr(this);
		mTimerMgr = new TimerMgr(this);
		mGraphic = new Graphic(this);
		mImageMgr = new ImageMgr(this);
		mCookieMgr = new CookieMgr();
		
		startCoreThread();
		
		postEventToCore(new NbkEvent(NbkEvent.EVENT_INIT));

		setFocusable(true);
		
		addMouseListener(new MouseHandler());
	}
	
	private void startCoreThread() {
		
		Runnable r = new Runnable() {

			@Override
			public void run() {
				
				System.out.println("start event loop ==========>>>");

				boolean run = true;
				while (run) {
					try {
						NbkEvent e = null;
						
						synchronized(mCoreEventQueue) {
							while (mCoreEventQueue.isEmpty())
								mCoreEventQueue.wait();
							
							e = mCoreEventQueue.poll();
						}
						if (e == null)
							continue;
						
//						System.out.println("JAVA EVENT - " + e.getType());
						
						switch (e.getType()) {
						case NbkEvent.EVENT_INIT:
							mNbkCore.init();
							postEventToShell(new NbkEvent(NbkEvent.EVENT_INIT));
							break;
							
						case NbkEvent.EVENT_QUIT:
							mNbkCore.destroy();
							run = false;
							break;
							
						case NbkEvent.EVENT_LOAD_URL:
							mNbkCore.loadUrl(e.getUrl());
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
					catch (Exception e) {
					}
				}
				
				System.out.println("<<<========== end event loop");
			}
		};
		
		mCoreThread = new Thread(r);
		mCoreThread.setName("NBK-Core");
		mCoreThread.start();
	}

	public void abortCore() {
		postEventToCore(new NbkEvent(NbkEvent.EVENT_QUIT));
		
		try {
			mCoreThread.join();
		}
		catch (Exception e) {
		}
	}
	
	private class MouseHandler extends MouseAdapter {

		@Override
		public void mousePressed(MouseEvent e) {
			requestFocus();
			if (e.getButton() == MouseEvent.BUTTON1) {
				NbkEvent mouse = new NbkEvent(NbkEvent.EVENT_MOUSE_LEFT);
				mouse.setClick(NbkEvent.MOUSE_PRESS, e.getX(), e.getY());
				postEventToCore(mouse);
			}
		}

		@Override
		public void mouseReleased(MouseEvent e) {
			if (e.getButton() == MouseEvent.BUTTON1) {
				NbkEvent mouse = new NbkEvent(NbkEvent.EVENT_MOUSE_LEFT);
				mouse.setClick(NbkEvent.MOUSE_RELEASE, e.getX(), e.getY());
				postEventToCore(mouse);
			}
		}
		
	}

	public LoaderMgr getLoader() {
		return mLoaderMgr;
	}
	
	public NbkCore getNbkCore() {
		return mNbkCore;
	}
	
	public TimerMgr getTimerMgr() {
		return mTimerMgr;
	}
	
	public Graphic getGraphic() {
		return mGraphic;
	}
	
	public ImageMgr getImageMgr() {
		return mImageMgr;
	}
	
	public CookieMgr getCookieMgr() {
		return mCookieMgr;
	}
	
	private void handleShellEvent() {
		
		while (!mShellEventQueue.isEmpty()) {
			NbkEvent e = mShellEventQueue.poll();
			
			switch (e.getType()) {
			case NbkEvent.EVENT_INIT:
				mNbkCoreInit = true;
				break;
				
			case NbkEvent.EVENT_UPDATE_VIEW:
				if (e.getWidth() != mPageWidth || e.getHeight() != mPageHeight) {
					mPageWidth = e.getWidth();
					mPageHeight = e.getHeight();
					updateScrollBar();
				}
				updatePage();
				break;
			}
		}
	}
	
	private void updatePage() {
		Graphics2D g2 = (Graphics2D)mScreen[0].getGraphics();
		g2.drawImage(mScreen[1], 0, 0, null);
		repaint();
	}

	@Override
	protected void paintComponent(Graphics g) {
		super.paintComponent(g);
		
		Graphics2D g2 = (Graphics2D)g;
		
		g2.drawImage(mScreen[0], 0, 0, null);
	}
	
	public void postEventToCore(NbkEvent event) {
		synchronized(mCoreEventQueue) {
			mCoreEventQueue.add(event);
			if (mCoreEventQueue.peek() == event)
				mCoreEventQueue.notify();
		}
	}

	public void postEventToShell(NbkEvent event) {
		mShellEventQueue.add(event);
		
		Runnable r = new Runnable() {
			@Override
			public void run() {
				handleShellEvent();
			}
		};
		
		EventQueue.invokeLater(r);
	}
	
	public BufferedImage getScreen() {
		return mScreen[1];
	}
	
	public int getScreenWidth() {
		return mWidth;
	}
	
	public int getScreenHeight() {
		return mHeight;
	}
	
	public int getPageWidth() {
		return mPageWidth;
	}
	
	public int getPageHeight() {
		return mPageHeight;
	}
	
	public void loadUrl(String url) {
		NbkEvent load = new NbkEvent(NbkEvent.EVENT_LOAD_URL);
		load.setUrl(url);
		postEventToCore(load);
	}
	
	private void updateScrollBar() {
		if (mPageWidth <= mWidth) {
			mHbar.setVisible(false);
		}
		else {
			mHbar.setVisible(true);
			mHbar.setValues(0, mWidth, 0, mPageWidth);
		}
		
		if (mPageHeight <= mHeight) {
			mVbar.setMinimum(0);
			mVbar.setMaximum(0);
		}
		else {
			mVbar.setValues(0, mHeight, 0, mPageHeight);
		}
	}
}
