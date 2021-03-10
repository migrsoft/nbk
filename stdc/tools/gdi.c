/*
 * nbk_gdi.c
 *
 *  Created on: 2011-1-7
 *      Author: wuyulun
 */

#include "../inc/nbk_gdi.h"

int32 float_imul(int32 src, float factor)
{
#if NBK_DISABLE_ZOOM
    return src;
#else
    return (int32)(src * factor + 0.5f);
#endif
}

int32 float_idiv(int32 src, float factor)
{
#if NBK_DISABLE_ZOOM
    return src;
#else
    return (int32)(src / factor + 0.5f);
#endif
}

coord rect_getWidth(const NRect* rect)
{
    return rect->r - rect->l;
}

coord rect_getHeight(const NRect* rect)
{
    return rect->b - rect->t;
}

void rect_setWidth(NRect* rect, coord width)
{
    rect->r = rect->l + width;
}

void rect_setHeight(NRect* rect, coord height)
{
    rect->b = rect->t + height;
}

void rect_unite(NRect* dst, NRect* src)
{
    dst->l = N_MIN(dst->l, src->l);
    dst->t = N_MIN(dst->t, src->t);
    dst->r = N_MAX(dst->r, src->r);
    dst->b = N_MAX(dst->b, src->b);
}

void rect_intersect(NRect* dst, const NRect* src)
{
    coord l, t, r, b;
    
    l = N_MAX(dst->l, src->l);
    t = N_MAX(dst->t, src->t);
    r = N_MIN(dst->r, src->r);
    b = N_MIN(dst->b, src->b);
    
    dst->l = l;
    dst->t = t;
    dst->r = r;
    dst->b = b;
    
    if (dst->l > dst->r)
        dst->r = dst->l;
    if (dst->t > dst->b)
        dst->b = dst->t;
}

nbool rect_isIntersect(const NRect* dst, const NRect* src)
{
    coord l, t, r, b;
    
    l = N_MAX(dst->l, src->l);
    t = N_MAX(dst->t, src->t);
    r = N_MIN(dst->r, src->r);
    b = N_MIN(dst->b, src->b);
    
    return (l > r || t > b) ? N_FALSE : N_TRUE;
}

void rect_move(NRect* rect, coord x, coord y)
{
    coord w = rect->r - rect->l;
    coord h = rect->b - rect->t;
    rect->l = x;
    rect->r = x + w;
    rect->t = y;
    rect->b = y + h;
}

void rect_grow(NRect* rect, coord dx, coord dy)
{
    rect->l -= dx;
    rect->r += dx;
    rect->t -= dy;
    rect->b += dy;
}

nbool rect_hasPt(NRect* dst, coord x, coord y)
{
    if (x >= dst->l && x < dst->r && y >= dst->t && y < dst->b)
        return N_TRUE;
    else
        return N_FALSE;
}

nbool rect_isContain(const NRect* dst, const NRect* src)
{
    if (   src->l >= dst->l && src->t >= dst->t
        && src->r <= dst->r && src->b <= dst->b)
        return N_TRUE;
    else
        return N_FALSE;
}

nbool rect_isEmpty(const NRect* rect)
{
    return ((rect->l == 0 && rect->t == 0 && rect->r == 0 && rect->b == 0) ? N_TRUE : N_FALSE);
}

nbool rect_equals(const NRect* r1, const NRect* r2)
{
    if (   r1->l == r2->l
        && r1->t == r2->t
        && r1->r == r2->r
        && r1->b == r2->b )
        return N_TRUE;
    else
        return N_FALSE;
}

nbool rect_isEquaTopLeft(const NRect* r1, const NRect* r2)
{
    if (   r1->l == r2->l
        && r1->t == r2->t )
        return N_TRUE;
    else
        return N_FALSE;
}

void rect_toView(NRect* rect, float factor)
{
#if !NBK_DISABLE_ZOOM
    rect->l = (coord)(rect->l * factor + 0.5f);
    rect->t = (coord)(rect->t * factor + 0.5f);
    rect->r = (coord)(rect->r * factor + 0.5f);
    rect->b = (coord)(rect->b * factor + 0.5f);
#endif
}

void rect_toDoc(NRect* rect, float factor)
{
#if !NBK_DISABLE_ZOOM
    rect->l = (coord)(rect->l / factor + 0.5f);
    rect->t = (coord)(rect->t / factor + 0.5f);
    rect->r = (coord)(rect->r / factor + 0.5f);
    rect->b = (coord)(rect->b / factor + 0.5f);
#endif
}

void rect_set(NRect* rect, coord x, coord y, coord w, coord h)
{
    rect->l = x;
    rect->t = y;
    rect->r = rect->l + w;
    rect->b = rect->t + h;
}

void color_set(NColor* color, uint8 r, uint8 g, uint8 b, uint8 a)
{
    color->r = r;
    color->g = g;
    color->b = b;
    color->a = a;
}

nbool color_equals(const NColor* c1, const NColor* c2)
{
	if (   c1->r == c2->r
		&& c1->g == c2->g
		&& c1->b == c2->b
		&& c1->a == c2->a )
		return N_TRUE;
	else
		return N_FALSE;
}

void point_set(NPoint* point, coord x, coord y)
{
    point->x = x;
    point->y = y;
}

void point_toView(NPoint* point, float zoom)
{
    point->x = (coord)(point->x * zoom + 0.5f);
    point->y = (coord)(point->y * zoom + 0.5f);
}
