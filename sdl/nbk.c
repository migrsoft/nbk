// create by wuyulun, 2011.12.24

#include "../stdc/inc/nbk_gdi.h"
#include "../stdc/inc/nbk_cbDef.h"
#include "../stdc/inc/nbk_version.h"
#include "../stdc/css/color.h"
#include "../stdc/dom/xml_atts.h"
#include "../stdc/dom/page.h"
#include "../stdc/dom/document.h"
#include "../stdc/dom/view.h"
#include "../stdc/render/imagePlayer.h"
#include "../stdc/render/renderImage.h"
#include "../stdc/tools/str.h"
#include "../stdc/tools/unicode.h"
#include <SDL.h>
#include <SDL_image.h>
#include "nbk.h"
#include "nbk_conf.h"
#include "runtime.h"
#include "ini.h"
#include "SDL_gfxPrimitives.h"

#ifdef PLATFORM_WEBOS
    #include "PDL.h"
    #include "glutil.h"
#endif

#define NBK_WIN_MAJOR	1
#define NBK_WIN_MINOR	0
#define NBK_WIN_REV		15

#define NBK_MAX_EVENT_QUEUE     64
#define EVTQ_INC(i) ((i < NBK_MAX_EVENT_QUEUE - 1) ? i+1 : 0)
#define EVTQ_DEC(i) ((i > 0) ? i-1 : NBK_MAX_EVENT_QUEUE - 1)

#define MOVE_DISTANCE_BY_ARROW  0.5
#define MOVE_DISTANCE_BY_SCREEN 0.85

#define ARROW_MOVE_STEP 12
#define ARROW_STOP      2

#define PEN_MOVE_MIN    24

#define FIXED_SCREEN_WIDTH  0
#define SCREEN_WIDTH        240

#define TOOLBAR_HEIGHT          48
#define TOOLBAR_ICON_SIZE       48
#define TOOLBAR_ICON_SPACING    4

#define GET_NBK_PAGE(history)   (history_curr((NHistory*)history))

NBK_core* g_nbk_core = N_NULL;


//#define DEBUG_EVENT_QUEUE

#ifdef DEBUG_EVENT_QUEUE
static char* l_evtDesc[NBK_EVENT_LAST] = {
    "none",
    "new document",
    "history document",
    "do ia",
    "waiting",
    "got response",
    "new document begin",
    "got data",
    "update (size changed)",
    "update pics",
    "layout finish",
    "image downloading",
    "paint control",
    "enter main body",
    "quit main body",
    "quit main body after click",
    "loading error page",
    "reposition",
    "got inc-data",
    "download attachment",
    "text select finish",
    "debug"
};
#endif

static void nbk_func_test(NBK_core* nbk)
{
    if (nbk->textSel) {
        int len;
        wchr* txt = textSel_getText(nbk->textSel, &len);
        if (txt) {
            probe_outputWchr(nbk->probe, txt, len);
            NBK_free(txt);
        }
    }
}

// -----------------------------------------------------------------------------
// 显示提示信息
// -----------------------------------------------------------------------------

static void nbk_set_message(NBK_core* nbk, const char* msg)
{
    if (msg == N_NULL)
        nbk->progress.msg[0] = 0;
    else
        nbk_strcpy(nbk->progress.msg, msg);

//    nbk->gdi->update = N_TRUE;
}

static void nbk_set_progress(NBK_core* nbk, int curr, int total, nbool doc)
{
	if (total != -1) {
		nbk->progress.indeterminate = N_FALSE;
		nbk->progress.curr = curr;
		nbk->progress.total = total;
		nbk->progress.doc = doc;
	}

    nbk->gdi->update = N_TRUE;
}

//static void progress_draw(NBK_core* nbk)
//{
//	coord fh = 8;
//	NSize lcd;
//	NRect pr;
//    int size;
//    coord w;
//
//    if (nbk->progress.msg[0] == 0)
//        return;
//
//	lcd = gdi_getScreenSize(nbk->gdi);
//    size = (nbk->progress.total == -1) ? 50000 : nbk->progress.total;
//    w = lcd.w - (TOOLBAR_ICON_SIZE + TOOLBAR_ICON_SPACING) * 4;
//
//	fh += 4;
//	pr.l = (TOOLBAR_ICON_SIZE + TOOLBAR_ICON_SPACING) * 2;
//	pr.t = lcd.h - fh;
//	pr.r = lcd.w - (TOOLBAR_ICON_SIZE + TOOLBAR_ICON_SPACING) * 2;
//	pr.b = lcd.h;
//
//	NBK_gdi_setBrushColor(nbk, &colorBlue);
//	NBK_gdi_clearRect(nbk, &pr);
//
//	pr.l += 2;
//	pr.t += 2;
//    stringRGBA(nbk->gdi->screen, pr.l, pr.t, nbk->progress.msg, colorYellow.r, colorYellow.g, colorYellow.b, colorYellow.a);
//
//    if (nbk->progress.total == -1)
//        nbk->progress.curr %= size;
//
//	pr.l = (TOOLBAR_ICON_SIZE + TOOLBAR_ICON_SPACING) * 2;
//	pr.t = lcd.h - 8 - 4 - 5;
//    pr.r = pr.l + nbk->progress.curr * w / size;
//	pr.b = pr.t + 5;
//
//	NBK_gdi_setBrushColor(nbk, (nbk->progress.doc) ? &colorBlue : &colorFuchsia);
//	NBK_gdi_clearRect(nbk, &pr);
//}

// -----------------------------------------------------------------------------
// 工具栏
// -----------------------------------------------------------------------------

static void toolbar_init(NBK_toolbar* bar, NSize size)
{
    char path[512];
    const char* wp = get_work_path();
    coord y = size.h - TOOLBAR_HEIGHT;

    if (wp == N_NULL)
        return;

    sprintf(path, "%s/images/button_prev.png", wp);
    bar->btnPrev = IMG_Load(path);

    sprintf(path, "%s/images/button_next.png", wp);
    bar->btnNext = IMG_Load(path);

    sprintf(path, "%s/images/button_refresh.png", wp);
    bar->btnRefresh = IMG_Load(path);

//    sprintf(path, "%s/images/button_stop.png", wp);
//    bar->btnStop = IMG_Load(path);

    sprintf(path, "%s/images/progress-pill.png", wp);
    bar->progress = IMG_Load(path);

    sprintf(path, "%s/images/progress_indeterminate.png", wp);
    bar->progress_1 = IMG_Load(path);

    sprintf(path, "%s/images/progress_indicator.png", wp);
    bar->progress_2 = IMG_Load(path);

    sprintf(path, "%s/images/button_home.png", wp);
    bar->btnHome = IMG_Load(path);

    // 后退
    bar->pos[0].x = 0;
    bar->pos[0].y = y;

    // 前进
    bar->pos[1].x = TOOLBAR_ICON_SIZE + TOOLBAR_ICON_SPACING;
    bar->pos[1].y = y;

    // 刷新/停止
    bar->pos[2].x = size.w - TOOLBAR_ICON_SIZE * 2 - TOOLBAR_ICON_SPACING;
    bar->pos[2].y = y;

    // 主页
    bar->pos[3].x = size.w - TOOLBAR_ICON_SIZE;
    bar->pos[3].y = y;
}

static void toolbar_end(NBK_toolbar* bar)
{
    if (bar->btnPrev)
        SDL_FreeSurface(bar->btnPrev);

    if (bar->btnNext)
        SDL_FreeSurface(bar->btnNext);

    if (bar->btnRefresh)
        SDL_FreeSurface(bar->btnRefresh);

    if (bar->btnStop)
        SDL_FreeSurface(bar->btnStop);

    if (bar->btnHome)
        SDL_FreeSurface(bar->btnHome);

    if (bar->progress)
        SDL_FreeSurface(bar->progress);

    if (bar->progress_1)
    	SDL_FreeSurface(bar->progress_1);

    if (bar->progress_2)
    	SDL_FreeSurface(bar->progress_2);
}

static void toolbar_draw_icon(SDL_Surface* screen, SDL_Surface* icon, const NPoint* pt, int index, coord height)
{
    SDL_Rect dst, src;

    src.x = 0;
    src.y = index * height;
    src.w = icon->w;
    src.h = icon->h;

    dst.x = pt->x;
    dst.y = pt->y;
    dst.w = icon->w;
    dst.h = icon->h;

    SDL_BlitSurface(icon, &src, screen, &dst);
}

static void toolbar_draw(NBK_core* nbk, SDL_Surface* layer)
{
	NBK_toolbar* bar = &nbk->toolbar;
	NBK_progress* prg = &nbk->progress;

    if (   bar->btnPrev == NULL
        || bar->btnNext == NULL
        || bar->btnRefresh == NULL
        || bar->btnHome == NULL
        || bar->progress_1 == N_NULL
        || bar->progress_2 == NULL )
        return;

    toolbar_draw_icon(layer, bar->btnPrev, &bar->pos[0], 0, 0);
    toolbar_draw_icon(layer, bar->btnNext, &bar->pos[1], 0, 0);
    toolbar_draw_icon(layer, bar->btnHome, &bar->pos[3], 0, 0);

    if (bar->inLoading) {
    	toolbar_draw_icon(layer, bar->progress, &bar->pos[2], 0, 0);
    	if (prg->indeterminate)
    		toolbar_draw_icon(layer, bar->progress_1, &bar->pos[2], prg->curr, 48);
    	else {
    		int i = prg->curr * 26 / prg->total;
    		i = (i == 26) ? 25 : i;
    		toolbar_draw_icon(layer, bar->progress_2, &bar->pos[2], i, 48);
    	}
    }
    else
        toolbar_draw_icon(layer, bar->btnRefresh, &bar->pos[2], 0, 0);
}

static int toolbar_get_icon_under_point(NBK_toolbar* bar, coord x, coord y)
{
    int i;
    NRect r;

    for (i=0; i < 4; i++) {
        rect_set(&r, bar->pos[i].x, bar->pos[i].y, TOOLBAR_ICON_SIZE, TOOLBAR_ICON_SIZE);
        if (rect_hasPt(&r, x, y))
            return i;
    }

    return -1;
}

static nbool toolbar_handle_event(NBK_core* nbk, const SDL_Event* evt)
{
    NBK_toolbar* bar = &nbk->toolbar;
    nbool handled = N_FALSE;
    int btn;

    switch (evt->type) {
    case SDL_MOUSEBUTTONDOWN:
        btn = toolbar_get_icon_under_point(bar, evt->button.x, evt->button.y);
        bar->captureMouse = (btn == -1) ? N_FALSE : N_TRUE;
        handled = bar->captureMouse;
        break;

    case SDL_MOUSEBUTTONUP:
        if (bar->captureMouse) {
            btn = toolbar_get_icon_under_point(bar, evt->button.x, evt->button.y);
            sync_wait("toolbar");
            switch (btn) {
            case 0:
                nav_prev(nbk);
                break;

            case 1:
                nav_next(nbk);
                break;

            case 2:
                if (bar->inLoading) {
                    bar->inLoading = N_FALSE;
                    page_stop(GET_NBK_PAGE(nbk->history));
                }
                else {
                    nav_refresh(nbk);
                }
                break;

            case 3:
                nbk_loadUrl(nbk, ini_getString(NEINI_HOMEPAGE));
                break;
            }
            sync_post("toolbar");
            handled = N_TRUE;
        }
        break;

    case SDL_MOUSEMOTION:
        handled = bar->captureMouse;
        break;
    }

    return handled;
}

static void toolbar_set_loading(NBK_core* nbk, nbool loading)
{
    nbk->toolbar.inLoading = loading;

	nbk->progress.indeterminate = N_TRUE;
	nbk->progress.curr = 0;
	nbk->progress.total = 500;

	nbk->gdi->update = N_TRUE;
}

// -----------------------------------------------------------------------------
// Shell
// -----------------------------------------------------------------------------

// 获取页面高度（加工具条高度）
static coord get_page_height(NPage* page)
{
    coord h = page_height(page);
    return h + TOOLBAR_HEIGHT;
}

// 显示页面长度标尺
static void nbk_draw_page_pos(NBK_core* nbk)
{
	NSize lcd = gdi_getScreenSize(nbk->gdi);
    coord w = page_width(GET_NBK_PAGE(nbk->history));
    coord h = get_page_height(GET_NBK_PAGE(nbk->history));

    if (h > lcd.h) {
        coord y = nbk->viewRect.t * lcd.h / (h - lcd.h);
	    filledCircleRGBA(nbk->gdi->screen, lcd.w - 6, y, 5, colorRed.r, colorRed.g, colorRed.b, 128);
    }

    if (w > lcd.w) {
        coord x = nbk->viewRect.l * lcd.w / (w - lcd.w);
	    filledCircleRGBA(nbk->gdi->screen, x, lcd.h - 6, 5, colorRed.r, colorRed.g, colorRed.b, 128);
    }
}

// NBK 事件处理入口
static void event_schedule(void* user)
{
	NBK_core* nbk = (NBK_core*)user;
    NBK_Msg* nevt;

	tim_stop(nbk->evtTimer);

    nevt = &nbk->evtQueue[nbk->evtUsed];
    if (nevt->eventId == 0)
        return;

#ifdef DEBUG_EVENT_QUEUE
	fprintf(stderr, "event: %s\n", l_evtDesc[nevt->eventId]);
#endif

    nbk->eventId = nevt->eventId;
    switch (nevt->eventId) {

    // 准备发出新文档请求
    case NBK_EVENT_NEW_DOC:
    {
        nbk_set_message(nbk, "request...");
        toolbar_set_loading(nbk, N_TRUE);
        break;
    }

    // 打开历史页面
    case NBK_EVENT_HISTORY_DOC:
    {
        nbk_set_message(nbk, "history...");
        toolbar_set_loading(nbk, N_TRUE);
        break;
    }

    // Post 数据已发出，等待响应
    case NBK_EVENT_WAITING:
    {
        nbk_set_message(nbk, "waiting...");
        break;
    }

    // 接收到数据，开始建立文档
    case NBK_EVENT_NEW_DOC_BEGIN:
    {
        //Uint32 cv;
        //NColor c = (nbk->settings.night) ? nbk->nightTheme.background : colorWhite;
        //cv = SDL_MapRGB(nbk->gdi->screen->format, c.r, c.g, c.b);
        //SDL_FillRect(nbk->gdi->screen, NULL, cv);
        break;
    }

    // 收到服务器回应
    case NBK_EVENT_GOT_RESPONSE:
    {
        nbk_set_message(nbk, "response...");
        break;
    }

    // 接收到数据
    case NBK_EVENT_GOT_DATA:
    {
        //char msg[64];
        //sprintf(msg, "receiving %d / %d ...", nevt->datReceived, nevt->datTotal);
        //nbk_set_message(nbk, msg);
		nbk_set_progress(nbk, nevt->datReceived, nevt->datTotal, N_TRUE);
        break;
    }

    // 更新视图，文档尺寸有改变
    case NBK_EVENT_UPDATE:
    {
        nbk_calcMinZoom(nbk);
        nbk_updateScreen(nbk);
        break;
    }

    // 更新视图，文档尺寸无改变
    case NBK_EVENT_UPDATE_PIC:
    {
        nbk_updateScreen(nbk);
        break;
    }
	
    // 文档排版完成
    case NBK_EVENT_LAYOUT_FINISH:
    {
		nbk_updateCaption(nbk);
        nbk_calcMinZoom(nbk);
        nbk->lastPos.x = nevt->x;
        nbk->lastPos.y = nevt->y;
        nbk_updateScreen(nbk);
        break;
    }

    // 正在下载图片
    case NBK_EVENT_DOWNLOAD_IMAGE:
    {
        if (nevt->imgCur < nevt->imgTotal) {
            char msg[64];
            sprintf(msg, "images %d / %d ...", nevt->imgCur, nevt->imgTotal);
            nbk_set_message(nbk, msg);
			nbk_set_progress(nbk, nevt->imgCur, nevt->imgTotal, N_FALSE);
        }
        else {
            nbk_set_message(nbk, N_NULL);
            nbk_updateScreen(nbk);
            toolbar_set_loading(nbk, N_FALSE);
        }
        break;
    }

    // 绘制控件，文档尺寸无改变
    case NBK_EVENT_PAINT_CTRL:
    {
        nbk_updateScreen(nbk);
        break;
    }

    // 进入主体模式
    case NBK_EVENT_ENTER_MAINBODY:
    {
        // 保存状态
        nbk->lastPosSave = nbk->lastPos;
        nbk->minZoomSave = nbk->minZoom;
        nbk->curZoomSave = nbk->curZoom;
        // 设置新状态
        point_set(&nbk->lastPenPos, 0, 0);
        nfloat_set(&nbk->minZoom, 1, 0);
        nfloat_set(&nbk->curZoom, 1, 0);
        nbk->inMainBodyMode = N_TRUE;
        break;
    }

    // 退出主体模式
    case NBK_EVENT_QUIT_MAINBODY:
    {
        // 恢复状态
        nbk->lastPos = nbk->lastPosSave;
        nbk->minZoom = nbk->minZoomSave;
        nbk->curZoom = nbk->curZoomSave;
        nbk->inMainBodyMode = N_FALSE;
        nbk_updateScreen(nbk);
        break;
    }

    // 退出主体模式
    case NBK_EVENT_QUIT_MAINBODY_AFTER_CLICK:
    {
        break;
    }

    // 通知即将载入错误页
    case NBK_EVENT_LOADING_ERROR_PAGE:
    {
        break;
    }

    // 文档重定位
    case NBK_EVENT_REPOSITION:
    {
		nbk->curZoom = nevt->zoom;
		nbk_reposition(nbk, nevt->x, nevt->y);
        nbk_updateScreen(nbk);
        break;
	}
        
    case NBK_EVENT_DO_IA:
        fprintf(stderr, "\n *** nbk-event NBK_EVENT_DO_IA\n\n");
        break;

    case NBK_EVENT_GOT_INC:
        fprintf(stderr, "\n *** nbk-event NBK_EVENT_GOT_INC\n\n");
        break;

    case NBK_EVENT_TEXTSEL_FINISH:
        fprintf(stderr, "select finish!\n");
        break;

	// other
    }

    NBK_memset(nevt, 0, sizeof(NBK_Msg));
    nbk->evtUsed = EVTQ_INC(nbk->evtUsed);
    if (nbk->evtQueue[nbk->evtUsed].eventId) {
        // 存在未处理事件
		tim_start(nbk->evtTimer, 1, 5000);
    }
}

// 内核事件回调接口
static int nbk_core_message_pump(int id, void* user, void* info)
{
    NBK_core* self = (NBK_core*)user;
    NBK_Msg* nevt;
    NBK_Msg* eq = self->evtQueue;

    // 调试输出
    if (id == NBK_EVENT_DEBUG1) {
		probe_output(self->probe, info);
        return 0;
    }
    // 附件下载通知
    if (id == NBK_EVENT_DOWNLOAD_FILE) {
        NBK_Event_DownloadFile* evt = (NBK_Event_DownloadFile*)info;
        fprintf(stderr, "Attachment: %s\n", evt->fileName);
        fprintf(stderr, "       Src: %s\n", evt->url);
        fprintf(stderr, "      Size: %d\n", evt->size);
		return 0;
	}

	// 合并事件
    if (   id == NBK_EVENT_DOWNLOAD_IMAGE
        || id == NBK_EVENT_UPDATE_PIC
        || id == NBK_EVENT_GOT_DATA
        || id == NBK_EVENT_REPOSITION )
	{
        // 查找未处理事件
        int pos = EVTQ_DEC(self->evtAdd);
        nevt = NULL;
        while (eq[pos].eventId && pos != self->evtAdd) {
            if (eq[pos].eventId == id) {
                nevt = &eq[pos];
                break;
            }
            pos = EVTQ_DEC(pos);
        }
        
        if (nevt) {
            if (id == NBK_EVENT_DOWNLOAD_IMAGE) {
                NBK_Event_GotImage* evt = (NBK_Event_GotImage*)info;
                nevt->imgCur = evt->curr;
                nevt->imgTotal = evt->total;
                nevt->datReceived = evt->receivedSize;
                nevt->datTotal = evt->totalSize;
            }
            else if (id == NBK_EVENT_GOT_DATA) {
                NBK_Event_GotResponse* evt = (NBK_Event_GotResponse*)info;
                nevt->datReceived = evt->received;
                nevt->datTotal = evt->total;
            }
            else if (id == NBK_EVENT_REPOSITION) {
                NBK_Event_RePosition* evt = (NBK_Event_RePosition*)info;
                nevt->x = evt->x;
                nevt->y = evt->y;
                nevt->zoom = evt->zoom;
            }
            return 0;
        }
    }

    // 将事件加入事件队列
    nevt = &eq[self->evtAdd];
    self->evtAdd = EVTQ_INC(self->evtAdd);
    NBK_memset(nevt, 0, sizeof(NBK_Msg));
    nevt->eventId = id;

    switch (id) {
    
    // 准备发出新文档请求
    case NBK_EVENT_NEW_DOC:
    {
        self->arrowPos.x = ARROW_STOP;
		self->arrowPos.y = ARROW_STOP;
        self->useMinZoom = N_TRUE;
        break;
    }
    case NBK_EVENT_NEW_DOC_BEGIN:
        gdi_resetFont(self->gdi);
        break;

    case NBK_EVENT_HISTORY_DOC:
    {
        self->useMinZoom = N_FALSE;
        break;
    }
    
    // 接收到数据
    case NBK_EVENT_GOT_DATA:
    {
        NBK_Event_GotResponse* evt = (NBK_Event_GotResponse*)info;
        nevt->datReceived = evt->received;
        nevt->datTotal = evt->total;
        break;
    }
    
    // 正在下载图片
    case NBK_EVENT_DOWNLOAD_IMAGE:
    {
        NBK_Event_GotImage* evt = (NBK_Event_GotImage*)info;
        nevt->imgCur = evt->curr;
        nevt->imgTotal = evt->total;
        nevt->datReceived = evt->receivedSize;
        nevt->datTotal = evt->totalSize;
        break;
    }

    // 排版完成（返回上次查看本页的位置）
    case NBK_EVENT_LAYOUT_FINISH:
    // 文档重定位
    case NBK_EVENT_REPOSITION:
    {
        NBK_Event_RePosition* evt = (NBK_Event_RePosition*)info;
        nevt->x = evt->x;
        nevt->y = evt->y;
        nevt->zoom = evt->zoom;
        break;
    }

    default:
        break;
    }

	// 启动事件调度
	tim_start(self->evtTimer, 1, 5000);

	return 0;
}

nbool NBK_ext_isDisplayDocument(void* document)
{
	return N_TRUE;
}

NBK_core* nbk_create(void)
{
	int cfg;
	NPage* page;
	NBK_core* nbk = (NBK_core*)NBK_malloc0(sizeof(NBK_core));

	g_nbk_core = nbk;

    nbk->probe = probe_create();
	nbk->gdi = gdi_create();
	nbk->timerMgr = timerMgr_create();
	nbk->resMgr = resMgr_create();
	nbk->picMgr = picMgr_create();

	nbk->resMgr->probe = nbk->probe;

	// 初始化事件队列
	nbk->evtTimer = tim_create(event_schedule, nbk);
    nbk->evtQueue = (NBK_Msg*)NBK_malloc0(sizeof(NBK_Msg) * NBK_MAX_EVENT_QUEUE);
    nbk->evtAdd = nbk->evtUsed = 0;

	// 内核设置项
	nbk->settings.mainFontSize = ini_getInt(NEINI_FONT_SIZE);
    nbk->settings.fontLevel = NEFNTLV_MIDDLE;
	nbk->settings.allowImage = (nbool)!ini_getInt(NEINI_IMAGE_OFF);
	nbk->settings.allowIncreateMode = 1;
	nbk->settings.selfAdaptionLayout = (nbool)ini_getInt(NEINI_SELF_ADAPT);
    nbk->settings.allowCollapse = (nbool)ini_getInt(NEINI_BLOCK_COLLAPSE);

    // 夜间配色
    color_set(&nbk->nightTheme.text, 0x73, 0x82, 0x95, 0xff);
    color_set(&nbk->nightTheme.link, 0x28, 0x5a, 0x8c, 0xff);
    color_set(&nbk->nightTheme.background, 0x0a, 0x0a, 0x0a, 0xff);
    //nbk->settings.night = &nbk->nightTheme;

	cfg = ini_getInt(NEINI_INIT_MODE);
	if (cfg == 1) { // 全排
		nbk->settings.initMode = NEREV_FF_FULL;
		nbk->settings.mode = NEREV_FF_FULL;
	}
	else if (cfg == 2) { // 简排
		nbk->settings.initMode = NEREV_FF_NARROW;
		nbk->settings.mode = NEREV_FF_NARROW;
	}
	else {
		nbk->settings.initMode = NEREV_STANDARD;
		nbk->settings.mode = NEREV_FF_FULL;
	}

	// 创建内核实例
	nbk->history = history_create();
	history_setPlatformData(nbk->history, nbk);
	history_setSettings(nbk->history, &nbk->settings);
	history_enablePic(nbk->history, nbk->settings.allowImage);

	page = history_curr(nbk->history);
	page_setEventNotify(page, nbk_core_message_pump, nbk);

	return nbk;
}

void nbk_delete(NBK_core** nbk)
{
	NBK_core* n = *nbk;

	history_delete(&n->history);

	tim_delete(&n->evtTimer);
	NBK_free(n->evtQueue);

	picMgr_delete(&n->picMgr);
	resMgr_delete(&n->resMgr);
	timerMgr_delete(&n->timerMgr);
	gdi_delete(&n->gdi);
	probe_delete(&n->probe);

    toolbar_end(&n->toolbar);

	NBK_free(n);
	*nbk = N_NULL;
}

void nbk_updateCaption(NBK_core* nbk)
{
#ifndef PLATFORM_WEBOS

	const char* zhi = "\xe7\x9b\xb4";
	const char* yun = "\xe4\xba\x91";
	const char* quan = "\xe5\x85\xa8";
	const char* jian = "\xe7\xae\x80";
    const char* wap = "Wap";

	char title[64];
	int p1, p2;
	NPage* page = GET_NBK_PAGE(nbk->history);
	nid docType = doc_getType(page->doc);

	p1 = sprintf(title, "NBK %s%s | ",
		(nbk->settings.initMode == NEREV_STANDARD) ? zhi : yun,
		(nbk->settings.mode == NEREV_FF_FULL) ? quan : jian);

	switch (docType) {
	case NEDOC_FULL:
		p2 = sprintf(title + p1, "%s%s", yun, quan);
		break;
	case NEDOC_FULL_XHTML:
		p2 = sprintf(title + p1, "%s%s", yun, wap);
		break;
	case NEDOC_NARROW:
		p2 = sprintf(title + p1, "%s%s", yun, jian);
		break;
	default:
		p2 = sprintf(title + p1, "%s", zhi);
		break;
	}

	sprintf(title + p1 + p2, " %d.%d.%d-%d.%d.%d.%d",
        NBK_WIN_MAJOR, NBK_WIN_MINOR, NBK_WIN_REV,
        NBK_VER_MAJOR, NBK_VER_MINOR, NBK_VER_REV, NBK_VER_BUILD);

	SDL_WM_SetCaption(title, NULL);

#endif
}

void nbk_setScreen(NBK_core* nbk, SDL_Surface* screen)
{
	NRect r;

	nbk->gdi->screen = screen;
	
	r.l = r.t = 0;
	r.r = screen->w;
	r.b = screen->h;

	NBK_gdi_setBrushColor(nbk, &colorLightGray);
	NBK_gdi_clearRect(nbk, &r);
}

void nbk_display(NBK_core* nbk, SDL_Surface* dev)
{
	if (nbk->toolbar.inLoading && nbk->progress.indeterminate) {
		nbk->progress.total += 50;
		if (nbk->progress.total >= 500) {
			nbk->progress.total = 0;
			nbk->progress.curr++;
			if (nbk->progress.curr == 12)
				nbk->progress.curr = 0;
			nbk->gdi->update = N_TRUE;
		}
	}

    if (nbk->gdi->update) {
        Uint32 color;
#ifdef PLATFORM_WEBOS
        color = SDL_MapRGB(dev->format, 255, 255, 255);
        SDL_FillRect(dev, NULL, color);
        SDL_BlitSurface(nbk->gdi->screen, NULL, dev, NULL);
        toolbar_draw(nbk, dev);
		gl_drawSurfaceAsTexture(dev);
//		toolbar_draw(nbk, nbk->gdi->screen);
//		gl_drawSurfaceAsTexture(nbk->gdi->screen);
#else
        color = SDL_MapRGB(dev->format, 255, 255, 255);
        SDL_FillRect(dev, NULL, color);
        SDL_BlitSurface(nbk->gdi->screen, NULL, dev, NULL);
        toolbar_draw(nbk, dev);
		SDL_UpdateRect(dev, 0, 0, 0, 0);
#endif
		nbk->gdi->update = N_FALSE;
	}
}

void nbk_init(NBK_core* nbk)
{
	NSize lcd = gdi_getScreenSize(nbk->gdi);
	NPage* page = GET_NBK_PAGE(nbk->history);

    nbk->progress.total = -1;
    toolbar_init(&nbk->toolbar, lcd);

    page_setScreenWidth(page, lcd.w);
	history_setMainBodyWidth(nbk->history, lcd.w);

	nbk->useMinZoom = N_TRUE;
    nbk_calcMinZoom(nbk);
}

void nbk_loadUrl(NBK_core* nbk, const char* url)
{
	NPage* page = history_curr(nbk->history);
    page_loadUrl(page, url);
}

void nbk_calcMinZoom(NBK_core* nbk)
{
	NSize lcd = gdi_getScreenSize(nbk->gdi);
	NHistory* history = nbk->history;
    NBK_Page* p = history_curr(history);
    nid docType = doc_getType(p->doc);
	NFloat minZ;
	coord w;
	NPoint pt;

    nfloat_set(&nbk->minZoom, 1, 0);
	nbk->curZoom = history_getZoom(history);
    minZ = view_getMinZoom(p->view);

    nbk->viewXOffset = 0;

    if (docType == NEDOC_FULL) {
		nbk->minZoom = minZ;
    }
    else if (docType != NEDOC_MAINBODY) {
		nbk->minZoom = minZ;
        history_setZoom(history, minZ);
//        w = page_width(p);
//        if (w < lcd.w)
//            nbk->viewXOffset = (lcd.w - w) >> 1;
    }

	pt = gdi_getDrawOffset(nbk->gdi);
    pt.x += nbk->viewXOffset;
    gdi_setDrawOffset(nbk->gdi, pt);

    // 新打开页面，记录适屏缩放因子
    if (1 && nbk->useMinZoom) {
		history_setZoom(history, nbk->minZoom);
		nbk->curZoom = nbk->minZoom;
    }
}

void nbk_changeZoom(NBK_core* nbk, NFloat zoom)
{
	NSize lcd = gdi_getScreenSize(nbk->gdi);
	coord w, h;

	nbk->curZoom = zoom;
	history_setZoom(nbk->history, zoom);
	w = page_width(GET_NBK_PAGE(nbk->history));
    h = get_page_height(GET_NBK_PAGE(nbk->history));

	rect_set(&nbk->viewRect, nbk->viewRect.l, nbk->viewRect.t, lcd.w, lcd.h);

	if (nbk->viewRect.r > w) {
		nbk->viewRect.r = w;
		nbk->viewRect.l = nbk->viewRect.r - lcd.w;
		if (nbk->viewRect.l < 0)
			nbk->viewRect.l = 0;
	}
	if (nbk->viewRect.b > h) {
		nbk->viewRect.b = h;
		nbk->viewRect.t = nbk->viewRect.b - lcd.h;
		if (nbk->viewRect.t < 0)
			nbk->viewRect.t = 0;
	}

	nbk_drawPage(nbk, &nbk->viewRect);
}

void nbk_updateViewRect(NBK_core* nbk)
{
	NSize lcd = gdi_getScreenSize(nbk->gdi);
	NBK_Page* p = GET_NBK_PAGE(nbk->history);
    coord w = page_width(p);
    coord h = get_page_height(p);

    nbk->viewRect.l = 0;
    nbk->viewRect.r = N_MIN(w, lcd.w);
    nbk->viewRect.t = 0;
    nbk->viewRect.b = N_MIN(h, lcd.h);

    if (nbk->lastPos.x) {
        coord vw = nbk->viewRect.r - nbk->viewRect.l;
        if (w > lcd.w && nbk->viewRect.l + nbk->lastPos.x < w) {
            nbk->viewRect.l += nbk->lastPos.x;
            if (nbk->viewRect.l + vw > w)
                nbk->viewRect.l = w - vw;
            nbk->viewRect.r = nbk->viewRect.l + vw;
        }
    }
    if (nbk->lastPos.y) {
        coord vh = nbk->viewRect.b - nbk->viewRect.t;
        if (h > lcd.h && nbk->viewRect.t + nbk->lastPos.y < h) {
            nbk->viewRect.t += nbk->lastPos.y;
            if (nbk->viewRect.t + vh > h)
                nbk->viewRect.t = h - vh;
            nbk->viewRect.b = nbk->viewRect.t + vh;
        }
    }
}

void nbk_updateScreen(NBK_core* nbk)
{
    if (   nbk->eventId != NBK_EVENT_UPDATE_PIC
        && nbk->eventId != NBK_EVENT_PAINT_CTRL
        && nbk->eventId != NBK_EVENT_REPOSITION )
        nbk_updateViewRect(nbk);

    if (nbk->eventId == NBK_EVENT_PAINT_CTRL) {
        // 绘制单个控件，当文档尺寸小于屏幕尺寸时，绘图区域扩大到屏幕尺寸
		page_paintControl(GET_NBK_PAGE(nbk->history), &nbk->viewRect);
    }
    else
		nbk_drawPage(nbk, &nbk->viewRect);

    if (nbk->eventId == NBK_EVENT_LAYOUT_FINISH) {
        nbk_set_message(nbk, "Layout Finish");
    }
}

void nbk_drawPage(NBK_core* nbk, const NRect* rect)
{
	NPoint offset = {0, 0};
	nbk_drawPageWithOffset(nbk, rect, offset);

    nbk->lastPos.x = nbk->viewRect.l;
    nbk->lastPos.y = nbk->viewRect.t;
}

void nbk_drawPageWithOffset(NBK_core* nbk, const NRect* rect, const NPoint offset)
{
	NPage* page = GET_NBK_PAGE(nbk->history);
	NSize lcd = gdi_getScreenSize(nbk->gdi);
    int w = page_width(page);
    int h = get_page_height(page);
    NColor bgColor = (nbk->settings.night) ? nbk->nightTheme.background : colorLightGray;

	// 局部绘制暂停动画
    if (offset.x == 0 && offset.y == 0)
        view_resume(page->view);
    else
        view_pause(page->view);

    // 清除未覆盖区域
    if (w < lcd.w) {
        if (nbk->viewXOffset == 0) {
            // 清除右侧区域
			NRect r = {w, 0, lcd.w, lcd.h};
			NBK_gdi_setBrushColor(nbk, &bgColor);
			NBK_gdi_clearRect(nbk, &r);
        }
        else {
            // 清除左侧、右侧区域
			NRect r = {0, 0, nbk->viewXOffset, lcd.h};
			NBK_gdi_setBrushColor(nbk, &bgColor);
			NBK_gdi_clearRect(nbk, &r);
            rect_set(&r, nbk->viewXOffset + w, 0, lcd.w, lcd.h);
			NBK_gdi_clearRect(nbk, &r);
        }
    }
    if (h < lcd.h) {
		NRect r = {0, h, lcd.w, lcd.h};
		NBK_gdi_setBrushColor(nbk, &bgColor);
		NBK_gdi_clearRect(nbk, &r);
    }

	gdi_setDrawOffset(nbk->gdi, offset);

	page_paint(page, &nbk->viewRect);

    // 显示主体区域
    if (    nbk->curZoom.i == 0
        || (nbk->curZoom.i == 1 && nbk->curZoom.f == 0) ) {
        NRect r;
        NBK_helper_getViewableRect(nbk, &r);
        nbk->showMainBody = page_isPaintMainBodyBorder(page, &r);
        if (nbk->showMainBody) {
            r = *rect;
            page_paintMainBodyBorder(page, &r, N_NULL);
        }
    }

    // 绘制文本选择区
    if (!nbk->showMainBody && nbk->textSel)
        textSel_paint(nbk->textSel, &nbk->viewRect);
}

static void nbk_drag_by_pen(NBK_core* nbk, coord x, coord y)
{
	NPage* page = GET_NBK_PAGE(nbk->history);
	NSize lcd = gdi_getScreenSize(nbk->gdi);
    coord w = page_width(page);
    coord h = get_page_height(page);
    coord dx = x;
    coord dy = y;
    nbool update = N_FALSE;
    NRect vr;
    NRect pr;
	NPoint pos;
	NRect rect;

#if 0

    if (w > lcd.w) {
        if (dx < 0)
            dx = (nbk->viewRect.l + dx < 0) ? dx - (nbk->viewRect.l + dx) : dx;
        else
            dx = (nbk->viewRect.r + dx > w) ? dx - (nbk->viewRect.r + dx - w) : dx;
        update = N_TRUE;
    }
    else
        dx = 0;

    if (h > lcd.h) {
        if (dy < 0)
            dy = (nbk->viewRect.t + dy < 0) ? dy - (nbk->viewRect.t + dy) : dy;
        else
            dy = (nbk->viewRect.b + dy > h) ? dy - (nbk->viewRect.b + dy - h) : dy;
        update = N_TRUE;
    }
    else
        dy = 0;

    if (!update)
        return;

    // 水平移动
    if (dx) {
		pr = nbk->viewRect;
        nbk->viewRect.l += dx;
        nbk->viewRect.r += dx;

        if (dx > 0) {
            // 向左移动
			pos.x = -dx;
			pos.y = 0;
			rect_set(&rect, dx, 0, lcd.w, lcd.h);
			gdi_copyRect(nbk->gdi, pos, rect);
            pr.l += lcd.w;
            pr.r = pr.l + dx;
            rect_set(&vr, pr.l, pr.t, pr.r, pr.b);
			pos.x = lcd.w - dx;
			pos.y = 0;
            nbk_drawPageWithOffset(nbk, &vr, pos);
        }
        else if (dx < 0) {
            // 向右移动
			pos.x = -dx;
			pos.y = 0;
			rect_set(&rect, 0, 0, lcd.w, lcd.h);
			gdi_copyRect(nbk->gdi, pos, rect);
            pr.l += dx;
            pr.r = pr.l + N_ABS(dx);
            rect_set(&vr, pr.l, pr.t, pr.r, pr.b);
			pos.x = pos.y = 0;
            nbk_drawPageWithOffset(nbk, &vr, pos);
        }
    }

    // 垂直移动
    if (dy) {
		pr = nbk->viewRect;
        nbk->viewRect.t += dy;
        nbk->viewRect.b += dy;

        if (dy > 0) {
            // 向上移动
			pos.x = 0;
			pos.y = -y;
			rect_set(&rect, 0, dy, lcd.w, lcd.h);
			gdi_copyRect(nbk->gdi, pos, rect);
            pr.t += lcd.h;
            pr.b = pr.t + dy;
            rect_set(&vr, pr.l, pr.t, pr.r, pr.b);
			pos.x = 0;
			pos.y = lcd.h - dy;
            nbk_drawPageWithOffset(nbk, &vr, pos);
        }
        else if (dy < 0) {
            // 向下移动
			pos.x = 0;
			pos.y = -dy;
			rect_set(&rect, 0, 0, lcd.w, lcd.h);
			gdi_copyRect(nbk->gdi, pos, rect);
            pr.t += dy;
            pr.b = pr.t + N_ABS(dy);
            rect_set(&vr, pr.l, pr.t, pr.r, pr.b);
			pos.x = pos.y = 0;
            nbk_drawPageWithOffset(nbk, &vr, pos);
        }
    }

#else

    if (w > lcd.w) {
        if (dx < 0)
			dx = (nbk->viewRect.l + dx < 0) ? dx - (nbk->viewRect.l + dx) : dx;
        else
			dx = (nbk->viewRect.r + dx > w) ? dx - (nbk->viewRect.r + dx - w) : dx;
        nbk->viewRect.l += dx;
        nbk->viewRect.r += dx;
        update = N_TRUE;
    }

    if (h > lcd.h) {
        if (dy < 0)
			dy = (nbk->viewRect.t + dy < 0) ? dy - (nbk->viewRect.t + dy) : dy;
        else
			dy = (nbk->viewRect.b + dy > h) ? dy - (nbk->viewRect.b + dy - h) : dy;
        nbk->viewRect.t += dy;
        nbk->viewRect.b += dy;
        update = N_TRUE;
    }

    if (update)
		nbk_drawPage(nbk, &nbk->viewRect);

#endif
}

static void nbk_page_down(NBK_core* nbk)
{
	NSize lcd = gdi_getScreenSize(nbk->gdi);
	NBK_Page* p = GET_NBK_PAGE(nbk->history);
    coord h = get_page_height(p);
	int move, t;

    if (lcd.h >= h)
        return;
    if (nbk->viewRect.b == h)
        return;

    move = lcd.h - TOOLBAR_HEIGHT;

    t = nbk->viewRect.b + move;
    if (t > h) t = h;

    nbk->viewRect.b = t;
    nbk->viewRect.t = nbk->viewRect.b - lcd.h;

	nbk_drawPage(nbk, &nbk->viewRect);
    nbk_draw_page_pos(nbk);
}

static void nbk_page_up(NBK_core* nbk)
{
	NSize lcd = gdi_getScreenSize(nbk->gdi);
	NBK_Page* p = GET_NBK_PAGE(nbk->history);
    coord h = get_page_height(p);
	int move, t;

    if (lcd.h >= h)
        return;
    if (nbk->viewRect.t == 0)
        return;

    move = lcd.h - TOOLBAR_HEIGHT;

    t = nbk->viewRect.t - move;
    if (t < 0) t = 0;

    nbk->viewRect.t = t;
    nbk->viewRect.b = nbk->viewRect.t + lcd.h;

	nbk_drawPage(nbk, &nbk->viewRect);
    nbk_draw_page_pos(nbk);
}

static void nbk_adjust_view(NBK_core* nbk, NRect* rect, nbool dirDown)
{
	NSize lcd = gdi_getScreenSize(nbk->gdi);
	NBK_Page* p = GET_NBK_PAGE(nbk->history);
    coord h = get_page_height(p);

    if (lcd.h >= h)
        return;

    if (dirDown) {
        if (rect->b > nbk->viewRect.b) {
            nbk->viewRect.t = rect->t;
            nbk->viewRect.b = nbk->viewRect.t + lcd.h;
            if (nbk->viewRect.b > h) {
                nbk->viewRect.b = h;
                nbk->viewRect.t = nbk->viewRect.b - lcd.h;
            }
        }
    }
    else {
        if (rect->t < nbk->viewRect.t) {
            nbk->viewRect.b = rect->b;
            nbk->viewRect.t = nbk->viewRect.b - lcd.h;
            if (nbk->viewRect.t < 0) {
                nbk->viewRect.t = 0;
                nbk->viewRect.b = lcd.h;
            }
        }
    }
}

static void nbk_focus_next(NBK_core* nbk)
{
	NBK_Page* page = GET_NBK_PAGE(nbk->history);
    NNode* focus = (NNode*)view_getFocusNode(page->view);
    NRect r;

    if (focus == NULL) {
        focus = doc_getFocusNode(page->doc, &nbk->viewRect);
    }
    else {
        r = view_getNodeRect(page->view, focus);
        if (rect_isIntersect(&nbk->viewRect, &r))
            focus = doc_getNextFocusNode(page->doc, focus);
        else
            focus = doc_getFocusNode(page->doc, &nbk->viewRect);
    }

    if (focus) {
        r = view_getNodeRect(page->view, focus);
        nbk_adjust_view(nbk, &r, N_TRUE);
        view_setFocusNode(page->view, focus);
		nbk_drawPage(nbk, &nbk->viewRect);
    }
    else
        nbk_page_down(nbk);
}

static void nbk_focus_prev(NBK_core* nbk)
{
	NBK_Page* page = GET_NBK_PAGE(nbk->history);
    NNode* focus = (NNode*)view_getFocusNode(page->view);
    NRect r;

    if (focus == NULL) {
        focus = doc_getFocusNode(page->doc, &nbk->viewRect);
    }
    else {
        r = view_getNodeRect(page->view, focus);
        if (rect_isIntersect(&nbk->viewRect, &r))
            focus = doc_getPrevFocusNode(page->doc, focus);
        else
            focus = doc_getFocusNode(page->doc, &nbk->viewRect);
    }

    if (focus) {
        r = view_getNodeRect(page->view, focus);
        nbk_adjust_view(nbk, &r, N_FALSE);
        view_setFocusNode(page->view, focus);
		nbk_drawPage(nbk, &nbk->viewRect);
    }
    else
        nbk_page_up(nbk);
}

static void nbk_focus_click(NBK_core* nbk)
{
	NBK_Page* page = GET_NBK_PAGE(nbk->history);

	if (nbk->curZoom.i >= 1) {
        void* focus = view_getFocusNode(page->view);
        doc_clickFocusNode(page->doc, (NNode*)focus);
    }
    else {
		NFloat zoom = {1, 0};

        if (page_hasMainBody(page)) {
            coord x, y;
			NNode* body;

            x = nbk->viewRect.l + nbk->arrowPos.x;
            y = nbk->viewRect.t + nbk->arrowPos.y;

            body = history_getMainBodyByPos(nbk->history, x, y);
            if (body) {
                history_enterMainBodyMode(nbk->history, body);
                return;
            }
        }

        nbk_changeZoom(nbk, zoom);
    }
}

static void nbk_move_left(NBK_core* nbk)
{
	NSize lcd = gdi_getScreenSize(nbk->gdi);
	NBK_Page* p = GET_NBK_PAGE(nbk->history);
    coord w = page_width(p);
	int move, l, d;

    if (lcd.w >= w)
        return;
    if (nbk->viewRect.l == 0)
        return;

    move = lcd.w * MOVE_DISTANCE_BY_SCREEN;
    l = nbk->viewRect.l;
    d = nbk->viewRect.l - move;
    if (d < 0) d = 0;

    nbk->viewRect.l = d;
    nbk->viewRect.r = nbk->viewRect.l + lcd.w;

	nbk_drawPage(nbk, &nbk->viewRect);
    nbk_draw_page_pos(nbk);
}

static void nbk_move_right(NBK_core* nbk)
{
	NSize lcd = gdi_getScreenSize(nbk->gdi);
	NBK_Page* p = GET_NBK_PAGE(nbk->history);
    coord w = page_width(p);
	int move, r, d;

    if (lcd.w >= w)
        return;
    if (nbk->viewRect.r == w)
        return;

    move = lcd.w * MOVE_DISTANCE_BY_SCREEN;
    r = nbk->viewRect.r;
    d = nbk->viewRect.r + move;
    if (d > w) d = w;

    nbk->viewRect.r = d;
    nbk->viewRect.l = nbk->viewRect.r - lcd.w;

	nbk_drawPage(nbk, &nbk->viewRect);
    nbk_draw_page_pos(nbk);
}

static void nbk_move_up(NBK_core* nbk)
{
	NSize lcd = gdi_getScreenSize(nbk->gdi);
	NBK_Page* p = GET_NBK_PAGE(nbk->history);
    coord h = get_page_height(p);
	coord move, t, d;

    if (lcd.h >= h)
        return;
    if (nbk->viewRect.t == 0)
        return;

    move = lcd.h * MOVE_DISTANCE_BY_SCREEN;
    t = nbk->viewRect.t;
    d = nbk->viewRect.t - move;
    if (d < 0) d = 0;

    nbk->viewRect.t = d;
    nbk->viewRect.b = nbk->viewRect.t + lcd.h;

	nbk_drawPage(nbk, &nbk->viewRect);
    nbk_draw_page_pos(nbk);
}

static void nbk_move_down(NBK_core* nbk)
{
	NSize lcd = gdi_getScreenSize(nbk->gdi);
	NBK_Page* p = GET_NBK_PAGE(nbk->history);
    coord h = get_page_height(p);
	coord move, b, d;

    if (lcd.h >= h)
        return;
    if (nbk->viewRect.b == h)
        return;

    move = lcd.h * MOVE_DISTANCE_BY_SCREEN;
    b = nbk->viewRect.b;
    d = nbk->viewRect.b + move;
    if (d > h) d = h;

    nbk->viewRect.b = d;
    nbk->viewRect.t = nbk->viewRect.b - lcd.h;

	nbk_drawPage(nbk, &nbk->viewRect);
    nbk_draw_page_pos(nbk);
}

#define NBK_MODE_NUM	3

static void nbk_change_mode(NBK_core* nbk, nbool init)
{
	nid modes[NBK_MODE_NUM] = {NEREV_STANDARD, NEREV_FF_FULL, NEREV_FF_NARROW};
	int i;

	if (init) {
		for (i = 0; i < NBK_MODE_NUM; i++) {
			if (nbk->settings.initMode == modes[i])
				break;
		}
		i++;
		if (i >= NBK_MODE_NUM) i = 0;

		nbk->settings.initMode = modes[i];
		if (modes[i] != NEREV_STANDARD)
			nbk->settings.mode = modes[i];
	}
	else {
		for (i = 1; i < NBK_MODE_NUM; i++) {
			if (nbk->settings.mode == modes[i])
				break;
		}
		i++;
		if (i >= NBK_MODE_NUM) i = 1;

		nbk->settings.mode = modes[i];
	}

	nbk_updateCaption(nbk);
}

static nbool nbk_handle_event_by_control(NBK_core* nbk, SDL_Event evt)
{
	nbool process = N_FALSE;
	NEvent e;

	sync_wait("Control");

	NBK_memset(&e, 0, sizeof(NEvent));
	e.page = GET_NBK_PAGE(nbk->history);

	if (evt.type == SDL_KEYDOWN) {
        // 编辑器处理
		process = N_TRUE;
		e.type = NEEVENT_KEY;

		if (nbk->inEditing) { // 编辑状态
			switch (evt.key.keysym.sym) {
			case SDLK_LEFT:
				e.d.keyEvent.key = NEKEY_LEFT;
				break;
			case SDLK_RIGHT:
				e.d.keyEvent.key = NEKEY_RIGHT;
				break;
			case SDLK_UP:
				e.d.keyEvent.key = NEKEY_UP;
				break;
			case SDLK_DOWN:
				e.d.keyEvent.key = NEKEY_DOWN;
				break;
			case SDLK_BACKSPACE:
				e.d.keyEvent.key = NEKEY_BACKSPACE;
				break;
			default:
				if (   evt.key.keysym.sym >= SDLK_SPACE
					&& evt.key.keysym.sym <= SDLK_z )
				{
					e.d.keyEvent.key = NEKEY_CHAR;
					e.d.keyEvent.chr = evt.key.keysym.sym;
                    if (evt.key.keysym.mod & KMOD_SHIFT) {
                        if (nbk_key_map[e.d.keyEvent.chr])
                            e.d.keyEvent.chr = nbk_key_map[e.d.keyEvent.chr];
                        else if (e.d.keyEvent.chr >= SDLK_a && e.d.keyEvent.chr <= SDLK_z)
                            e.d.keyEvent.chr -= 32;
                    }
                    else if (evt.key.keysym.mod & KMOD_CAPS) {
                        if (e.d.keyEvent.chr >= SDLK_a && e.d.keyEvent.chr <= SDLK_z)
                            e.d.keyEvent.chr -= 32;
                    }
				}
				else
					process = N_FALSE;
				break;
			}

			if (process) {
				if (doc_processKey(&e)) {
				}
				else if (   evt.key.keysym.sym == SDLK_UP
						 || evt.key.keysym.sym == SDLK_DOWN )
				{
					NNode* focus = (NNode*)view_getFocusNode(GET_NBK_PAGE(nbk->history)->view);
					nbk_clickFocus(nbk, focus);
					process = N_FALSE;
				}
			}
		}
		else { // 非编辑状态
			switch (evt.key.keysym.sym) {
			case SDLK_LEFT:
				e.d.keyEvent.key = NEKEY_LEFT;
				break;
			case SDLK_RIGHT:
				e.d.keyEvent.key = NEKEY_RIGHT;
				break;
			case SDLK_UP:
				e.d.keyEvent.key = NEKEY_UP;
				break;
			case SDLK_DOWN:
				e.d.keyEvent.key = NEKEY_DOWN;
				break;
			case SDLK_BACKSPACE:
				e.d.keyEvent.key = NEKEY_BACKSPACE;
				break;
            case SDLK_RETURN:
                e.d.keyEvent.key = NEKEY_ENTER;
                break;
			default:
				process = N_FALSE;
				break;
			}

			if (process) {
                if (nbk->textSel)
                    process = textSel_processEvent(nbk->textSel, &e);
                else
					process = doc_processKey(&e);
			}
		}
	}
	else if (evt.type == SDL_KEYUP) {
		if (nbk->inEditing)
			process = N_TRUE;
        else if (nbk->textSel) {
            switch (evt.key.keysym.sym) {
			case SDLK_LEFT:
			case SDLK_RIGHT:
			case SDLK_UP:
			case SDLK_DOWN:
            case SDLK_RETURN:
                process = N_TRUE;
                break;
            }
        }
	}
	else if (   evt.type == SDL_MOUSEBUTTONDOWN
			 || evt.type == SDL_MOUSEBUTTONUP
			 || evt.type == SDL_MOUSEMOTION )
	{
		e.type = NEEVENT_PEN;
		e.d.penEvent.pos.x = nbk->viewRect.l + evt.button.x;
		e.d.penEvent.pos.y = nbk->viewRect.t + evt.button.y;

		if (evt.type == SDL_MOUSEBUTTONDOWN)
			e.d.penEvent.type = NEPEN_DOWN;
		else if (evt.type == SDL_MOUSEBUTTONUP)
			e.d.penEvent.type = NEPEN_UP;
		else if (   evt.type == SDL_MOUSEMOTION
				 && evt.motion.state == SDL_PRESSED)
			e.d.penEvent.type = NEPEN_MOVE;

        if (nbk->textSel) // 复制模式
            process = textSel_processEvent(nbk->textSel, &e);
        else // 内核控件
		    process = doc_processPen(&e);
	}

	sync_post("Control");
	return process;
}

void nbk_handleEvent(NBK_core* nbk, SDL_Event evt)
{
	NPoint pt;

    if (toolbar_handle_event(nbk, &evt))
        return;

    if (nbk_handle_event_by_control(nbk, evt))
		return;

	sync_wait("Event");

	nbk->in4ways = page_4ways(GET_NBK_PAGE(nbk->history));

	switch (evt.type) {
	case SDL_KEYUP:
	{
		switch (evt.key.keysym.sym) {
		case SDLK_h: // 载入主页
			nbk_loadUrl(nbk, ini_getString(NEINI_HOMEPAGE));
			break;

        //case SDLK_t: // 输入测试页
        //    nbk_loadUrl(nbk, ini_getString(NEINI_TESTPAGE));
        //    break;

        case SDLK_y: // 内部测试
            nbk_func_test(nbk);
            break;

        case SDLK_1: // 适屏模式
		{
			if (   nbk->curZoom.i != nbk->minZoom.i
				|| nbk->curZoom.f != nbk->minZoom.f )
				nbk_changeZoom(nbk, nbk->minZoom);
			break;
		}
		case SDLK_2: // 100% 模式
		{
			NFloat zoom = {1, 0};
			nbk_changeZoom(nbk, zoom);
			break;
		}
		case SDLK_3: // 150% 模式
		{
			NFloat zoom = {1, 5000};
			nbk_changeZoom(nbk, zoom);
			break;
		}
		case SDLK_z: // 切换模式
			nbk_change_mode(nbk, N_TRUE);
			break;

        case SDLK_x: // 切换云模式
			nbk_change_mode(nbk, N_FALSE);
			break;

        case SDLK_r: // 刷新
            nav_refresh(nbk);
            break;

		case SDLK_LEFT:
		case SDLK_a:
		{
			if (   (evt.key.keysym.mod & KMOD_LALT)
				|| (evt.key.keysym.mod & KMOD_RALT) )
				nav_prev(nbk);
			else if (nbk->in4ways)
				nbk_move_left(nbk);
			else
				nbk_page_up(nbk);
			break;
		}
		case SDLK_RIGHT:
		case SDLK_d:
		{
			if (   (evt.key.keysym.mod & KMOD_LALT)
				|| (evt.key.keysym.mod & KMOD_RALT) )
                nav_next(nbk);
			else if (nbk->in4ways)
				nbk_move_right(nbk);
			else
				nbk_page_down(nbk);
			break;
		}
		case SDLK_UP:
		case SDLK_w:
		{
			if (nbk->in4ways)
				nbk_move_up(nbk);
			else
				nbk_focus_prev(nbk);
			break;
		}
		case SDLK_DOWN:
		case SDLK_s:
		{
			if (nbk->in4ways)
				nbk_move_down(nbk);
			else
				nbk_focus_next(nbk);
			break;
		}
		case SDLK_RETURN:
			if (!nbk->in4ways)
				nbk_focus_click(nbk);
			break;

        case SDLK_n: // 切换夜间模式
            nbk_switchNightTheme(nbk);
		    break;

        case SDLK_LEFTBRACKET: // 启动自由复制
            nbk_textSelBegin(nbk);
            break;

        case SDLK_RIGHTBRACKET: // 结束自由复制
            nbk_textSelEnd(nbk);
            break;
		}
        break;
	}

#ifdef PLATFORM_WEBOS
	case SDL_KEYDOWN:
		switch (evt.key.keysym.sym) {
		case PDLK_GESTURE_BACK:
			nav_prev(nbk);
			break;
		case PDLK_GESTURE_FORWARD:
			nav_next(nbk);
			break;
		}
		break;
#endif

	case SDL_MOUSEBUTTONDOWN:
	{
        if (evt.button.button == SDL_BUTTON_LEFT) {
            nbk->catchMotion = N_TRUE;
		    nbk->lastPenPos.x = evt.button.x;
		    nbk->lastPenPos.y = evt.button.y;
		    pt = nbk->lastPenPos;
            pt.x += nbk->viewRect.l;
            pt.y += nbk->viewRect.t;
		    nbk->penFocus = nbk_getFocusByPos(nbk, pt);
		    nbk_setFocus(nbk, nbk->penFocus);
		    nbk_drawPage(nbk, &nbk->viewRect);
        }
        else if (evt.button.button == SDL_BUTTON_WHEELUP) {
			if (nbk->in4ways)
				nbk_move_up(nbk);
			else
				nbk_page_up(nbk);
        }
        else if (evt.button.button == SDL_BUTTON_WHEELDOWN) {
			if (nbk->in4ways)
				nbk_move_down(nbk);
			else
				nbk_page_down(nbk);
        }
        else if (evt.button.button == SDL_BUTTON_RIGHT) {
            NPoint pt = {evt.button.x, evt.button.y};
            nbk_inspectByPos(nbk, &pt);
        }
		break;
	}

	case SDL_MOUSEBUTTONUP:
	{
        if (evt.button.button == SDL_BUTTON_LEFT) {
            nbk->catchMotion = N_FALSE;
		    pt.x = evt.button.x;
		    pt.y = evt.button.y;
            pt.x += nbk->viewRect.l;
            pt.y += nbk->viewRect.t;

            if (nbk->showMainBody && !nbk->inMainBodyMode) {
                int dx = evt.button.x - nbk->lastPenPos.x;
                int dy = evt.button.y - nbk->lastPenPos.y;
                if (   N_ABS(dx) < 5
                    && N_ABS(dy) < 5
                    && page_hasMainBody(GET_NBK_PAGE(nbk->history)) )
                {
                    NNode* body = history_getMainBodyByPos(nbk->history, pt.x, pt.y);
                    if (body) {
                        history_enterMainBodyMode(nbk->history, body);
                        break;
                    }
                }
            }

            if (nbk->penFocus) {
                if (nbk->penFocus == nbk_getFocusByPos(nbk, pt))
                    nbk_clickFocus(nbk, nbk->penFocus);
            }
		    nbk_drawPage(nbk, &nbk->viewRect);
        }
		break;
	}

	case SDL_MOUSEMOTION:
	{
        if (nbk->catchMotion && evt.motion.state == SDL_PRESSED) {
			coord dx, dy;
			dx = evt.motion.x - nbk->lastPenPos.x;
			dy = evt.motion.y - nbk->lastPenPos.y;
			if (N_ABS(dx) > PEN_MOVE_MIN || N_ABS(dy) > PEN_MOVE_MIN) {
				nbk->lastPenPos.x = evt.motion.x;
				nbk->lastPenPos.y = evt.motion.y;
				if (nbk->penFocus)
					nbk->penFocus = NULL;
				nbk_drag_by_pen(nbk, -dx, -dy);
			}
		}
		break;
	}
	}

	sync_post("Event");
}

NNode* nbk_getFocusByPos(NBK_core* nbk, NPoint pos)
{
	return doc_getFocusNodeByPos(GET_NBK_PAGE(nbk->history)->doc, pos.x - nbk->viewXOffset, pos.y);
}

void nbk_setFocus(NBK_core* nbk, NNode* focus)
{
	page_setFocusedNode(GET_NBK_PAGE(nbk->history), focus);
}

void nbk_clickFocus(NBK_core* nbk, NNode* focus)
{
	doc_clickFocusNode(GET_NBK_PAGE(nbk->history)->doc, focus);
}

void nbk_reposition(NBK_core* nbk, int x, int y)
{
	NSize lcd = gdi_getScreenSize(nbk->gdi);

    nbk->viewRect.l = x;
    nbk->viewRect.r = nbk->viewRect.l + lcd.w;
    nbk->viewRect.t = y;
    nbk->viewRect.b = nbk->viewRect.t + lcd.h;
}

void nbk_switchNightTheme(NBK_core* nbk)
{
    nbk->settings.night = (nbk->settings.night) ? N_NULL : &nbk->nightTheme;
    page_layout(GET_NBK_PAGE(nbk->history), N_TRUE);
    nbk_updateScreen(nbk);
}

void nbk_textSelBegin(NBK_core* nbk)
{
    NPage* page = GET_NBK_PAGE(nbk->history);
    NPoint pt = {10, 10};

    N_ASSERT(nbk->textSel == N_NULL);

    nbk->textSel = textSel_begin(page->view);

    //textSel_setStartPoint(nbk->textSel, &pt);
    textSel_useKey(nbk->textSel, &pt);
}

void nbk_textSelEnd(NBK_core* nbk)
{
    N_ASSERT(nbk->textSel);
    textSel_end(&nbk->textSel);
}

void nbk_inspectByPos(NBK_core* nbk, const NPoint* pos)
{
    NPage* page = GET_NBK_PAGE(nbk->history);
    NFloat zoom = history_getZoom(nbk->history);
    NPoint pt = {nbk->viewRect.l + pos->x, nbk->viewRect.t + pos->y};
    NRenderNode* rn;
    nbool absLayout;

    if (page->view->root == N_NULL)
        return;

    absLayout = doc_isAbsLayout(page->doc->type);

    fprintf(stderr, "\n======================================== INSPECT ========================================\n");
    fprintf(stderr, "window pos: %d , %d\n", pos->x, pos->y);
    fprintf(stderr, "  page pos: %d , %d\n", pt.x, pt.y);

    fprintf(stderr, " page size: %d x %d\t", page_width(page), page_height(page));
    fprintf(stderr, "     zoom: %d.%d\n", zoom.i, zoom.f);

    fprintf(stderr, "  doc size: %d x %d\t", rect_getWidth(&page->view->root->r), rect_getHeight(&page->view->root->r));
    fprintf(stderr, " overflow: %d x %d\n", view_width(page->view), view_height(page->view));

    fprintf(stderr, "\n");
    rn = view_findRenderByPos(page->view, &pt);
    if (rn) {
        NNode* n = (rn->type == RNT_TEXTPIECE) ? (NNode*)rn->parent->node : (NNode*)rn->node;
        fprintf(stderr, "tag: %s\n", xml_getTagNames()[n->id]);

        if (n->id == TAGID_TEXT) {
            if (absLayout)
                n = n->parent; // <span>
        }

        if (absLayout) {
            NRect* tc_rect = attr_getRect(n->atts, ATTID_TC_RECT);
            if (tc_rect)
                fprintf(stderr, "tc_rect: %d,%d,%d,%d\n", tc_rect->l, tc_rect->t, rect_getWidth(tc_rect), rect_getHeight(tc_rect));
        }

        if (n->id == TAGID_IMG) {
            NRenderImage* ri = (NRenderImage*)rn;
            fprintf(stderr, "src: %s\n", attr_getValueStr(n->atts, ATTID_SRC));
            if (ri->iid != -1 && ri->iid != IMAGE_STOCK_TYPE_ID) {
                NImage* im = (NImage*)page->view->imgPlayer->list->data[ri->iid];
                fprintf(stderr, "id: %d  ", ri->iid);
                fprintf(stderr, "status: %d  ", im->state);
                fprintf(stderr, "size: %d x %d\n", im->size.w, im->size.h);
            }
        }
    }
}

void nav_prev(NBK_core* nbk)
{
    picMgr_stopAllDecoding(nbk->picMgr);
	history_prev(nbk->history);
}

void nav_next(NBK_core* nbk)
{
    picMgr_stopAllDecoding(nbk->picMgr);
	history_next(nbk->history);
}

void nav_refresh(NBK_core* nbk)
{
    picMgr_stopAllDecoding(nbk->picMgr);
    page_refresh(GET_NBK_PAGE(nbk->history));
}
