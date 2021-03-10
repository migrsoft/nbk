/*
 * page.h
 *
 *  Created on: 2010-12-25
 *      Author: wuyulun
 */

#ifndef __NBK_PAGE_H__
#define __NBK_PAGE_H__

#include "../inc/config.h"
#include "../inc/nbk_settings.h"
#include "../inc/nbk_gdi.h"
#include "../inc/nbk_callback.h"
#include "../css/cssSelector.h"
#include "../loader/loader.h"
#include "../loader/upCmd.h"
#include "../tools/slist.h"
#include "document.h"
#include "view.h"

#ifdef __cplusplus
extern "C" {
#endif
    
enum NEWORKSTATE {
    
    NEWORK_IDLE,
    NEWORK_PARSE,
    NEWORK_LAYOUT,
    NEWORK_PAINT
};

typedef struct _NBK_Page {
    
    nid         id;
    
    uint8       workState;
    uint8       mode;
    
    uint8       main : 1;
    uint8       begin : 1;
    uint8       attach : 1;
    uint8       cache : 1;
    uint8       refresh : 1;
    uint8       wbxml : 1;
    
    uint8       incNum; // counter
    uint8       docNum; // doc number got from pkg
    
    nid         encoding;
    
    NSettings*  settings;
    
    NDocument*  doc;
    NView*      view;
    NSheet*     sheet;
    
    int         total;
    int         rece; // data received
    
    NBK_Callback    cbEventNotify; // for all events from page
    
    void*       platform;
    
    struct _NBK_Page*   parent;
    NSList*             subPages;
    
    struct _NBK_Page*   modPage;
    struct _NBK_Page*   incPage;
    
    void*       history;
    
} NBK_Page, NPage;

#ifdef NBK_MEM_TEST
int page_memUsed(const NPage* page);
void mem_checker(const NPage* page);
#endif

int page_testApi(NPage* page); // for test

NBK_Page* page_create(NPage* parent, void* pfd);
void page_delete(NPage** page);

void page_setScreenWidth(NPage* page, coord width);
void page_setId(NPage* page, nid id);
nid page_getId(NPage* page);

void page_enablePic(NPage* page, nbool enable);

NUpCmdSet page_decideRequestMode(NPage* page, const char* url, nbool submit);

void page_loadData(NPage* page, const char* url, const char* str, int length);
void page_loadUrl(NPage* page, const char* url);
void page_loadUrlInternal(NPage* page, const char* url, nid enc, char* body, NFileUpload** files, nbool submit);
void page_loadUrlStandard(NPage* page, const char* url, nid enc, char* body, NFileUpload** files);
void page_loadFromCache(NPage* page);
void page_refresh(NPage* page);

nbool page_4ways(NBK_Page* page);

coord page_width(NBK_Page* page);
coord page_height(NBK_Page* page);

void page_paint(NBK_Page* page, NRect* rect);
void page_paintControl(NBK_Page* page, NRect* rect);
void page_paintMainBodyBorder(NBK_Page* page, NRect* rect, NNode* focus);

void page_setEventNotify(NBK_Page* page, NBK_CALLBACK func, void* user);

void page_onResponse(NEResponseType type, void* user, void* data, int length, int comprLen);

void page_stop(NPage* page);

void page_setFocusedNode(NPage* page, NNode* node);

//nbool page_hasMainBody(NPage* page);
//void page_createMainBody(NPage* page, NNode* root, NPage* mainBody);
//void page_resetMainBody(NPage* mainBody);
//NNode* page_getMainBodyByPos(NPage* page, coord x, coord y);
//nbool page_isPaintMainBodyBorder(NPage* page, NRect* rect);
//nbool page_isMainBodyMode(NPage* page);

//void page_updateCtrlNode(NPage* page, NNode* node,NRect* rect);
nbool page_isCtrlNode(NPage* page, NNode* node);

NPage* page_getMainPage(NPage* page);
nid page_getStopId(NPage* page);

void page_layout(NPage* page, nbool force);

#ifdef __cplusplus
}
#endif

#endif /* __NBK_PAGE_H__ */
