#ifndef __NBK_PIC_H__
#define __NBK_PIC_H__

#include "../stdc/inc/config.h"
#include "../stdc/inc/nbk_gdi.h"
#include "../stdc/render/imagePlayer.h"
#include <SDL.h>
#include "nbkgdi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NPic {

	NImage*	d;

	uint8*	data;
	int		len;

	float	zoom;
    nbool    stop;

	SDL_Surface*	bmp;
	SDL_Surface*	bmpStretch;
	SDL_Thread*		worker;

	NImagePlayer*	player;
	void*			mgr;

} NPic;

NPic* pic_create(NImage* image);
void pic_delete(NPic** pic);

void pic_onData(NPic* pic, void* data, int len);
void pic_onDataEnd(NPic* pic, nbool succeed);

void pic_decode(NPic* pic);
void pic_cancel(NPic* pic);

void pic_draw(NPic* pic, NBK_gdi* gdi, const NRect* rect);
void pic_drawAsBackground(NPic* pic, NBK_gdi* gdi, const NRect* rect, int rx, int ry);

#ifdef __cplusplus
}
#endif

#endif
