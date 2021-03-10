#ifndef __NBK_GDI_IMPL_H__
#define __NBK_GDI_IMPL_H__

#include "../stdc/inc/config.h"
#include "../stdc/inc/nbk_gdi.h"
#include <SDL.h>
#include <SDL_ttf.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FONTS	32

NFloat float_to_NFloat(float f);
float NFloat_to_float(NFloat nf);

typedef struct _NBK_font {
	TTF_Font*	font;
	int			pixel;
	int			ascent;
	int			descent;
	int			height;
	NFloat		zoom;
	nbool		bold;
	nbool		italic;
} NBK_font;

typedef struct _NBK_fontMgr {
	NBK_font	lst[MAX_FONTS];
	NBK_font	zoomLst[MAX_FONTS];
	int			use;
} NBK_fontMgr;

typedef struct _NBK_gdi {

	SDL_Surface*	screen; // 主屏

	NBK_fontMgr*	fontMgr;

	nbool	update; // 是否存在图形更新
    nbool    fontNoZoom; // 是否使用缩放的字体

	Uint32	fg;
	Uint32	bg;
	NColor	n_fg;
	NColor	n_bg;

	int		fontId;

	NPoint	drawOffset;

} NBK_gdi;

NBK_gdi* gdi_create(void);
void gdi_delete(NBK_gdi** gdi);

void gdi_resetFont(NBK_gdi* gdi);

NSize gdi_getScreenSize(NBK_gdi* gdi);

void gdi_setDrawOffset(NBK_gdi* gdi, const NPoint offset);
NPoint gdi_getDrawOffset(NBK_gdi* gdi);

void gdi_setBrushColor(NBK_gdi* gdi, const NColor* color);

void gdi_clearRect(NBK_gdi* gdi, const NRect* rect);
void gdi_copyRect(NBK_gdi* gdi, NPoint pos, NRect rect);

void gdi_drawText(NBK_gdi* gdi, const wchr* text, int length, const NPoint* pos);
void gdi_drawBitmap(NBK_gdi* gdi, SDL_Surface* bmp, const NRect* rect);

#ifdef __cplusplus
}
#endif

#endif
