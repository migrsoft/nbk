#ifndef __NBK_CORE_H__
#define __NBK_CORE_H__

#include "../stdc/inc/config.h"
#include "../stdc/inc/nbk_gdi.h"
#include "../stdc/inc/nbk_settings.h"
#include "../stdc/dom/history.h"
#include "../stdc/dom/node.h"
#include "../stdc/editor/textSel.h"
#include <SDL.h>
#include "nbkgdi.h"
#include "runtime.h"
#include "resmgr.h"
#include "picmgr.h"
#include "probe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NBK_Msg {
        
    int eventId;
        
    int imgCur;
    int imgTotal;
    int datReceived;
    int datTotal;
        
    int x;
    int y;
        
    NFloat	zoom;

} NBK_Msg;

typedef struct _NBK_toolbar {

    SDL_Surface*    btnPrev;
    SDL_Surface*    btnNext;
    SDL_Surface*    btnRefresh;
    SDL_Surface*    btnStop;
    SDL_Surface*    btnHome;
    SDL_Surface*    progress;
    SDL_Surface*	progress_1; // indeterminate
    SDL_Surface*	progress_2;

    NPoint  pos[4];

    nbool    inLoading;
    nbool    captureMouse;

} NBK_toolbar;

typedef struct _NBK_progress {

    char    msg[64];

    int     curr;
    int     total;
    nbool    doc;
    nbool	indeterminate;

} NBK_progress;

typedef struct _NBK_core {

	NBK_probe*		probe;
	NBK_gdi*		gdi;
	NBK_timerMgr*	timerMgr;
	NBK_resMgr*		resMgr;
	NBK_picMgr*		picMgr;

	NTimer*		evtTimer;
    int			eventId;
    // event queue
    NBK_Msg*	evtQueue;
    int			evtAdd;
    int			evtUsed;

	NSettings	settings;
    NNightTheme nightTheme;
	NHistory*	history;

	NRect		viewRect;
	int			viewXOffset;
	NPoint		arrowPos;
	NPoint		lastPos;

	NPoint		lastPenPos;
	NNode*		penFocus;

	nbool		useMinZoom;
	nbool		inEditing;
	nbool		in4ways;
    nbool        showMainBody; // 显示主体指示框
    nbool        inMainBodyMode; // 当前为主体阅读模式
    nbool        catchMotion;

	NFloat		minZoom;
	NFloat		curZoom;

    // 进入主体模式后，保存正常模式参数
    NPoint      lastPosSave;
    NFloat      minZoomSave;
    NFloat      curZoomSave;

    NTextSel*   textSel;

    NBK_toolbar     toolbar;
    NBK_progress    progress;

} NBK_core;

extern NBK_core* g_nbk_core;

NBK_core* nbk_create(void);
void nbk_delete(NBK_core** nbk);

void nbk_updateCaption(NBK_core* nbk);
void nbk_setScreen(NBK_core* nbk, SDL_Surface* screen);
void nbk_display(NBK_core* nbk, SDL_Surface* dev);

void nbk_init(NBK_core* nbk);

void nbk_loadUrl(NBK_core* nbk, const char* url);
void nbk_drawPage(NBK_core* nbk, const NRect* rect);
void nbk_drawPageWithOffset(NBK_core* nbk, const NRect* rect, const NPoint offset);

void nbk_calcMinZoom(NBK_core* nbk);
void nbk_changeZoom(NBK_core* nbk, NFloat zoom);
void nbk_updateScreen(NBK_core* nbk);
void nbk_reposition(NBK_core* nbk, int x, int y);

void nbk_handleEvent(NBK_core* nbk, SDL_Event evt);

void nav_prev(NBK_core* nbk);
void nav_next(NBK_core* nbk);
void nav_refresh(NBK_core* nbk);

NNode* nbk_getFocusByPos(NBK_core* nbk, NPoint pos);
void nbk_setFocus(NBK_core* nbk, NNode* focus);
void nbk_clickFocus(NBK_core* nbk, NNode* focus);

void nbk_switchNightTheme(NBK_core* nbk);

void nbk_textSelBegin(NBK_core* nbk);
void nbk_textSelEnd(NBK_core* nbk);

void nbk_inspectByPos(NBK_core* nbk, const NPoint* pos);

#ifdef __cplusplus
}
#endif

#endif
