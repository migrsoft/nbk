package com.migrsoft.nbk;

import java.util.List;

public class CookieMgr {
	
	private static final boolean DEBUG = false;
	
	private static final int MAX_COOKIES = 50;
	private Cookie[] mList;
	
	public CookieMgr() {
		mList = new Cookie[MAX_COOKIES];
	}

	public synchronized void addCookies(String host, List<String> cookies) {
		if (DEBUG)
			System.out.println("  host: " + host);
		for (int i=0; i < cookies.size(); i++) {
			if (DEBUG)
				System.out.println("  " + (i + 1) + " cookie " + cookies.get(i));
			addCookie(host, cookies.get(i));
		}
	}
	
	private void addCookie(String host, String cookie) {
		Cookie c = new Cookie(host, cookie);
		if (DEBUG)
			c.dump();
		int i;
		int free = -1;
		for (i = 0; i < MAX_COOKIES; i++) {
			if (mList[i] != null) {
				if (mList[i].getDomain().equals(c.getDomain()) && mList[i].getName().equals(c.getName())) {
					if (c.isDeleted()) {
						mList[i] = null;
						if (free == -1)
							free = i;
					}
					else
						mList[i] = c;
					return;
				}
			}
			else if (free == -1) {
				free = i;
			}
		}
		if (free != -1)
			i = free;
		if (i == MAX_COOKIES)
			i = 0;
		mList[i] = c;
	}
	
	public synchronized String getCookie(String host) {
		StringBuilder bu = new StringBuilder();
		boolean begin = true;
		for (int i = 0; i < MAX_COOKIES; i++) {
			Cookie c = mList[i];
			if (c != null && host.endsWith(c.getDomain())) {
				if (!begin)
					bu.append("; ");
				bu.append(c.getName());
				bu.append('=');
				bu.append(c.getValue());
				begin = false;
			}
		}
		return (begin) ? null : bu.toString();
	}
	
	private class Cookie {
		
		private String mDomain = null;
		private String mName = null;
		private String mValue = null;
		private boolean mDelete = false;
		
		public Cookie(String host, String cookie) {
			String[] mod = cookie.split(";");
			for (String c : mod) {
				if (mName == null) {
					int p = c.indexOf('=');
					mName = c.substring(0, p);
					mValue = c.substring(p + 1);
					if (mValue.equals("deleted")) {
						mValue = null;
						mDelete = true;
					}
					continue;
				}
				int p = c.indexOf('=');
				if (p != -1) {
					String key = c.substring(0, p).toLowerCase();
					if (mDomain == null) {
						if (key.contains("domain"))
							mDomain = c.substring(p + 1);
					}
				}
			}
			if (mDomain == null)
				mDomain = host;
		}
		
		public String getDomain() {
			return mDomain;
		}
		
		public String getName() {
			return mName;
		}
		
		public String getValue() {
			return mValue;
		}
		
		public boolean isDeleted() {
			return mDelete;
		}
		
		public void dump() {
			System.out.println("  DOMAIN " + mDomain + " COOKIE " + mName + "( " + mValue + " )");
		}
	}
}
