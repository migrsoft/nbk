// create by wuyulun, 2011.12.24

#include "nbk_conf.h"
#include "../stdc/inc/nbk_gdi.h"
#include "../stdc/tools/str.h"
#include "nbkgdi.h"
#include "nbk.h"
#include "SDL_gfxPrimitives.h"
#include "ini.h"

//////////////////////////////////////////////////
//
// 工具函数
//

NFloat float_to_NFloat(float f)
{
    NFloat nf;
    nf.i = (int)f;
    nf.f = (int)((f - (int)f) * NFLOAT_EMU);
    return nf;
}

float NFloat_to_float(NFloat nf)
{
    float f = nf.i;
    f += (float)nf.f / NFLOAT_EMU;
    return f;
}

////////////////////////////////////////////////////////////
//
// 字体管理器
//

static NBK_fontMgr* fontMgr_create(void)
{
	NBK_fontMgr* mgr = (NBK_fontMgr*)NBK_malloc0(sizeof(NBK_fontMgr));
	return mgr;
}

static void fontMgr_reset(NBK_fontMgr* mgr)
{
    int i;
	for (i=0; i < mgr->use; i++) {
		if (mgr->lst[i].font)
			TTF_CloseFont(mgr->lst[i].font);
	}
    mgr->use = 0;
}

static void fontMgr_delete(NBK_fontMgr** fontMgr)
{
	int i;
	NBK_fontMgr* mgr = *fontMgr;
    fontMgr_reset(mgr);
	NBK_free(mgr);
	*fontMgr = N_NULL;
}

// 获取指定的字体，当不存在时，创建该字体
static int fontMgr_getFont(NBK_fontMgr* mgr, int pixel, nbool bold, nbool italic)
{
	int i;
	TTF_Font* font;

	for (i=1; i < mgr->use; i++) {
		if (   mgr->lst[i].pixel  == pixel
			&& mgr->lst[i].bold   == bold
			&& mgr->lst[i].italic == italic )
			return i;
	}

	if (i == MAX_FONTS)
		return 1;

	font = TTF_OpenFont(ini_getString(NEINI_TTF_FONT), pixel);
	if (font) {
        int style = TTF_STYLE_NORMAL;
        if (bold)   style |= TTF_STYLE_BOLD;
        if (italic) style |= TTF_STYLE_ITALIC;
        TTF_SetFontStyle(font, style);

		//fprintf(stderr, "create font: %d pixel bold: %d italic %d\n", pixel, bold, italic);
		mgr->lst[i].font    = font;
		mgr->lst[i].pixel   = pixel;
		mgr->lst[i].ascent  = TTF_FontAscent(font);
		mgr->lst[i].descent = TTF_FontDescent(font);
		mgr->lst[i].height  = TTF_FontHeight(font);
		mgr->lst[i].bold    = bold;
		mgr->lst[i].italic  = italic;
		mgr->use = i + 1;
	}

	return i;
}

// 根据分配的字体号获取字体，缩放时，返回缩放字体
static TTF_Font* fontMgr_getFontById(NBK_fontMgr* mgr, int fontId, NFloat zoom, int* pixel)
{
	TTF_Font* font = N_NULL;

	if (fontId < 1 || fontId >= mgr->use)
		return N_NULL;

	if (zoom.i == 1 && zoom.f == 0) {
		font = mgr->lst[fontId].font;
		*pixel = mgr->lst[fontId].pixel;
	}
	else {
		if (   mgr->zoomLst[fontId].zoom.i == zoom.i
			&& mgr->zoomLst[fontId].zoom.f == zoom.f ) {
				font = mgr->zoomLst[fontId].font;
				*pixel = mgr->zoomLst[fontId].pixel;
		}
		else {
			int px = nfloat_imul(mgr->lst[fontId].pixel, zoom);
			*pixel = px;

			if (px < 8) // 最小字号不低于指定像素
				return N_NULL;

			if (mgr->zoomLst[fontId].font)
				TTF_CloseFont(mgr->zoomLst[fontId].font);
			NBK_memset(&mgr->zoomLst[fontId], 0, sizeof(NBK_font));

			font = TTF_OpenFont(ini_getString(NEINI_TTF_FONT), px);
			if (font) {
                int style = TTF_STYLE_NORMAL;
                if (mgr->lst[fontId].bold)   style |= TTF_STYLE_BOLD;
                if (mgr->lst[fontId].italic) style |= TTF_STYLE_ITALIC;
                TTF_SetFontStyle(font, style);

				mgr->zoomLst[fontId].font       = font;
				mgr->zoomLst[fontId].pixel      = px;
				mgr->zoomLst[fontId].ascent     = TTF_FontAscent(font);
				mgr->zoomLst[fontId].descent    = TTF_FontDescent(font);
				mgr->zoomLst[fontId].height     = TTF_FontHeight(font);
				mgr->zoomLst[fontId].bold       = mgr->lst[fontId].bold;
				mgr->zoomLst[fontId].italic     = mgr->lst[fontId].italic;
				mgr->zoomLst[fontId].zoom       = zoom;
			}
		}
	}

	return font;
}

////////////////////////////////////////////////////////////
//
// 绘图实现
//

NBK_gdi* gdi_create(void)
{
	NBK_gdi* gdi = (NBK_gdi*)NBK_malloc0(sizeof(NBK_gdi));
	gdi->fontMgr = fontMgr_create();
	return gdi;
}

void gdi_delete(NBK_gdi** gdi)
{
	NBK_gdi* g = *gdi;
	fontMgr_delete(&g->fontMgr);
	NBK_free(g);
	*gdi = N_NULL;
}

void gdi_resetFont(NBK_gdi* gdi)
{
    fontMgr_reset(gdi->fontMgr);
}

// NBK gdi interface

void NBK_gdi_setPenColor(void* pfd, const NColor* color)
{
	NBK_core* nbk = (NBK_core*)pfd;
	nbk->gdi->fg = SDL_MapRGBA(nbk->gdi->screen->format, color->r, color->g, color->b, color->a);
	nbk->gdi->n_fg = *color;
}

void NBK_gdi_setBrushColor(void* pfd, const NColor* color)
{
	NBK_core* nbk = (NBK_core*)pfd;
    gdi_setBrushColor(nbk->gdi, color);
}

void NBK_gdi_drawText(void* pfd, const wchr* text, int length, const NPoint* pos, const int decor)
{
	NBK_core* nbk = (NBK_core*)pfd;
	TTF_Font* font;
	SDL_Surface* render;
	SDL_Color c;
	wchr *t, save;
	int pixel;
    NFloat zoom = {1, 0};

    if (!nbk->gdi->fontNoZoom)
        zoom = nbk->curZoom;

	font = fontMgr_getFontById(nbk->gdi->fontMgr, nbk->gdi->fontId, zoom, &pixel);

	if (font == N_NULL) {
		coord w = NBK_gdi_getTextWidth(pfd, nbk->gdi->fontId, text, length);
		coord h = NBK_gdi_getFontHeight(pfd, nbk->gdi->fontId);
		NRect r;
		Uint32 color = nbk->gdi->bg;
		rect_set(&r, pos->x, pos->y, w, h);
		rect_setHeight(&r, 2);
		nbk->gdi->bg = nbk->gdi->fg;
		NBK_gdi_clearRect(pfd, &r);
		nbk->gdi->bg = color;
		return;
	}

	if (length > 0) {
		t = (wchr*)text;
		save = t[length];
		t[length] = 0L;
	}

	c.r = nbk->gdi->n_fg.r;
	c.g = nbk->gdi->n_fg.g;
	c.b = nbk->gdi->n_fg.b;

	render = TTF_RenderUNICODE_Blended(font, text, c);

	if (length > 0)
		t[length] = save;

	if (render) {
		SDL_Rect src, dst;

		src.x = src.y = 0;
		src.w = render->w;
		src.h = render->h;

		dst.x = pos->x + nbk->gdi->drawOffset.x;
		dst.y = pos->y + nbk->gdi->drawOffset.y - (render->h - pixel) / 2; // 去除行距
		dst.w = src.w;
		dst.h = src.h;

		SDL_BlitSurface(render, &src, nbk->gdi->screen, &dst);
		SDL_FreeSurface(render);

		if (decor) {
			hlineRGBA(nbk->gdi->screen, dst.x, dst.x + dst.w,
				pos->y + nbk->gdi->drawOffset.y + dst.h - 1,
				nbk->gdi->n_fg.r, nbk->gdi->n_fg.g, nbk->gdi->n_fg.b, nbk->gdi->n_fg.a);
		}

		nbk->gdi->update = N_TRUE;
	}
}

void gdi_drawText(NBK_gdi* gdi, const wchr* text, int length, const NPoint* pos)
{
	TTF_Font* font = gdi->fontMgr->lst[gdi->fontId].font;
	SDL_Color c;
	SDL_Surface* render;

	c.r = gdi->n_fg.r;
	c.g = gdi->n_fg.g;
	c.b = gdi->n_fg.b;

	render = TTF_RenderUNICODE_Blended(font, text, c);

	if (render) {
		SDL_Rect src, dst;

		src.x = src.y = 0;
		src.w = render->w;
		src.h = render->h;

		dst.x = pos->x;
		dst.y = pos->y;
		dst.w = src.w;
		dst.h = src.h;

		SDL_BlitSurface(render, &src, gdi->screen, &dst);
		SDL_FreeSurface(render);

		gdi->update = N_TRUE;
	}
}

void NBK_gdi_drawRect(void* pfd, const NRect* rect)
{
	NBK_core* nbk = (NBK_core*)pfd;
	SDL_Rect r;

	// 左边
	r.x = rect->l + nbk->gdi->drawOffset.x;
	r.y = rect->t + nbk->gdi->drawOffset.y;
	r.w = 1;
	r.h = rect_getHeight(rect);
	SDL_FillRect(nbk->gdi->screen, &r, nbk->gdi->fg);

	// 顶边
	r.x = rect->l + nbk->gdi->drawOffset.x;
	r.y = rect->t + nbk->gdi->drawOffset.y;
	r.w = rect_getWidth(rect);
	r.h = 1;
	SDL_FillRect(nbk->gdi->screen, &r, nbk->gdi->fg);

	// 右边
	r.x = rect->r - 1 + nbk->gdi->drawOffset.x;
	r.y = rect->t + nbk->gdi->drawOffset.y;
	r.w = 1;
	r.h = rect_getHeight(rect);
	SDL_FillRect(nbk->gdi->screen, &r, nbk->gdi->fg);

	// 底边
	r.x = rect->l + nbk->gdi->drawOffset.x;
	r.y = rect->b - 1 + nbk->gdi->drawOffset.y;
	r.w = rect_getWidth(rect);
	r.h = 1;
	SDL_FillRect(nbk->gdi->screen, &r, nbk->gdi->fg);

	nbk->gdi->update = N_TRUE;
}

void NBK_gdi_drawCircle(void* pfd, const NRect* rect)
{
	NBK_core* nbk = (NBK_core*)pfd;
	coord x = rect->l + rect_getWidth(rect) / 2 + nbk->gdi->drawOffset.x;
	coord y = rect->t + rect_getHeight(rect) / 2 + nbk->gdi->drawOffset.y;
	coord rad = rect_getWidth(rect) / 2;
	rad = N_MIN(rad, rect_getHeight(rect) / 2) - 1;
	circleRGBA(nbk->gdi->screen, x, y, rad, nbk->gdi->n_fg.r, nbk->gdi->n_fg.g, nbk->gdi->n_fg.b, nbk->gdi->n_fg.a);
	nbk->gdi->update = N_TRUE;
}

void NBK_gdi_clearRect(void* pfd, const NRect* rect)
{
	NBK_core* nbk = (NBK_core*)pfd;
    gdi_clearRect(nbk->gdi, rect);
}

/* font metrics */

static coord font_get_text_width(NBK_core* nbk, NFontId id, const wchr* text, int length, nbool zoom)
{
	TTF_Font* font;
	coord w = 0;
    int pixel;

	if (zoom) {
		font = fontMgr_getFontById(nbk->gdi->fontMgr, id, nbk->curZoom, &pixel);
	}
	else {
		font  = nbk->gdi->fontMgr->lst[id].font;
        pixel = nbk->gdi->fontMgr->lst[id].pixel;
	}

	if (font) {
		wchr *t, save;
		int fw, fh;

		if (length > 0) {
			t = (wchr*)text;
			save = t[length];
			t[length] = 0;
		}

		if (TTF_SizeUNICODE(font, text, &fw, &fh) == 0)
			w = fw;

		if (length > 0)
			t[length] = save;
	}
	else { // 缩略模式，无有效字体。假定字符宽2像素
		if (length > 0)
			w = 2 * length;
		else
			w = 2 * nbk_wcslen(text);
	}

	return w;
}

static coord font_get_char_width(NBK_core* nbk, NFontId id, const wchr ch, nbool zoom)
{
	wchr t[2];
	t[0] = ch;
	t[1] = 0L;
	return font_get_text_width(nbk, id, t, -1, zoom);
}

static coord font_get_font_height(NBK_core* nbk, NFontId id, nbool zoom)
{
	TTF_Font* font;
	int height = 2;

	if (zoom) {
		int pixel;
		font = fontMgr_getFontById(nbk->gdi->fontMgr, id, nbk->curZoom, &pixel);
	}
	else {
		font = nbk->gdi->fontMgr->lst[id].font;
	}

	if (font) {
		NBK_font* lst = (zoom) ? nbk->gdi->fontMgr->zoomLst : nbk->gdi->fontMgr->lst;
		height = lst[id].height;
	}

	return height;
}

coord NBK_gdi_getCharWidth(void* pfd, NFontId id, const wchr ch)
{
	return font_get_char_width((NBK_core*)pfd, id, ch, N_FALSE);
}

coord NBK_gdi_getCharWidthByEditor(void* pfd, NFontId id, const wchr ch)
{
	NBK_core* nbk = (NBK_core*)pfd;
	nbool zoom = (nbk->curZoom.i == 1 && nbk->curZoom.f == 0) ? N_FALSE : N_TRUE;
	return font_get_char_width(nbk, id, ch, zoom);
}

// 提供文本宽度。不关心缩放
coord NBK_gdi_getTextWidth(void* pfd, NFontId id, const wchr* text, int length)
{
	return font_get_text_width((NBK_core*)pfd, id, text, length, N_FALSE);
}

// 提供编辑器文本宽度。当缩放时，该宽度为缩放字体的准确值。
coord NBK_gdi_getTextWidthByEditor(void* pfd, NFontId id, const wchr* text, int length)
{
	NBK_core* nbk = (NBK_core*)pfd;
	nbool zoom = (nbk->curZoom.i == 1 && nbk->curZoom.f == 0) ? N_FALSE : N_TRUE;
	return font_get_text_width(nbk, id, text, length, zoom);
}

coord NBK_gdi_getFontHeight(void* pfd, NFontId id)
{
	return font_get_font_height((NBK_core*)pfd, id, N_FALSE);
}

coord NBK_gdi_getFontHeightByEditor(void* pfd, NFontId id)
{
	NBK_core* nbk = (NBK_core*)pfd;
	nbool zoom = (nbk->curZoom.i == 1 && nbk->curZoom.f == 0) ? N_FALSE : N_TRUE;
	return font_get_font_height(nbk, id, zoom);
}

NFontId NBK_gdi_getFont(void* pfd, int16 pixel, nbool bold, nbool italic)
{
	NBK_core* nbk = (NBK_core*)pfd;
	return (NFontId)fontMgr_getFont(nbk->gdi->fontMgr, pixel, bold, italic);
}

void NBK_gdi_useFont(void* pfd, NFontId id)
{
	NBK_core* nbk = (NBK_core*)pfd;
	int pixel;
	nbk->gdi->fontId = id;
    nbk->gdi->fontNoZoom = N_FALSE;
	fontMgr_getFontById(nbk->gdi->fontMgr, id, nbk->curZoom, &pixel);
}

void NBK_gdi_useFontNoZoom(void* pfd, NFontId id)
{
	NBK_core* nbk = (NBK_core*)pfd;
	int pixel;
	nbk->gdi->fontId = id;
    nbk->gdi->fontNoZoom = N_TRUE;
}

void NBK_gdi_releaseFont(void* pfd)
{
}

//coord NBK_gdi_getFontAscent(void* pfd, NFontId id)
//{
//	int ascent = 0;
//	NBK_core* nbk = (NBK_core*)pfd;
//
//	//if (id > 0 && id < nbk->gdi->fontMgr->use)
//	//	ascent = nbk->gdi->fontMgr->lst[id].ascent;
//
//	return ascent;
//}
//
//coord NBK_gdi_getFontDescent(void* pfd, NFontId id)
//{
//    return 0;
//}

void NBK_gdi_setClippingRect(void* pfd, const NRect* rect)
{
	NBK_core* nbk = (NBK_core*)pfd;
	SDL_Rect r;

	r.x = rect->l + nbk->gdi->drawOffset.x;
	r.y = rect->t + nbk->gdi->drawOffset.y;
	r.w = rect_getWidth(rect);
	r.h = rect_getHeight(rect);

	SDL_SetClipRect(nbk->gdi->screen, &r);
}

void NBK_gdi_cancelClippingRect(void* pfd)
{
	NBK_core* nbk = (NBK_core*)pfd;
	SDL_SetClipRect(nbk->gdi->screen, NULL);
}

void NBK_helper_getViewableRect(void* pfd, NRect* view)
{
	NBK_core* nbk = (NBK_core*)pfd;
	view->l = nbk->viewRect.l;
	view->t = nbk->viewRect.t;
	view->r = view->l + nbk->gdi->screen->w;
	view->b = view->t + nbk->gdi->screen->h;
}

void NBK_gdi_drawEditorCursor(void* pfd, NPoint* pos, coord xOffset, coord cursorHeight)
{
	NBK_core* nbk = (NBK_core*)pfd;
	NRect rect;
	rect.l = pos->x + nbk->gdi->drawOffset.x + xOffset;
	rect.t = pos->y + nbk->gdi->drawOffset.y;
	rect.r = rect.l + 2;
	rect.b = rect.t + cursorHeight;
	NBK_gdi_clearRect(pfd, &rect);
}

void NBK_gdi_drawEditorScrollbar(void* pfd, NPoint* pos, coord xOffset, coord yOffset, NSize* size)
{
	NBK_core* nbk = (NBK_core*)pfd;
	NRect rect;
	rect.l = pos->x + nbk->gdi->drawOffset.x + xOffset;
	rect.t = pos->y + nbk->gdi->drawOffset.y + xOffset;
	rect.r = rect.l + size->w;
	rect.b = rect.t + size->h;
	NBK_gdi_clearRect(pfd, &rect);
}

void NBK_gdi_drawEditorText(void* pfd, const wchr* text, int length, const NPoint* pos, coord xOffset)
{
	NPoint newPos = *pos;
	newPos.x += xOffset;
	NBK_gdi_drawText(pfd, text, length, &newPos, 0);
}

NSize gdi_getScreenSize(NBK_gdi* gdi)
{
	NSize sz;
	sz.w = gdi->screen->w;
	sz.h = gdi->screen->h;
	return sz;
}

void gdi_setDrawOffset(NBK_gdi* gdi, const NPoint offset)
{
	gdi->drawOffset = offset;
}

NPoint gdi_getDrawOffset(NBK_gdi* gdi)
{
	return gdi->drawOffset;
}

void gdi_copyRect(NBK_gdi* gdi, NPoint pos, NRect rect)
{
}

void gdi_drawBitmap(NBK_gdi* gdi, SDL_Surface* bmp, const NRect* rect)
{
	SDL_Rect src, dst;

	src.x = src.y = 0;
	src.w = bmp->w;
	src.h = bmp->h;

	dst.x = rect->l + gdi->drawOffset.x;
	dst.y = rect->t + gdi->drawOffset.y;
	dst.w = rect_getWidth(rect);
	dst.h = rect_getHeight(rect);

	SDL_BlitSurface(bmp, &src, gdi->screen, &dst);
	gdi->update = N_TRUE;
}

void gdi_setBrushColor(NBK_gdi* gdi, const NColor* color)
{
	gdi->bg = SDL_MapRGBA(gdi->screen->format, color->r, color->g, color->b, color->a);
	gdi->n_bg = *color;
}

void gdi_clearRect(NBK_gdi* gdi, const NRect* rect)
{
	SDL_Rect dst;

	dst.x = rect->l + gdi->drawOffset.x;
	dst.y = rect->t + gdi->drawOffset.y;
	dst.w = rect_getWidth(rect);
	dst.h = rect_getHeight(rect);

	if (gdi->n_bg.a < 255) {
		int16 x1 = rect->l + gdi->drawOffset.x;
		int16 y1 = rect->t + gdi->drawOffset.y;
		int16 x2 = x1 + rect_getWidth(rect) - 1;
		int16 y2 = y1 + rect_getHeight(rect) - 1;

		boxRGBA(gdi->screen, x1, y1, x2, y2, gdi->n_bg.r, gdi->n_bg.g, gdi->n_bg.b, gdi->n_bg.a);
	}
	else {
		SDL_FillRect(gdi->screen, &dst, gdi->bg);
	}

	gdi->update = N_TRUE;
}
