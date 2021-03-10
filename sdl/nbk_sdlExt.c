#include "nbk_sdlExt.h"

void nse_getPixelRGBA(SDL_Surface* dst, Sint16 x, Sint16 y, NColor* color)
{
    Uint32 pixel = 0xffffffff;

    switch (dst->format->BytesPerPixel) {
    case 1:
        pixel = *((Uint8*)dst->pixels + y * dst->pitch + x);
        break;
    case 2:
        pixel = *((Uint16*)dst->pixels + y * dst->pitch / 2 + x);
        break;
    case 3:
        pixel = *((Uint8*)dst->pixels + y * dst->pitch + x * 3);
        break;
    case 4:
        pixel = *((Uint32*)dst->pixels + y * dst->pitch / 4 + x);
        break;
    }

    SDL_GetRGBA(pixel, dst->format, &color->r, &color->g, &color->b, &color->a);
}

void nse_copySurface(SDL_Surface* src, SDL_Rect* srcrect, SDL_Surface* dst, SDL_Rect* dstrect)
{
    int i, j;
    int x1, y1, x2, y2;

    if (   src->format->BytesPerPixel != dst->format->BytesPerPixel
        || srcrect->w != dstrect->w
        || srcrect->h != dstrect->h )
        return;

    if (src->format->palette) {
        memcpy(dst->format->palette->colors, src->format->palette->colors, sizeof(SDL_Color) * src->format->palette->ncolors);
    }

    for (i=0; i < srcrect->h; i++) { // for row
        y1 = srcrect->y + i;
        y2 = dstrect->y + i;
        for (j=0; j < srcrect->w; j++) { // for col
            x1 = srcrect->x + j;
            x2 = dstrect->x + j;
            switch (src->format->BytesPerPixel) {
            case 1:
                *((Uint8*)dst->pixels + y2 * dst->pitch + x2) = *((Uint8*)src->pixels + y1 * src->pitch + x1);
                break;
            case 2:
                *((Uint16*)dst->pixels + y2 * dst->pitch / 2 + x2) = *((Uint16*)src->pixels + y1 * src->pitch / 2 + x1);
                break;
            case 4:
                *((Uint32*)dst->pixels + y2 * dst->pitch / 4 + x2) = *((Uint32*)src->pixels + y1 * src->pitch / 4 + x1);
                break;
            }
        }
    }
}
