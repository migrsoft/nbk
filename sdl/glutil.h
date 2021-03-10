/*
 * glutil.h
 *
 *  Created on: 2012-4-29
 *      Author: wuyulun
 */

#ifndef GLUTIL_H_
#define GLUTIL_H_

#include <SDL.h>
#include <SDL_opengles.h>

#ifdef __cplusplus
extern "C" {
#endif

SDL_Surface* gl_init(void);
void gl_end(void);

void gl_drawSurfaceAsTexture(SDL_Surface* s);

#ifdef __cplusplus
}
#endif

#endif /* GLUTIL_H_ */
