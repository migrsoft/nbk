/*
 * document.h
 *
 *  Created on: 2010-12-25
 *      Author: wuyulun
 */

#ifndef DOCUMENT_H_
#define DOCUMENT_H_

#include "../inc/config.h"
#include "../inc/nbk_public.h"
#include "../inc/nbk_limit.h"
#include "xml_tokenizer.h"
#include "node.h"
#include "view.h"
#include "xpath.h"
#include "event.h"
#include "wbxmlDec.h"
#include "../tools/strPool.h"
#include "../tools/strBuf.h"
#include "../loader/upCmd.h"
#include "../loader/loader.h"
#include "../css/cssMgr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _NEDocType {
    
    NEDOC_UNKNOWN,
    //---
    NEDOC_MAINBODY,
    //---
    NEDOC_WML,
    NEDOC_XHTML,
    NEDOC_HTML,
    //---
    NEDOC_DXXML,
    NEDOC_FULL,
    NEDOC_FULL_XHTML,
    NEDOC_NARROW,
    //---
    NEDOC_LAST
    
} NEDocType;

typedef enum _NERedirectType {
    
     NEREDIR_NONE,
     NEREDIR_WML_ONTIMER,
     NEREDIR_WML_ONEVENT,
     NEREDIR_WML_ONEVENTFORWARD,
     NEREDIR_META_REFRESH
     
} NERedirectType;

// DOM document

typedef struct _NDocument {

	NSmartPtr	_smart;

	void*       page; // not own, type is NPage
    NNode*      root;
    
    NXmlTokenizer*   tokenizer;
    
    // for build tree
    struct {
        NNode** dat;
        int16   top;
    } stack;
    NNode*  lastChild;
    
    nbool    finish : 1; // parse over
    nbool    layoutFinish : 1;
    nbool    inTitle : 1;
    nbool    inStyle : 1;
    nbool    inScript : 1;
    nbool    inCtrl : 1;
    nbool    inTable : 1;
    nbool    firstShow : 1; // is the first screen displayed
    nbool    picUpdate : 1;
    nbool    hasSpace : 1;
    nbool    stop : 1;
    nbool    ownStrPool : 1;
    nbool    disable : 1; // disable display
    
    NStrPool*   spool; // for attributes
    int         words;
    int         wordProcessed;
    int         wordCount;
    
    char*       url;
    int16       urlMaxLen;
    
    nid         type; // NEDocType
    
    wchr*       title;

	int         delayLoad; // for redirect
    nid         delayLoadType; // NERedirectType
    
    nid*        fix;
	nid			exceptNode;
    
    // for decoding
    nid         encoding;
    uint8       gbRest; // the second char in a GB
    
    NExtCssMgr* extCssMgr;
    
    NWbxmlDecoder*  wbxmlDecoder;
    
	// 测试项目
    int         __n_no;
	uint32		__time_start;
    
} NDocument;

typedef struct _NLoadCmd {

	NSmartPtr	_smart;
    
    NDocument*  doc;
    char*       url;
    char*       body;
    nid         enc; // body encode type
    
    nid         via;
    
    NFileUpload*    files;
    
    nbool        noDeal : 1;
    nbool        submit : 1;
    
} NLoadCmd;

#ifdef NBK_MEM_TEST
int doc_memUsed(const NDocument* doc);
#endif

NDocument* doc_create(void* page);
void doc_delete(NDocument** doc);

void doc_attachWbxmlDecoder(NDocument* doc, int length);
void doc_detachWbxmlDecoder(NDocument* doc);

void doc_begin(NDocument* doc);
void doc_clear(NDocument* doc);
void doc_end(NDocument* doc);
void doc_stop(NDocument* doc);

void doc_handleBySelf(NDocument* doc);

void doc_setUrl(NDocument* doc, const char* url);
void doc_write(NDocument* doc, const char* str, int length, nbool final);
void doc_processToken(NDocument* doc, NToken* token);
void doc_acceptType(NDocument* doc);

char* doc_getUrl(NDocument* doc);
wchr* doc_getTitle(NDocument* doc);
nid doc_getType(NDocument* doc);

char* doc_getFrameName(NDocument* doc);

NNode* doc_getFocusNode(NDocument* doc, const NRect* area);
NNode* doc_getFocusNodeByPos(NDocument* doc, coord x, coord y);
NNode* doc_getImageNodeByPos(NDocument* doc, coord x, coord y);
char* doc_getFocusUrl(NDocument* doc, NNode* focus);
NNode* doc_getPrevFocusNode(NDocument* doc, NNode* curr);
NNode* doc_getNextFocusNode(NDocument* doc, NNode* curr);
void doc_getFocusNodeRect(NDocument* doc, NNode* focus, NRect* rect);
void doc_clickFocusNode(NDocument* doc, NNode* focus);

// network load
void doc_doGet(NDocument* doc, const char* url, nbool submit, nbool noDeal);
void doc_doPost(NDocument* doc, const char* url, nid enc, char* body, NFileUpload* files);

// local load
void doc_doBack(NDocument* doc);
void doc_jumpTo(NDocument* doc, const char* name);

void doc_scheduleLayout(NDocument* doc);

void doc_viewChange(NDocument* doc);
void doc_modeChange(NDocument* doc, NEDocType type);

NNode* dom_query(NDocument* doc, NXPathCell* xpath, NNode** sibling, NNode** parent, nbool ignoreTag);

nbool doc_processKey(NEvent* event);
nbool doc_processPen(NEvent* event);

void doc_dumpDOM(NDocument* doc);
void doc_outputXml(NDocument* doc);

NStrPool* doc_getStrPool(NDocument* doc);
void doc_setStrPool(NDocument* doc, NStrPool* spool, nbool own);

NLoadCmd* loadCmd_create(void);
void loadCmd_delete(NLoadCmd** cmd);

#ifdef __cplusplus
}
#endif

#endif /* DOCUMENT_H_ */
