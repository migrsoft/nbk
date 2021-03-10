#include "LoaderMgr.h"
#include "NbkDll.h"
#include "com_migrsoft_nbk_LoaderMgr.h"
#include "../../stdc/dom/history.h"
#include "../../stdc/tools/str.h"

////////////////////////////////////////////////////////////////////////////////
//
// 来自NBK的资源请求
//

nbool NBK_handleError(int error)
{
	return N_FALSE;
}

char* NBK_getStockPage(int type)
{
	return N_NULL;
}

nbool NBK_resourceLoad(void* pfd, NRequest* req)
{
	NbkCore* core = (NbkCore*)pfd;
	return core->GetLoader()->Load(req);
}

void NBK_resourceStopAll(void* pfd, nid pageId)
{
	NbkCore* core = (NbkCore*)pfd;
	return core->GetLoader()->RemoveAllRequests(pageId);
}

void NBK_resourceClean(void* pfd, nid pageId)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetLoader()->ResourceClean(pageId);
}

////////////////////////////////////////////////////////////////////////////////
//
// 来自JAVA的资源响应
//

JNIEXPORT void JNICALL Java_com_migrsoft_nbk_LoaderMgr_register
  (JNIEnv* env, jobject obj)
{
	LoaderMgr* mgr = NbkCore::GetInstance()->GetLoader();

	mgr->JNI_env = env;
	mgr->OBJ_LoaderMgr = env->NewGlobalRef(obj);

	jclass clazz = env->GetObjectClass(obj);
	mgr->MID_loadFile = env->GetMethodID(clazz, "loadFile", "(IILjava/lang/String;)V");
	mgr->MID_loadHistory = env->GetMethodID(clazz, "loadHistory", "(IILjava/lang/String;)V");
	mgr->MID_httpGet = env->GetMethodID(clazz, "httpGet", "(IILjava/lang/String;Ljava/lang/String;)V");
	mgr->MID_deleteHistoryDir = env->GetMethodID(clazz, "deleteHistoryDir", "(I)V");
}

static const int RECEIVE_HEADER = 300;
static const int RECEIVE_DATA = 301;
static const int RECEIVE_COMPLETE = 302;
static const int RECEIVE_ERROR = 303;

static const int MIME_JAVA[] = {
	0,	// MIME_UNKNOWN
	1,	// MIME_HTML
	2,	// MIME_XHTML
	3,	// MIME_WML
	4,	// MIME_WMLC
	5,	// MIME_CSS
	6,	// MIME_SCRIPT
	10,	// MIME_JPEG
	11,	// MIME_GIF
	12,	// MIME_PNG
	0xffff
};

static const nid MIME_NBK[] = {
	NEMIME_UNKNOWN,
	NEMIME_DOC_HTML,
	NEMIME_DOC_XHTML,
	NEMIME_DOC_WML,
	NEMIME_DOC_WMLC,
	NEMIME_DOC_CSS,
	NEMIME_DOC_SCRIPT,
	NEMIME_IMAGE_JPEG,
	NEMIME_IMAGE_GIF,
	NEMIME_IMAGE_PNG
};

static const int ENCODING_JAVA[] = {
	0,	// ENCODING_UNKNOWN
	1,	// ENCODING_UTF8
	2,	// ENCODING_GBK
	0xffff
};

static const nid ENCODING_NBK[] = {
	NEENC_UNKNOWN,
	NEENC_UTF8,
	NEENC_GB2312
};

JNIEXPORT void JNICALL Java_com_migrsoft_nbk_LoaderMgr_nbkLoaderResponse
  (JNIEnv* env, jobject obj, jint evtType, jint pageId, jint connId, jstring url, jint v1, jint v2, jint v3, jbyteArray data)
{
	bool delReq = false;
	LoaderMgr* mgr = NbkCore::GetInstance()->GetLoader();

	const char* u = env->GetStringUTFChars(url, NULL);
	NRequest* req = mgr->FindRequest((nid)pageId, connId, u);
	env->ReleaseStringUTFChars(url, u);

	if (req == N_NULL)
		return;

	switch (evtType) {
	case RECEIVE_HEADER:
		NRespHeader hdr;
		NBK_memset(&hdr, 0, sizeof(NRespHeader));
		for (int i = 0; MIME_JAVA[i] != 0xffff; i++) {
			if (MIME_JAVA[i] == v1) {
				hdr.mime = MIME_NBK[i];
				break;
			}
		}
		for (int i = 0; ENCODING_JAVA[i] != 0xffff; i++) {
			if (ENCODING_JAVA[i] == v2) {
				hdr.encoding = ENCODING_NBK[i];
				break;
			}
		}
		hdr.content_length = (int)v3;
		loader_onReceiveHeader(req, &hdr);
		break;

	case RECEIVE_DATA:
	{
		int len = env->GetArrayLength(data);
		jbyte* d = env->GetByteArrayElements(data, NULL);
		loader_onReceiveData(req, (char*)d, len, len);
		env->ReleaseByteArrayElements(data, d, 0);
		break;
	}

	case RECEIVE_COMPLETE:
		loader_onComplete(req);
		delReq = true;
		break;

	case RECEIVE_ERROR:
		loader_onError(req, (int)v1);
		delReq = true;
		break;
	}

	if (delReq) {
		mgr->RemoveRequest(req);
		smartptr_release(req);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
// LoaderMgr 接口类
//

LoaderMgr::LoaderMgr()
{
	OBJ_LoaderMgr = NULL;

	mCounter = 1;
	NBK_memset(&mConns, 0, sizeof(Conn) * MAX_REQUEST);
}

LoaderMgr::~LoaderMgr()
{
	if (OBJ_LoaderMgr) {
		JNI_env->DeleteGlobalRef(OBJ_LoaderMgr);
		OBJ_LoaderMgr = NULL;
	}

	RemoveAllRequests(0);
}

nbool LoaderMgr::Load(NRequest* request)
{
	int connId = AddRequest(request);
	if (connId == 0)
		return N_FALSE;

	if (request->method == NEREM_HISTORY) {
		jstring url = JNI_env->NewStringUTF(request->url);
		JNI_env->CallVoidMethod(OBJ_LoaderMgr, MID_loadHistory, (jint)request->pageId, connId, url);
		JNI_env->DeleteLocalRef(url);
		return N_TRUE;
	}
	else if (nbk_strncmp(request->url, "file://", 7) == 0) {
		jstring url = JNI_env->NewStringUTF(request->url);
		JNI_env->CallVoidMethod(OBJ_LoaderMgr, MID_loadFile, (jint)request->pageId, connId, url);
		JNI_env->DeleteLocalRef(url);
		return N_TRUE;
	}
	else if (nbk_strncmp(request->url, "http://", 7) == 0) {
		jstring url = JNI_env->NewStringUTF(request->url);
		jstring referer = NULL;
		if (request->referer)
			referer = JNI_env->NewStringUTF(request->referer);
		JNI_env->CallVoidMethod(OBJ_LoaderMgr, MID_httpGet, (jint)request->pageId, connId, url, referer);
		JNI_env->DeleteLocalRef(url);
		if (referer)
			JNI_env->DeleteLocalRef(referer);
		return N_TRUE;
	}
	else {
		RemoveRequest(request);
	}

	return N_FALSE;
}

int LoaderMgr::AddRequest(NRequest* request)
{
	for (int i=0; i < MAX_REQUEST; i++) {
		if (mConns[i].id == 0) {
			mConns[i].id = mCounter++;
			mConns[i].req = request;
			return mConns[i].id;
		}
	}
	return 0;
}

void LoaderMgr::RemoveRequest(NRequest* request)
{
	for (int i=0; i < MAX_REQUEST; i++) {
		if (mConns[i].req == request) {
			mConns[i].id = 0;
			mConns[i].req = N_NULL;
			break;
		}
	}
}

NRequest* LoaderMgr::FindRequest(nid pageId, int connId, const char* url)
{
	for (int i=0; i < MAX_REQUEST; i++) {
		if (mConns[i].id == connId) {
			if (nbk_strcmp(mConns[i].req->url, url)) {
				// 因重定向造成地址改变时，更新地址
				char* u = (char*)NBK_malloc(nbk_strlen(url) + 1);
				nbk_strcpy(u, url);
				loader_setRequestUrl(mConns[i].req, u, N_TRUE);
			}
			return mConns[i].req;
		}
	}
	return N_NULL;
}

void LoaderMgr::RemoveAllRequests(nid pageId)
{
	for (int i=0; i < MAX_REQUEST; i++) {
		if (mConns[i].id && (pageId == 0 || mConns[i].req->pageId == pageId)) {
			smartptr_release(mConns[i].req);
			mConns[i].id = 0;
			mConns[i].req = N_NULL;
		}
	}
}

void LoaderMgr::ResourceClean(nid pageId)
{
	JNI_env->CallVoidMethod(OBJ_LoaderMgr, MID_deleteHistoryDir, (int)pageId);
}
