/*
 * loader.h
 *
 *  Created on: 2011-2-3
 *      Author: wuyulun
 */

#ifndef LOADER_H_
#define LOADER_H_

#include "../inc/config.h"
#include "upCmd.h"

#ifdef __cplusplus
extern "C" {
#endif
    
enum NEStockPage {
    
    NESPAGE_ERROR,
    NESPAGE_ERROR_404
};

enum NEMimeType {
    
    NEMIME_UNKNOWN,
    
    NEMIME_DOC_HTML,
    NEMIME_DOC_XHTML,
    NEMIME_DOC_WML,
    NEMIME_DOC_WMLC,
    NEMIME_DOC_CSS,
    NEMIME_DOC_SCRIPT,
    
    NEMIME_IMAGE_GIF,
    NEMIME_IMAGE_JPEG,
    NEMIME_IMAGE_PNG,

    NEMIME_FF_XML,
    NEMIME_FF_IMAGE,
    NEMIME_FF_BG_IMAGE,
    NEMIME_FF_INC_DATA,
    
    NEMIME_LAST
};

enum NEEncoding {
    
    NEENC_UNKNOWN,
    NEENC_UTF8,
    NEENC_GB2312,
    NEENC_LAST
};

enum NEEncType {
    
    NEENCT_URLENCODED,
    NEENCT_MULTIPART,
    NEENCT_LAST
};

enum NERequestType {
    
    NEREQT_NONE,
    NEREQT_MAINDOC,
    NEREQT_SUBDOC,
    NEREQT_CSS,
    NEREQT_JS,
    NEREQT_IMAGE,
	NEREQT_LAST
};

enum NERequestVia {
    
    NEREV_STANDARD,
    NEREV_TF,
    NEREV_LAST
};

enum NERequestMethod {
    
    NEREM_NORMAL,
    NEREM_REFRESH,
    NEREM_HISTORY,
    NEREM_JUST_CACHE
};

typedef enum _NEResponseType {

    NEREST_NONE,

    NEREST_HEADER,
    NEREST_DATA,
    NEREST_COMPLETE,
    NEREST_ERROR,
    NEREST_CANCEL,

    NEREST_LAST
    
} NEResponseType;

enum NELoadError {
    
    NELERR_ERROR = -1,
    NELERR_NOCACHE = -2,
    NELERR_NBK_NOT_SUPPORT = -4
};

typedef struct _NFileUpload {
    
    char*   name;
    char*   path;
    
} NFileUpload;

typedef struct _NRequest {

	NSmartPtr	_smart;
    
    uint8   type;
    uint8   via;
    uint8   method;
    uint8   enc;
    uint8   ownUrl : 1;
    uint8   ownUser : 1;
    uint8   ownReferer : 1;
    uint8   noDeal : 1; // this document hasn't be processed by previous request

    nid     pageId;
    char*   url;
    char*   referer;
    char*   body;
    void*   user;
    void*   platform;
    NUpCmd* upcmd;
    
    NFileUpload*    files;
    
    void(*responseCallback)(NEResponseType type, void* user, void* data, int length, int comprLen);
    
} NRequest;

typedef struct _NRespHeader {
    
	nid		mime;
	nid		encoding;
	int		content_length;
	void*	user;

} NRespHeader;

NRequest* loader_createRequest(void);
void loader_deleteRequest(NRequest** req);
void loader_setRequest(NRequest* req, uint8 type, char* url, void* user);
void loader_setRequestUrl(NRequest* req, char* url, nbool own);
void loader_setRequestUser(NRequest* req, void* user);
void loader_setRequestReferer(NRequest* req, char* referer, nbool own);

nbool loader_load(NRequest* request);
void loader_stopAll(void* pfd, nid pageId);

// 标准请求回应
void loader_onReceiveHeader(NRequest* req, NRespHeader* header);
void loader_onReceiveData(NRequest* req, char* data, int length, int comprLen);
void loader_onComplete(NRequest* req);
void loader_onCancel(NRequest* req);
void loader_onError(NRequest* req, int error);

// implemented by platform
nbool NBK_handleError(int error);
char* NBK_getStockPage(int type);
nbool NBK_resourceLoad(void* pfd, NRequest* req);
void NBK_resourceStopAll(void* pfd, nid pageId);

#ifdef __cplusplus
}
#endif

#endif /* LOADER_H_ */
