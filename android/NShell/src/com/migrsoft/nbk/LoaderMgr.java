package com.migrsoft.nbk;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URI;
import java.net.URL;
import java.security.MessageDigest;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.zip.GZIPInputStream;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.AssetManager;
import android.util.Log;

public class LoaderMgr {
	
	private static final String TAG = "NbkLoaderMgr";
	
	private static final String ASSET_PREFIX = "file://assets/";
	
	@SuppressLint("SdCardPath")
	private static final String HISTORY_DIR_FORMAT = "/data/data/com.wyl.nshell/history/n%04d/";
	
	private final int BUFFER_SIZE = 16384;
	
	private INbkGlue mGlue;
	private Context mContext;
	
	private ExecutorService mPool;
	
	private CookieMgr mCookieMgr;
	
	public LoaderMgr(INbkGlue glue, Context ctx) {
		mGlue = glue;
		mContext = ctx;
		mPool = Executors.newFixedThreadPool(2);
		mCookieMgr = new CookieMgr();
	}
	
	// 资源请求发起入口
	
	public void loadFile(int pageId, int connId, String path) {
		LoadFile r;
		if (path.startsWith(ASSET_PREFIX)) {
			r = new LoadFileFromAsset(pageId, connId, path);
		}
		else {
			r = new LoadFile(pageId, connId, path);
		}
		mPool.submit(r);
	}
	
	public void loadHistory(int pageId, int connId, String url) {
		LoadFile r;
		if (url.startsWith(ASSET_PREFIX)) {
			r = new LoadFileFromAsset(pageId, connId, url);
		}
		else {
			r = new LoadHistory(pageId, connId, url);
		}
		mPool.submit(r);
	}
	
	public void httpGet(int pageId, int connId, String url, String referer) {
		HttpGet r = new HttpGet(pageId, connId, url, referer);
		mPool.submit(r);
	}
	
	/////
	
	private boolean isAssets(String path) {
		String prefix = "file://" + ASSET_PREFIX;
		return path.startsWith(prefix);
	}
	
	public void deleteHistoryDir(int pageId) {
		String path = String.format(HISTORY_DIR_FORMAT, pageId);
		deleteDirectory(path);
	}
	
	public void deleteDirectory(String dir) {
		File f = new File(dir);
		if (!f.exists())
			return;
		
		File[] lst = f.listFiles();
		if (lst != null) {
			for (int i=0; i < lst.length; i++) {
				lst[i].delete();
			}
		}
		
		f.delete();
	}
	
	public static final String getMd5(String s) {
		char hexDigits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
		char str[];
		byte strTemp[] = s.getBytes();
		try {
			MessageDigest mdTemp = MessageDigest.getInstance("MD5");
			mdTemp.update(strTemp);
			byte md[] = mdTemp.digest();
			int j = md.length;
			str = new char[j * 2];
			int k = 0;
			for (int i = 0; i < j; i++) {
				byte byte0 = md[i];
				str[k++] = hexDigits[byte0 >>> 4 & 0xf];
				str[k++] = hexDigits[byte0 & 0xf];
			}
			return new String(str);
		} catch (Exception e) {
		}
		return null;
	}
	
	private class LoadData {
		protected int mPageId;
		protected int mConnId;
		protected String mUrl;
		protected String mUrlMd5;
		
		public int getPageId() {
			return mPageId;
		}
		
		public int getId() {
			return mConnId;
		}
		
		public String getUrl() {
			return mUrl;
		}
		
		public String getUrlMd5() {
			return mUrlMd5;
		}
	}

	private class LoadFile extends LoadData implements Runnable {

		public LoadFile(int pageId, int connId, String path) {
			mPageId = pageId;
			mConnId = connId;
			mUrl = path;
		}

		protected String getFilePath() {
			// file:///c:/path/some.htm => c:\\path\\some.htm
			String p = mUrl.substring(7);
			if (p.charAt(2) == ':')
				p = p.substring(1);
			p = p.replace("/", File.separator);
			return p;
		}
		
		public int decideMimeByExtName(String name) {
			int mime = NbkEvent.MIME_UNKNOWN;
			String path = name.toLowerCase();
			if (path.endsWith(".htm") || path.endsWith(".html"))
				mime = NbkEvent.MIME_HTML;
			else if (path.endsWith(".xhtm"))
				mime = NbkEvent.MIME_XHTML;
			else if (path.endsWith(".wml"))
				mime = NbkEvent.MIME_WML;
			else if (path.endsWith(".wmc"))
				mime = NbkEvent.MIME_WMLC;
			else if (path.endsWith(".css"))
				mime = NbkEvent.MIME_CSS;
			else if (path.endsWith(".jpg") || path.endsWith(".jpeg"))
				mime = NbkEvent.MIME_JPEG;
			else if (path.endsWith(".gif"))
				mime = NbkEvent.MIME_GIF;
			else if (path.endsWith(".png"))
				mime = NbkEvent.MIME_PNG;
			return mime;
		}
		
		@Override
		public void run() {
			String path = getFilePath();
			Log.i(TAG, "load -> " + path);
			if (path == null) {
				receiveError(this, 404);
				return;
			}
			
			File f = new File(path);
			if (f.exists() && f.length() > 0) {
				// 资源类型
				int mime = decideMimeByExtName(path);
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
					receiveComplete(this);
				}
				catch (Exception e) {
					receiveError(this, -1);
				}
			}
			else {
				receiveError(this, -1);
			}
		}
	};
	
	private class LoadFileFromAsset extends LoadFile {
		
		public LoadFileFromAsset(int pageId, int connId, String path) {
			super(pageId, connId, path);
		}

		@Override
		public void run() {
			String path = mUrl.substring(ASSET_PREFIX.length());
			Log.i(TAG, "asset -> " + path);
			
			AssetManager am = mContext.getAssets();
			try {
				InputStream is = am.open(path);
				
				int mime = decideMimeByExtName(path);
				receiveHeader(this, mime, NbkEvent.ENCODING_UNKNOWN, is.available());
				
				while (is.available() > 0) {
					int len = (is.available() > BUFFER_SIZE) ? BUFFER_SIZE : is.available();
					byte b[] = new byte[len];
					is.read(b, 0, len);
					receiveData(this, b);
				}
				is.close();
				receiveComplete(this);
			}
			catch (IOException e) {
				receiveError(this, -1);
			}
		}
	}
	
	private class LoadHistory extends LoadFile {

		public LoadHistory(int pageId, int connId, String url) {
			super(pageId, connId, url);
			mUrlMd5 = getMd5(url);
		}
		
		protected String getFilePath() {
			String path = String.format(HISTORY_DIR_FORMAT, getPageId());
			File f = new File(path);
			String[] lst = f.list();
			if (lst != null) {
				for (int i = 0; i < lst.length; i++) {
					if (lst[i].startsWith(getUrlMd5()))
						return path + lst[i];
				}
			}
			return null;
		}
	}
	
	private class HttpGet extends LoadData implements Runnable {
		
		private final static int CONNECT_TIMEOUT = 10 * 1000;
		private final static int READ_TIMEOUT = 10 * 1000;
		
		private String mReferer;
		
		private int mFollow;
		
		FileOutputStream mFout;
		
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
					mCookieMgr.addCookies(url.getHost(), cookies);
				}
			}
		}
		
		private void createHistoryFile(int mime) {
			mUrlMd5 = getMd5(mUrl);
			String path = String.format(HISTORY_DIR_FORMAT, getPageId());
			String name;
			if (mime == NbkEvent.MIME_XHTML)
				name = path + getUrlMd5() + ".xhtm";
			else if (mime == NbkEvent.MIME_HTML)
				name = path + getUrlMd5() + ".htm";
			else if (mime == NbkEvent.MIME_WMLC)
				name = path + getUrlMd5() + ".wmc";
			else if (mime == NbkEvent.MIME_WML)
				name = path + getUrlMd5() + ".wml";
			else if (mime == NbkEvent.MIME_CSS)
				name = path + getUrlMd5() + ".css";
			else if (mime == NbkEvent.MIME_GIF)
				name = path + getUrlMd5() + ".gif";
			else if (mime == NbkEvent.MIME_JPEG)
				name = path + getUrlMd5() + ".jpg";
			else if (mime == NbkEvent.MIME_PNG)
				name = path + getUrlMd5() + ".png";
			else
				return;
			
			File f = new File(path);
			if (!f.exists())
				f.mkdirs();
			f = new File(name);
			try {
				mFout = new FileOutputStream(f);
			} catch (Exception e) {
			}
		}
		
		private void writeHistoryData(byte[] data, boolean over) {
			if (mFout != null) {
				try {
					if (data != null)
						mFout.write(data);
					if (over)
						mFout.close();
				} catch (Exception e) {
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
						Log.i(TAG, "load -> " + mUrl);
					else
						Log.i(TAG, "redi -> " + mUrl);
					
					URL url = new URL(mUrl);
					conn = (HttpURLConnection) url.openConnection();
					conn.setInstanceFollowRedirects(false);
					conn.setConnectTimeout(CONNECT_TIMEOUT);
					conn.setReadTimeout(READ_TIMEOUT);
					conn.setRequestMethod("GET");
					
					conn.setRequestProperty("User-Agent", "Series60/3.2; Nokia; NBK/2.0");
					if (mReferer != null)
						conn.setRequestProperty("Referer", mReferer);
					conn.setRequestProperty("Accept-Encoding", "gzip,deflate");
					
					String cookie = mCookieMgr.getCookie(url.getHost());
					if (cookie != null) {
//						Log.i(TAG, "  [cookie] " + cookie);
						conn.setRequestProperty("Cookie", cookie);
					}
					
					conn.connect();
					
					code = conn.getResponseCode();
					if (code >= 300 && code < 400) {
						conn.disconnect();
						String loc = conn.getHeaderField("Location");
						if (loc != null) {
//							Log.i(TAG, "  [Location] " + loc);
							getCookies(conn);
							mReferer = mUrl;
							URI locUri = new URI(loc);
							if (!locUri.isAbsolute()) {
								URI base = new URI(mUrl);
								URI resolved = base.resolve(locUri);
								loc = resolved.toString();
							}
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
						else if (contentType.contains("wmlc"))
							mime = NbkEvent.MIME_WMLC;
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
							Log.i(TAG, "  content-type: " + contentType);
					}
					
					// 编码
					int encoding = NbkEvent.ENCODING_UNKNOWN;
					if (contentType != null) {
						if (contentType.contains("charset=utf-8"))
							encoding = NbkEvent.ENCODING_UTF8;
						else if (contentType.contains("charset=gb2312"))
							encoding = NbkEvent.ENCODING_GBK;
						else
							Log.i(TAG, "  no encoding in content-type: " + contentType);
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
						
						createHistoryFile(mime);

						do {
							if (leave == 0) {
								byte[] b = new byte[offs];
								System.arraycopy(buf, 0, b, 0, offs);
								receiveData(this, b);
								writeHistoryData(b, false);
								offs = 0;
								leave = BUFFER_SIZE;
							}
							len = in.read(buf, offs, leave);
//							Log.i(TAG, "  rece - " + len + " " + buf[0]);
							if (len > 0) {
								offs += len;
								leave -= len;
							}
						} while (len != -1);
						if (offs > 0) {
							byte[] b = new byte[offs];
							System.arraycopy(buf, 0, b, 0, offs);
							receiveData(this, b);
							writeHistoryData(b, false);
						}
						in.close();
						
						writeHistoryData(null, true);
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
		mGlue.sendNbkEventToCore(e);
	}
	
	private void receiveData(LoadData d, byte[] b) {
		NbkEvent e = new NbkEvent(NbkEvent.EVENT_RECEIVE_DATA);
		e.setPageId(d.getPageId());
		e.setId(d.getId());
		e.setUrl(d.getUrl());
		e.setData(b);
		mGlue.sendNbkEventToCore(e);
	}
	
	private void receiveComplete(LoadData d) {
		NbkEvent e = new NbkEvent(NbkEvent.EVENT_RECEIVE_COMPLETE);
		e.setPageId(d.getPageId());
		e.setId(d.getId());
		e.setUrl(d.getUrl());
		mGlue.sendNbkEventToCore(e);
	}
	
	private void receiveError(LoadData d, int error) {
		NbkEvent e = new NbkEvent(NbkEvent.EVENT_RECEIVE_ERROR);
		e.setPageId(d.getPageId());
		e.setId(d.getId());
		e.setUrl(d.getUrl());
		e.setError(error);
		mGlue.sendNbkEventToCore(e);
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
