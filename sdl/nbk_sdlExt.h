// Create by wuyulun, 2012.3.22

#ifndef _NBK_SDLEXT_H_
#define _NBK_SDLEXT_H_

#include <SDL.h>
#include "../stdc/inc/nbk_gdi.h"

#ifdef __cplusplus
extern "C" {
#endif

// 获取像素颜色
void nse_getPixelRGBA(SDL_Surface* dst, Sint16 x, Sint16 y, NColor* color);

// 复制
void nse_copySurface(SDL_Surface* src, SDL_Rect* srcrect, SDL_Surface* dst, SDL_Rect* dstrect);

#ifdef __cplusplus
}
#endif

#endif
