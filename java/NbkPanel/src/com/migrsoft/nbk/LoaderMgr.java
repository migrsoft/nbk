package com.migrsoft.nbk;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.zip.GZIPInputStream;

public class LoaderMgr {
	
	private final int BUFFER_SIZE = 16384;
	
	private NbkPanel mShell;
	
	private ExecutorService mPool;
	
	public LoaderMgr(NbkPanel shell) {
		mShell = shell;
		mPool = Executors.newFixedThreadPool(2);
	}
	
	public void loadFile(int pageId, int connId, String path) {
		LoadFile r = new LoadFile(pageId, connId, path);
		mPool.submit(r);
	}
	
	public void httpGet(int pageId, int connId, String url, String referer) {
		HttpGet r = new HttpGet(pageId, connId, url, referer);
		mPool.submit(r);
	}
	
	private class LoadData {
		protected int mPageId;
		protected int mConnId;
		protected String mUrl;
		
		public int getPageId() {
			return mPageId;
		}
		
		public int getId() {
			return mConnId;
		}
		
		public String getUrl() {
			return mUrl;
		}
	}

	private class LoadFile extends LoadData implements Runnable {

		public LoadFile(int pageId, int connId, String path) {
			mPageId = pageId;
			mConnId = connId;
			mUrl = path;
		}

		private String getFilePath() {
			// file:///c:/path/some.htm => c:\\path\\some.htm
			String p = mUrl.substring(7);
			if (p.charAt(2) == ':')
				p = p.substring(1);
			p = p.replace("/", File.separator);
			return p;
		}
		
		@Override
		public void run() {
			String path = getFilePath();
			File f = new File(path);
			if (f.exists() && f.length() > 0) {
				// 资源类型
				int mime = NbkEvent.MIME_UNKNOWN;
				path = path.toLowerCase();
				if (path.endsWith(".htm") || path.endsWith(".html"))
					mime = NbkEvent.MIME_HTML;
				else if (path.endsWith(".css"))
					mime = NbkEvent.MIME_CSS;
				else if (path.endsWith(".jpg") || path.endsWith(".jpeg"))
					mime = NbkEvent.MIME_JPEG;
				else if (path.endsWith(".gif"))
					mime = NbkEvent.MIME_GIF;
				else if (path.endsWith(".png"))
					mime = NbkEvent.MIME_PNG;
				
				// 编码
				int encoding = NbkEvent.ENCODING_UNKNOWN;
				
				receiveHeader(this, mime, encoding, (int)f.length());
				
				// 读取数据
				try {
					FileInputStream fin = new FileInputStream(f);
					while (fin.available() > 0) {
						int len = (fin.available() > BUFFER_SIZE) ? BUFFER_SIZE : fin.available();
						byte[] b = new byte[len];
						fin.read(b, 0, len);
						receiveData(this, b);
					}
					fin.close();
				}
				catch (Exception e) {
				}
				finally {
					receiveComplete(this);
				}
			}
			else {
				receiveError(this, -1);
			}
		}
	};
	
	private class HttpGet extends LoadData implements Runnable {
		
		private String mReferer;
		
		private int mFollow;
		
		public HttpGet(int pageId, int connId, String url, String referer) {
			mPageId = pageId;
			mConnId = connId;
			mUrl = url;
			mReferer = referer;
			mFollow = 0;
		}
		
		private void getCookies(HttpURLConnection conn) {
			Map<String, List<String>> resp = conn.getHeaderFields();
			List<String> cookies = resp.get("Set-Cookie");
			if (cookies != null) {
				URL url = null;
				try {
					url = new URL(mUrl);
				}
				catch (Exception e) {
				}
				if (url != null) {
					mShell.getCookieMgr().addCookies(url.getHost(), cookies);
				}
			}
		}
		
		@Override
		public void run() {
			HttpURLConnection conn = null;
			try {
				int code = 0;
				while (mFollow < 5) {
					if (mFollow == 0)
						System.out.println("load -> " + mUrl);
					else
						System.out.println("redi -> " + mUrl);
					
					URL url = new URL(mUrl);
					conn = (HttpURLConnection) url.openConnection();
					conn.setInstanceFollowRedirects(false);
					conn.setRequestMethod("GET");
					
					conn.setRequestProperty("User-Agent", "NBK/2.0");
					if (mReferer != null)
						conn.setRequestProperty("Referer", mReferer);
					conn.setRequestProperty("Accept-Encoding", "gzip,deflate");
					
					String cookie = mShell.getCookieMgr().getCookie(url.getHost());
					if (cookie != null) {
						System.out.println("  [cookie] " + cookie);
						conn.setRequestProperty("Cookie", cookie);
					}
					
					conn.connect();
					
					code = conn.getResponseCode();
					if (code >= 300 && code < 400) {
						conn.disconnect();
						String loc = conn.getHeaderField("Location");
						if (loc != null) {
							getCookies(conn);
							mReferer = mUrl;
							mUrl = loc;
							mFollow++;
							continue;
						}
						conn = null;
					}
					break;
				}
				
				if (code == 200) {
					
					getCookies(conn);
					
					// 资源类型
					int mime = NbkEvent.MIME_UNKNOWN;
					String contentType = conn.getContentType();
					if (contentType != null) {
						contentType = contentType.toLowerCase();
						if (contentType.contains("xhtml"))
							mime = NbkEvent.MIME_XHTML;
						else if (contentType.contains("html"))
							mime = NbkEvent.MIME_HTML;
						else if (contentType.contains("wml"))
							mime = NbkEvent.MIME_WML;
						else if (contentType.contains("css"))
							mime = NbkEvent.MIME_CSS;
						else if (contentType.contains("jpeg"))
							mime = NbkEvent.MIME_JPEG;
						else if (contentType.contains("png"))
							mime = NbkEvent.MIME_PNG;
						else if (contentType.contains("gif"))
							mime = NbkEvent.MIME_GIF;
						else
							System.out.println("  Content-Type: " + contentType);
					}
					
					// 编码
					int encoding = NbkEvent.ENCODING_UNKNOWN;
					if (contentType != null) {
						if (contentType.contains("charset=utf-8"))
							encoding = NbkEvent.ENCODING_UTF8;
						else if (contentType.contains("charset=gb2312"))
							encoding = NbkEvent.ENCODING_GBK;
						else
							System.out.println("  Content-Type: " + contentType);
					}
					
					receiveHeader(this, mime, encoding, conn.getContentLength());
					
					InputStream in = null;
					String encode = conn.getContentEncoding();
					if (encode != null && encode.contains("gzip"))
						in = new GZIPInputStream(conn.getInputStream());
					else
						in = conn.getInputStream();
					
					if (in != null) {
						byte[] buf = new byte[BUFFER_SIZE];
						int len;
						int offs = 0;
						int leave = BUFFER_SIZE;
						do {
							if (leave == 0) {
								byte[] b = new byte[offs];
								System.arraycopy(buf, 0, b, 0, offs);
								receiveData(this, b);
								offs = 0;
								leave = BUFFER_SIZE;
							}
							len = in.read(buf, offs, leave);
							if (len > 0) {
								offs += len;
								leave -= len;
							}
						} while (len != -1);
						if (offs > 0) {
							byte[] b = new byte[offs];
							System.arraycopy(buf, 0, b, 0, offs);
							receiveData(this, b);
						}
						in.close();
					}
					
					receiveComplete(this);
				}
				else if (code == 404) {
					receiveError(this, 404);
				}
				else {
					receiveError(this, -1);
				}
			}
			catch (Exception e) {
				receiveError(this, -1);
			}
			finally {
				if (conn != null)
					conn.disconnect();
			}
		}
	};
	
	private void receiveHeader(LoadData d, int mime, int encoding, int length) {
		NbkEvent e = new NbkEvent(NbkEvent.EVENT_RECEIVE_HEADER);
		e.setPageId(d.getPageId());
		e.setId(d.getId());
		e.setUrl(d.getUrl());
		e.setMime(mime);
		e.setEncoding(encoding);
		e.setLength(length);
		mShell.postEventToCore(e);
	}
	
	private void receiveData(LoadData d, byte[] b) {
		NbkEvent e = new NbkEvent(NbkEvent.EVENT_RECEIVE_DATA);
		e.setPageId(d.getPageId());
		e.setId(d.getId());
		e.setUrl(d.getUrl());
		e.setData(b);
		mShell.postEventToCore(e);
	}
	
	private void receiveComplete(LoadData d) {
		NbkEvent e = new NbkEvent(NbkEvent.EVENT_RECEIVE_COMPLETE);
		e.setPageId(d.getPageId());
		e.setId(d.getId());
		e.setUrl(d.getUrl());
		mShell.postEventToCore(e);
	}
	
	private void receiveError(LoadData d, int error) {
		NbkEvent e = new NbkEvent(NbkEvent.EVENT_RECEIVE_ERROR);
		e.setPageId(d.getPageId());
		e.setId(d.getId());
		e.setUrl(d.getUrl());
		e.setError(error);
		mShell.postEventToCore(e);
	}
	
	public void onLoaderResponse(NbkEvent e) {
		
		switch (e.getType()) {
		case NbkEvent.EVENT_RECEIVE_HEADER:
			nbkLoaderResponse(e.getType(), e.getPageId(), e.getId(), e.getUrl(), e.getMime(), e.getEncoding(), e.getLength(), null);
			break;
			
		case NbkEvent.EVENT_RECEIVE_DATA:
			nbkLoaderResponse(e.getType(), e.getPageId(), e.getId(), e.getUrl(), 0, 0, 0, e.getData());
			break;
			
		case NbkEvent.EVENT_RECEIVE_COMPLETE:
			nbkLoaderResponse(e.getType(), e.getPageId(), e.getId(), e.getUrl(), 0, 0, 0, null);
			break;
			
		case NbkEvent.EVENT_RECEIVE_ERROR:
			nbkLoaderResponse(e.getType(), e.getPageId(), e.getId(), e.getUrl(), e.getError(), 0, 0, null);
			break;
		}
	}
	
	public native void register();
	
	private native void nbkLoaderResponse(int evtType, int pageId, int connId, String url, int v1, int v2, int v3, byte[] data);
}
