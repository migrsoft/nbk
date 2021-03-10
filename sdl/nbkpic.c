// create by wuyulun, 2012.1.22

#include "../stdc/css/css_val.h"
#include "../stdc/css/css_helper.h"
#include <SDL_image.h>
#include <SDL_rwops.h>
#include "nbkpic.h"
#include "picmgr.h"
#include "SDL_rotozoom.h"
#include "SDL_gfxBlitFunc.h"
#include "ini.h"
#include "resmgr.h"
#include "nbk_sdlExt.h"

//#define DEBUG_STRETCH

static void pic_reset(NPic* pic)
{
	if (pic->data) {
		NBK_free(pic->data);
		pic->data = N_NULL;
	}
	pic->len = 0;
}

NPic* pic_create(NImage* image)
{
	NPic* p = (NPic*)NBK_malloc0(sizeof(NPic));
	p->d = image;
	p->zoom = 1.0;
	return p;
}

void pic_delete(NPic** pic)
{
	NPic* p = *pic;

    p->stop = N_TRUE;

    if (p->worker) {
        //printf(" wait thread %d\n", SDL_GetThreadID(p->worker));
        SDL_WaitThread(p->worker, NULL);
    }

	pic_reset(p);

	if (p->bmp)
		SDL_FreeSurface(p->bmp);
	if (p->bmpStretch)
		SDL_FreeSurface(p->bmpStretch);

    if (p->d->stis) {
        NImStretchInfo* si = p->d->sti;
        if (si->user)
            SDL_FreeSurface((SDL_Surface*)si->user);
        si = (NImStretchInfo*)sll_first(p->d->stis);
        while (si) {
            if (si->user)
                SDL_FreeSurface((SDL_Surface*)si->user);
            si = (NImStretchInfo*)sll_next(p->d->stis);
        }
    }

	NBK_free(p);
	*pic = N_NULL;
}

void pic_onData(NPic* pic, void* data, int len)
{
	if (pic->len == 0) {
		pic->len = len;
		pic->data = (uint8*)NBK_malloc(pic->len);
		NBK_memcpy(pic->data, data, len);
	}
	else {
		int pos = pic->len;
		pic->len += len;
		pic->data = (uint8*)NBK_realloc(pic->data, pic->len);
		NBK_memcpy(pic->data + pos, data, len);
	}
}

void pic_onDataEnd(NPic* pic, nbool succeed)
{
	if (!succeed) {
		if (pic->data) {
			NBK_free(pic->data);
			pic->data = N_NULL;
			pic->len = 0;
		}
	}
    else {
        if (ini_getInt(NEINI_DUMP_IMG)) {
	        char path[MAX_CACHE_PATH_LEN];
            sprintf(path, "%simg%03d.png", ini_getString(NEINI_DATA_PATH), pic->d->id);
	        dump_file_init(path, N_FALSE);
            dump_file_data(path, pic->data, pic->len);
        }
	}
}

// 从原图中切出指定区域
static SDL_Surface* image_crop(SDL_Surface* base, SDL_Rect dst)
{
    SDL_Rect r;
    SDL_Surface* bmp = SDL_CreateRGBSurface(SDL_SWSURFACE, dst.w, dst.h, base->format->BitsPerPixel,
		base->format->Rmask, base->format->Gmask, base->format->Bmask, base->format->Amask);

    if (bmp == N_NULL)
        return bmp;

    r.x = r.y = 0;
    r.w = dst.w;
    r.h = dst.h;
    nse_copySurface(base, &dst, bmp, &r);

    return bmp;
}

static SDL_Surface* image_stretch(SDL_Surface* base, coord w, coord h)
{
    double zoom_x = (double)w / base->w;
    double zoom_y = (double)h / base->h;
    return zoomSurface(base, zoom_x, zoom_y, 1);
}

static nbool is_valid_color(NColor color)
{
    if (   color.r > 240
        && color.g > 240
        && color.b > 240 )
        return N_FALSE;
    else
        return N_TRUE;
}

static nbool is_differ_color(NColor c1, NColor c2)
{
    if (   N_ABS(((int)c1.r - (int)c2.r)) > 30
        || N_ABS(((int)c1.g - (int)c2.g)) > 30
        || N_ABS(((int)c1.b - (int)c2.b)) > 30 )
        return N_TRUE;
    else
        return N_FALSE;
}

static nbool guess_sides(SDL_Surface* base, int* l, int* t, int* r, int* b)
{
    const int sideW = 2;
    int w = base->w;
    int h = base->h;
    nbool ret = N_TRUE;
    int x, y;
    NColor color, last;
    int i, limit;
    nbool found = N_FALSE;
    
    if (w < 6 || h < 6)
        return N_FALSE;

    *l = *t = *r = sideW;

    limit = h - (int)(h * 0.3);
    for (i = h-1; i >= limit; i--) {
        x = w / 2;
        y = i;
        nse_getPixelRGBA(base, x, y, &color);
        if (is_valid_color(color)) {
            if (!found) {
                found = N_TRUE;
                last = color;
            }
            else if (is_differ_color(last, color))
                break;
        }
        else if (found)
            break;
    }

    if (i > limit)
        *b = h - i;
    else
        ret = N_FALSE;
    
    return ret;
}

static SDL_Surface* image_stretch_nine_palace(SDL_Surface* base, coord w, coord h)
{
    const int sideW = 2;
    int sl, st, sr, sb;
    nbool got = guess_sides(base, &sl, &st, &sr, &sb);
    SDL_Surface* bmp;
    SDL_Rect src, dst;
    SDL_Surface* bmp_c;
    SDL_Surface* bmp_s;
    double zx, zy;

    if (   !got
        || base->w < sl + sr + sideW
        || base->h < st + sb + sideW
        || w < sl + sr + sideW
        || h < st + sb + sideW )
        return image_stretch(base, w, h);

    bmp = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, base->format->BitsPerPixel,
	    base->format->Rmask, base->format->Gmask, base->format->Bmask, base->format->Amask);

    // 左上角
    src.x = src.y = 0; src.w = sl; src.h = st;
    dst = src;
    nse_copySurface(base, &src, bmp, &dst);

    // 顶边
    dst.x = sl; dst.y = 0; dst.w = base->w - sl - sr; dst.h = st;
    bmp_c = image_crop(base, dst);
    zx = (double)(w - sl - sr) / dst.w;
    bmp_s = zoomSurface(bmp_c, zx, 1.0, 1);
    src.x = src.y = 0; src.w = bmp_s->w; src.h = bmp_s->h;
    dst.x = sl; dst.y = 0; dst.w = src.w; dst.h = src.h;
    nse_copySurface(bmp_s, &src, bmp, &dst);
    SDL_FreeSurface(bmp_c);
    SDL_FreeSurface(bmp_s);

    // 右上角
    src.x = base->w - sl; src.y = 0; src.w = sr; src.h = st;
    dst.x = w - sl; dst.y = 0; dst.w = sr; dst.h = st;
    nse_copySurface(base, &src, bmp, &dst);

    // 左边
    dst.x = 0; dst.y = st; dst.w = sl; dst.h = base->h - st - sb;
    bmp_c = image_crop(base, dst);
    zy = (double)(h - st - sb) / dst.h;
    bmp_s = zoomSurface(bmp_c, 1.0, zy, 1);
    src.x = src.y = 0; src.w = bmp_s->w; src.h = bmp_s->h;
    dst.x = 0; dst.y = st; dst.w = src.w; dst.h = src.h;
    nse_copySurface(bmp_s, &src, bmp, &dst);
    SDL_FreeSurface(bmp_c);
    SDL_FreeSurface(bmp_s);

    // 中心
    dst.x = sl; dst.y = st; dst.w = base->w - sl - sr; dst.h = base->h - st - sb;
    bmp_c = image_crop(base, dst);
    zx = (double)(w - sl - sr) / dst.w;
    zy = (double)(h - st - sb) / dst.h;
    bmp_s = zoomSurface(bmp_c, zx, zy, 1);
    src.x = src.y = 0; src.w = bmp_s->w; src.h = bmp_s->h;
    dst.x = sl; dst.y = st; dst.w = w - sl - sr; dst.h = h - st - sb;
    nse_copySurface(bmp_s, &src, bmp, &dst);
    SDL_FreeSurface(bmp_c);
    SDL_FreeSurface(bmp_s);

    // 右边
    dst.x = base->w - sr; dst.y = st; dst.w = sr; dst.h = base->h - st - sb;
    bmp_c = image_crop(base, dst);
    zy = (double)(h - st - sb) / dst.h;
    bmp_s = zoomSurface(bmp_c, 1.0, zy, 1);
    src.x = src.y = 0; src.w = bmp_s->w; src.h = bmp_s->h;
    dst.x = w - sr; dst.y = st; dst.w = src.w; dst.h = src.h;
    nse_copySurface(bmp_s, &src, bmp, &dst);
    SDL_FreeSurface(bmp_c);
    SDL_FreeSurface(bmp_s);

    // 左下角
    src.x = 0; src.y = base->h - sb; src.w = sl; src.h = sb;
    dst.x = 0; dst.y = h - sb; dst.w = sl; dst.h = sb;
    nse_copySurface(base, &src, bmp, &dst);

    // 底边
    dst.x = sl; dst.y = base->h - sb; dst.w = base->w - sl - sr; dst.h = sb;
    bmp_c = image_crop(base, dst);
    zx = (double)(w - sl - sr) / dst.w;
    bmp_s = zoomSurface(bmp_c, zx, 1.0, 1);
    src.x = src.y = 0; src.w = bmp_s->w; src.h = bmp_s->h;
    dst.x = sl; dst.y = h - sb; dst.w = src.w; dst.h = src.h;
    nse_copySurface(bmp_s, &src, bmp, &dst);
    SDL_FreeSurface(bmp_c);
    SDL_FreeSurface(bmp_s);

    // 右下角
    src.x = base->w - sr; src.y = base->h - sb; src.w = sr; src.h = sb;
    dst.x = w - sr; dst.y = h - sb; dst.w = sr; dst.h = sb;
    nse_copySurface(base, &src, bmp, &dst);

    return bmp;
}

static SDL_Surface* image_transform(
    SDL_Surface* base, NImStretchInfo* sti, NCssValue bg_x, NCssValue bg_y, uint8 repeat, nbool crop, nbool force)
{
    coord iw, ih, x, y, w, h, sw, sh;
    NFloat zoom;
    SDL_Surface* image = N_NULL;
    SDL_Surface* image_s = N_NULL;

    iw = base->w;
    ih = base->h;

    if (crop) {
        x = css_calcBgPos(sti->ow, iw, bg_x, (repeat == CSS_REPEAT || repeat == CSS_REPEAT_X));
        y = css_calcBgPos(sti->oh, ih, bg_y, (repeat == CSS_REPEAT || repeat == CSS_REPEAT_Y));

        if (x < 0) {
            if (-x < iw) // 图片内偏移
                x = -x;
            else // 偏移大于图片宽度
                x = 0;
        }
        else if (x >= iw) // 偏移超出图片区域，无效
            return N_NULL;

        if (y < 0) {
            if (-y < ih)
                y = -y;
            else
                y = 0;
        }
        else if (y >= ih)
            return N_NULL;

        // 贴图使用区域
        w = N_MIN(iw - x, sti->ow);
        h = N_MIN(ih - y, sti->oh);

        if (w != iw || h != ih) { // 切图
            SDL_Rect r;
            r.x = x;
            r.y = y;
            r.w = w;
            r.h = h;
            image = image_crop(base, r);
#ifdef DEBUG_STRETCH
            fprintf(stderr, "    crop at %d, %d, %d x %d\n", x, y, w, h);
#endif
        }

        if (w == sti->ow && repeat != CSS_REPEAT_NO) {
            if (repeat == CSS_REPEAT)
                repeat = CSS_REPEAT_Y;
            else if (repeat == CSS_REPEAT_X)
                repeat = CSS_REPEAT_NO;
        }
        if (h == sti->oh && repeat != CSS_REPEAT_NO) {
            if (repeat == CSS_REPEAT)
                repeat = CSS_REPEAT_X;
            else if (repeat == CSS_REPEAT_Y)
                repeat = CSS_REPEAT_NO;
        }
    }
    else {
        x = y = 0;
        w = iw;
        h = ih;
    }

    if (image == N_NULL)
        image = base;

    // 计算拉伸后的区域
    if (force) {
        sw = sti->w;
        sh = sti->h;
    }
    else if (repeat == CSS_REPEAT) {
        sw = nfloat_imul(w, sti->rate);
        sh = nfloat_imul(h, sti->rate);
    }
    else if (repeat == CSS_REPEAT_NO) {
        zoom = nfloat_divUseInts(sti->w, sti->ow);
        sw = nfloat_imul(w, zoom);
        zoom = nfloat_divUseInts(sti->h, sti->oh);
        sh = nfloat_imul(h, zoom);
    }
    else if (repeat == CSS_REPEAT_X) {
        sw = nfloat_imul(w, sti->rate);
        zoom = nfloat_divUseInts(sti->h, sti->oh);
        sh = nfloat_imul(h, zoom);
    }
    else {
        zoom = nfloat_divUseInts(sti->w, sti->ow);
        sw = nfloat_imul(w, zoom);
        sh = nfloat_imul(h, sti->rate);
    }

    if (sw <= 0) sw = w;
    if (sh <= 0) sh = h;

#ifdef DEBUG_STRETCH
    fprintf(stderr, "    stretch to %d x %d\n", sw, sh);
#endif

    if (sw != iw || sh != ih) {
        if (sti->stretch == NESTI_NINE_PALACE)
            image_s = image_stretch_nine_palace(image, sw, sh);
        else
            image_s = image_stretch(image, sw, sh);
    }

    if (image_s) {
        if (image != base)
            SDL_FreeSurface(image);
        return image_s;
    }
    else if (image != base)
        return image;

    return N_NULL;
}

static void pic_background_stretch(NPic* pic)
{
    NCssValue bg_x, bg_y;
    uint8 repeat = CSS_REPEAT;
    SDL_Surface* image;
    NImStretchInfo* si = pic->d->sti;

    if (pic->d->cli) {
        cssVal_setInt(&bg_x, pic->d->cli->new_x);
        cssVal_setInt(&bg_y, pic->d->cli->new_y);
        repeat = pic->d->cli->repeat;
    }
    else {
        bg_x = si->bg_x;
        bg_y = si->bg_y;
    }

    image = image_transform(pic->bmp, si, bg_x, bg_y, repeat, (pic->d->cli == N_NULL), (pic->d->stis != N_NULL));

    if (pic->d->stis) {
        if (image) {
            si->user = image;
            pic->d->size.w = image->w;
            pic->d->size.h = image->h;
        }

        si = (NImStretchInfo*)sll_first(pic->d->stis);
        while (si) {
            bg_x = si->bg_x;
            bg_y = si->bg_y;
            image = image_transform(pic->bmp, si, bg_x, bg_y, repeat, N_TRUE, (pic->d->stis != N_NULL));
            if (image) {
                si->user = image;
                si->w = image->w;
                si->h = image->h;
            }
            si = (NImStretchInfo*)sll_next(pic->d->stis);
        }
    }
    else if (image) {
        SDL_FreeSurface(pic->bmp);
        pic->bmp = image;
        pic->d->size.w = image->w;
        pic->d->size.h = image->h;
    }
}

static int pic_do_decode(void* user)
{
	nbool succeed = N_FALSE;
	NPic* pic = (NPic*)user;
	SDL_RWops* src;

    //printf("enter thread %d\n", SDL_ThreadID());

    //fprintf(stderr, "decode %s\n", pic->d->src);

	if (pic->len > 0) {
		src = SDL_RWFromConstMem(pic->data, pic->len);
		if (src)
			pic->bmp = IMG_Load_RW(src, 1);
		if (pic->bmp) {
			pic->d->total = 1;
			pic->d->size.w = pic->bmp->w;
			pic->d->size.h = pic->bmp->h;
			succeed = N_TRUE;

#ifdef DEBUG_STRETCH
            fprintf(stderr, "decode %s ( %d x %d )\n", pic->d->src, pic->bmp->w, pic->bmp->h);
            if (pic->d->sti) {
                fprintf(stderr, "    stretch %d, %d x %d ==> %d x %d\n",
                    pic->d->sti->stretch, pic->d->sti->ow, pic->d->sti->oh, pic->d->sti->w, pic->d->sti->h);
            }
#endif

            if (pic->d->sti)
                pic_background_stretch(pic);
		}
        else { // 解码失败
            fprintf(stderr, "decode FAILED! %s\n", pic->d->src);
        }
	}
	pic_reset(pic);

    //printf("  id %d %d %d\n", pic->d->id, pic->stop, succeed);
	if (!pic->stop)
		picMgr_onDecodeOver(pic, succeed);
    //printf("  id %d\n", pic->d->id);

    //picMgr_removeThread((NPicMgr*)pic->mgr, pic->worker);

    //printf(" quit thread %d\n", SDL_ThreadID());
	return 0;
}

void pic_decode(NPic* pic)
{
	if (pic->worker == N_NULL) {
		pic->worker = SDL_CreateThread(pic_do_decode, pic);
        //printf("start thread %d\n", SDL_GetThreadID(pic->worker));
        //picMgr_addThread((NPicMgr*)pic->mgr, pic->worker);
    }
}

void pic_cancel(NPic* pic)
{
    pic->stop = N_TRUE;
}

static void pic_stretch(NPic* pic, const NRect* rect)
{
	float zoom;
	coord w = rect_getWidth(rect);

	if (pic->bmp->w == w) { // 绘制区域与图片区域相等，不缩放
		if (pic->bmpStretch) {
			SDL_FreeSurface(pic->bmpStretch);
			pic->bmpStretch = N_NULL;
			pic->zoom = 1.0;
		}
		return;
	}

	zoom = picMgr_getZoom((NPicMgr*)pic->mgr);

	if (zoom == 1.0) // 页面未缩放，但绘制区域与图片不一致，需缩放
		zoom = (float)w / pic->bmp->w;

	if (pic->zoom != zoom) {
		pic->zoom = zoom;
		if (pic->bmpStretch)
			SDL_FreeSurface(pic->bmpStretch);
		pic->bmpStretch = zoomSurface(pic->bmp, zoom, zoom, 1);
	}
}

// 绘制前景图
void pic_draw(NPic* pic, NBK_gdi* gdi, const NRect* rect)
{
	SDL_Surface* bmp;

	if (pic->bmp == N_NULL)
		return;

    if (pic->bmp->w == 1 && pic->bmp->h == 1) { // 取色填充
        NColor c;
        nse_getPixelRGBA(pic->bmp, 0, 0, &c);
        gdi_setBrushColor(gdi, &c);
        gdi_clearRect(gdi, rect);
        return;
    }

	pic_stretch(pic, rect);
	bmp = (pic->bmpStretch) ? pic->bmpStretch : pic->bmp;
	gdi_drawBitmap(gdi, bmp, rect);
}

// 绘制背景图
void pic_drawAsBackground(NPic* pic, NBK_gdi* gdi, const NRect* rect, int rx, int ry)
{
	SDL_Surface* bmp;
	NRect dst, dd, dt;
	int i, j;

	if (pic->d->state != NEIS_DECODE_FINISH)
		return;

    if (pic->bmp->w == 1 || pic->bmp->h == 1) { // 取色填充
        int16 x = (pic->bmp->w == 1) ? 0 : pic->bmp->w / 2;
        int16 y = (pic->bmp->h == 1) ? 0 : pic->bmp->h / 2;
        coord w, h;
        NColor c;
        nse_getPixelRGBA(pic->bmp, x, y, &c);
        w = rect_getWidth(rect);
        w = (rx > 0) ? w * rx : w;
        h = rect_getHeight(rect);
        h = (ry > 0) ? h * ry : h;
        rect_set(&dst, rect->l, rect->t, w, h);
        gdi_setBrushColor(gdi, &c);
        gdi_clearRect(gdi, &dst);
        return;
    }
	
    if (pic->d->stis) {
        if (pic->d->si_use->user)
            bmp = (SDL_Surface*)pic->d->si_use->user;
        else
            return;
    }
    else {
	    pic_stretch(pic, rect);
	    bmp = (pic->bmpStretch) ? pic->bmpStretch : pic->bmp;
    }

	dst = *rect;
	dd = dst;

	if (rx == 0 && ry == 0) {
		gdi_drawBitmap(gdi, bmp, &dd);
	}
	else if (rx && ry == 0) {
		for (i=0; i < rx; i++) {
			dt = dd;
			gdi_drawBitmap(gdi, bmp, &dt);
			rect_move(&dd, dd.l + rect_getWidth(&dst), dd.t);
		}
	}
	else if (rx == 0 && ry) {
		for (i=0; i < ry; i++) {
			dt = dd;
			gdi_drawBitmap(gdi, bmp, &dt);
			rect_move(&dd, dd.l, dd.t + rect_getHeight(&dst));
		}
	}
	else {
		for (i=0; i < ry; i++) {
			for (j=0; j < rx; j++) {
				dt = dd;
				gdi_drawBitmap(gdi, bmp, &dt);
				rect_move(&dd, dd.l + rect_getWidth(&dst), dd.t);
			}
			dd = dst;
			rect_move(&dd, dd.l, dd.t + rect_getHeight(&dst));
		}
	}
}
