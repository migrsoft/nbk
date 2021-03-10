#pragma once

class NbkEvent {

private:
	static int _alloc;
	static int _free;

public:
	static void MemoryInfo();

public:
	enum EVENT_TYPE {
		EVENT_NULL,

		EVENT_INIT,
		EVENT_QUIT,
		EVENT_TIMER,

		EVENT_NBK,
		EVENT_NBK_CALL,

		EVENT_LOAD_URL,
		EVENT_LOAD_PREV_PAGE,
		EVENT_LOAD_NEXT_PAGE,

		EVENT_SCROLL_TO,
		EVENT_LEFT_BUTTON,

		EVENT_RECE_HEADER,
		EVENT_RECE_DATA,
		EVENT_RECE_COMPLETE,
		EVENT_RECE_ERROR,

		EVENT_DECODE_IMG,

		EVENT_NEW_DOC_BEGIN,
		EVENT_UPDATE,

		EVENT_PAGE_SIZE,

		EVENT_LAST
	};

	enum BUTTON_STATE {
		STAT_NONE,
		STAT_DOWN,
		STAT_MOVE,
		STAT_UP,
		STAT_LAST
	};

private:
	int		mId;
	int		mId2;

	int		mV1;
	int		mV2;
	int		mV3;

	char*	mData;
	int		mDataLen;

	char*	mUrl;
	bool	mDelUrl;

	void*	mFunc;
	void*	mUser;

public:
	NbkEvent(int id);
	~NbkEvent();

	int GetId() { return mId; }
	
	void SetTimerId(int id) { mV1 = id; }
	int GetTimerId() { return mV1; }

	void SetConnId(int id) { mId2 = id; }
	int GetConnId() { return mId2; }

	void SetMime(int mime) { mV1 = mime; }
	int GetMime() { return mV1; }

	void SetEncoding(int encoding) { mV2 = encoding; }
	int GetEncoding() { return mV2; }

	void SetLength(int len) { mV3 = len; }
	int GetLength() { return mV3; }

	void SetData(char* data, int len) { mData = data; mDataLen = len; }
	char* GetData() { return mData; }
	int GetDataLen() { return mDataLen; }

	void SetError(int error) { mV1 = error; }
	int GetError() { return mV1; }

	void SetUrl(const char* url, bool del);
	char* GetUrl() { return mUrl; }

	void SetCallback(int pageId, void* func, void* user);
	void GetCallback(int* pageId, void** func, void** user);

	void SetUser(void* user) { mUser = user; }
	void* GetUser() { return mUser; }

	void SetPageSize(int w, int h);
	int GetPageWidth() { return mV1; }
	int GetPageHeight() { return mV2; }

	void SetX(int x) { mV1 = x; }
	int GetX() { return mV1; }

	void SetY(int y) { mV2 = y; }
	int GetY() { return mV2; }

	void SetState(int stat) { mV3 = stat; }
	int GetState() { return mV3; }

private:
	void ReleaseUrl();
};
