package com.migrsoft.nbk;

public class NbkEvent {

	// 事件定义
	
	public static final int EVENT_NONE = 0;
	public static final int EVENT_INIT = 1;
	public static final int EVENT_QUIT = 2;
	
	public static final int EVENT_NBK_EVENT = 10;
	public static final int EVENT_NBK_CALL = 11;
	
	public static final int EVENT_LOAD_URL = 50;
	public static final int EVENT_SCROLL = 51;
	
	public static final int EVENT_TIMER = 100;
	public static final int EVENT_KEY = 110;
	public static final int EVENT_MOUSE_LEFT = 111;
	
	public static final int EVENT_UPDATE_VIEW = 200;
	
	public static final int EVENT_RECEIVE_HEADER = 300;
	public static final int EVENT_RECEIVE_DATA = 301;
	public static final int EVENT_RECEIVE_COMPLETE = 302;
	public static final int EVENT_RECEIVE_ERROR = 303;
	
	public static final int EVENT_IMAGE_DECODE = 310;
	
	// 文件类型定义
	
	public static final int MIME_UNKNOWN = 0;
	public static final int MIME_HTML = 1;
	public static final int MIME_XHTML = 2;
	public static final int MIME_WML = 3;
	public static final int MIME_WMLC = 4;
	public static final int MIME_CSS = 5;
	public static final int MIME_SCRIPT = 6;
	public static final int MIME_JPEG = 10;
	public static final int MIME_GIF = 11;
	public static final int MIME_PNG = 12;
	
	// 文件编码定义
	
	public static final int ENCODING_UNKNOWN = 0;
	public static final int ENCODING_UTF8 = 1;
	public static final int ENCODING_GBK = 2;
	
	// 指针行为定义
	
	public static final int MOUSE_PRESS = 1;
	public static final int MOUSE_MOVE = 2;
	public static final int MOUSE_RELEASE = 3;
	
	//////////////////////////////////////////////////
	
	private int mType;
	private int mSubType;
	
	private int mPageId;
	private String mUrl;
	
	private int mMime;
	private int mEncoding;
	private int mLength;
	
	private byte[] mData;
	
	private int mX;
	private int mY;
	private int mWidth;
	private int mHeight;
	
	private boolean mResult;
	
	public NbkEvent(int type) {
		mType = type;
		
		mPageId = 0;
		mMime = MIME_UNKNOWN;
		mEncoding = ENCODING_UNKNOWN;
	}
	
	public int getType() {
		return mType;
	}
	
	public int getSubType() {
		return mSubType;
	}

	public int getPageId() {
		return mPageId;
	}
	
	public void setPageId(int id) {
		mPageId = id;
	}
	
	public String getUrl() {
		return mUrl;
	}
	
	public void setUrl(String url) {
		mUrl = url;
	}
	
	public int getMime() {
		return mMime;
	}
	
	public void setMime(int mime) {
		mMime = mime;
	}
	
	public int getEncoding() {
		return mEncoding;
	}
	
	public void setEncoding(int encoding) {
		mEncoding = encoding;
	}
	
	public int getLength() {
		return mLength;
	}
	
	public void setLength(int length) {
		mLength = length;
	}
	
	public byte[] getData() {
		return mData;
	}
	
	public void setData(byte[] data) {
		mData = data;
	}
	
	public void setClick(int type, int x, int y) {
		mSubType = type;
		mX = x;
		mY = y;
	}
	
	public void setPos(int x, int y) {
		mX = x;
		mY = y;
	}
	
	public int getX() {
		return mX;
	}
	
	public int getY() {
		return mY;
	}
	
	public void setSize(int w, int h) {
		mWidth = w;
		mHeight = h;
	}
	
	public int getWidth() {
		return mWidth;
	}
	
	public int getHeight() {
		return mHeight;
	}
	
	public void setError(int err) {
		mSubType = err;
	}
	
	public int getError() {
		return mSubType;
	}
	
	public void setId(int id) {
		mSubType = id;
	}
	
	public int getId() {
		return mSubType;
	}
	
	public void setResult(boolean ok) {
		mResult = ok;
	}
	
	public boolean getResult() {
		return mResult;
	}
}
