// create by wuyuln, 2011.12.25

#include "picmgr.h"
#include "nbk.h"
#include "runtime.h"

void NBK_imageCreate(void* pfd, NImagePlayer* player, NImage* image)
{
	NBK_core* nbk = (NBK_core*)pfd;
	nbk->picMgr->nbk = nbk;
	picMgr_createImage(nbk->picMgr, player, image);
}

void NBK_imageDelete(void* pfd, NImagePlayer* player, NImage* image)
{
	NBK_core* nbk = (NBK_core*)pfd;
	picMgr_deleteImage(nbk->picMgr, image);
}

void NBK_imageDecode(void* pfd, NImagePlayer* player, NImage* image)
{
	NBK_core* nbk = (NBK_core*)pfd;
	picMgr_decodeImage(nbk->picMgr, image);
}

void NBK_imageDraw(void* pfd, NImagePlayer* player, NImage* image, const NRect* dest)
{
	NBK_core* nbk = (NBK_core*)pfd;
	picMgr_drawImage(nbk->picMgr, image, dest, nbk->gdi);
}

void NBK_imageDrawBg(void* pfd, NImagePlayer* player, NImage* image, const NRect* dest, int rx, int ry)
{
	NBK_core* nbk = (NBK_core*)pfd;
	picMgr_drawBackground(nbk->picMgr, image, dest, rx, ry, nbk->gdi);
}

void NBK_onImageData(void* pfd, NImage* image, void* data, int length)
{
	NBK_core* nbk = (NBK_core*)pfd;
	picMgr_onImageData(nbk->picMgr, image, data, length);
}

void NBK_onImageDataEnd(void* pfd, NImage* image, nbool succeed)
{
	NBK_core* nbk = (NBK_core*)pfd;
	picMgr_onImageDataEnd(nbk->picMgr, image, succeed);
}

void NBK_imageStopAll(void* pfd)
{
	NBK_core* nbk = (NBK_core*)pfd;
	picMgr_stopAllDecoding(nbk->picMgr);
}

nbool NBK_isImageCached(void* pfd, const char* url, nbool history)
{
	return N_FALSE;
}

// 绘制广告占位符
void NBK_drawAdPlaceHolder(void* pfd, NRect* rect)
{
	NColor color = {0xFF, 0x7F, 0x50, 0x80};
	NBK_gdi_setBrushColor(pfd, &color);
	NBK_gdi_clearRect(pfd, rect);
}

///////////////////////////////////
//
// Picture Manager
//

static NPic* picMgr_getPic(NPicMgr* mgr, NImage* image)
{
	NPic* pic;
	int i;
	for (i=0; i < mgr->ary->use; i++) {
		pic = (NPic*)mgr->ary->data[i];
		if (pic->d == image)
			return pic;
	}
	return N_NULL;
}

NPicMgr* picMgr_create(void)
{
	NPicMgr* mgr = (NPicMgr*)NBK_malloc0(sizeof(NPicMgr));
	mgr->ary = ptrArray_create();
    //mgr->mutex = SDL_CreateMutex();
	return mgr;
}

void picMgr_delete(NPicMgr** mgr)
{
	NPicMgr* m = *mgr;
	NPic* pic;
	int i;

    //// 取消所有解码任务
    //for (i=0; i < m->ary->use; i++) {
    //    pic = (NPic*)m->ary->data[i];
    //    if (pic)
    //        pic_cancel(pic);
    //}

    //// 等待解码线程完成
    //for (i=0; i < MAX_DECODE_THREAD; i++) {
    //    if (m->threadList[i]) {
    //        fprintf(stderr, "waiting decode thread (%d) finish...\n", SDL_GetThreadID(m->threadList[i]));
    //        SDL_WaitThread(m->threadList[i], NULL);
    //    }
    //}

    // 删除所有解码任务
	for (i=0; i < m->ary->use; i++) {
		pic = (NPic*)m->ary->data[i];
		if (pic)
			pic_delete(&pic);
	}
	ptrArray_delete(&m->ary);

    //SDL_DestroyMutex(m->mutex);

	NBK_free(m);
	*mgr = N_NULL;
}

void picMgr_createImage(NPicMgr* mgr, NImagePlayer* player, NImage* image)
{
	NPic* pic = pic_create(image);
	int i;

	pic->player = player;
	pic->mgr = mgr;

	for (i=0; i < mgr->ary->use; i++)
		if (mgr->ary->data[i] == N_NULL)
			break;

	if (i < mgr->ary->use)
		mgr->ary->data[i] = pic;
	else
		ptrArray_append(mgr->ary, pic);
}

void picMgr_deleteImage(NPicMgr* mgr, NImage* image)
{
	NPic* pic;
	int i;
	for (i=0; i < mgr->ary->use; i++) {
		pic = (NPic*)mgr->ary->data[i];
		if (pic && pic->d == image) {
			pic_delete(&pic);
			mgr->ary->data[i] = N_NULL;
			break;
		}
	}
}

void picMgr_decodeImage(NPicMgr* mgr, NImage* image)
{
	NPic* pic = picMgr_getPic(mgr, image);
	pic_decode(pic);
}

void picMgr_drawImage(NPicMgr* mgr, NImage* image, const NRect* rect, NBK_gdi* gdi)
{
	NPic* pic = picMgr_getPic(mgr, image);
	pic_draw(pic, gdi, rect);
}

void picMgr_drawBackground(NPicMgr* mgr, NImage* image, const NRect* rect, int rx, int ry, NBK_gdi* gdi)
{
	NPic* pic = picMgr_getPic(mgr, image);
	pic_drawAsBackground(pic, gdi, rect, rx, ry);
}

void picMgr_onImageData(NPicMgr* mgr, NImage* image, void* data, int len)
{
	NPic* pic = picMgr_getPic(mgr, image);
	pic_onData(pic, data, len);
}

void picMgr_onImageDataEnd(NPicMgr* mgr, NImage* image, nbool succeed)
{
	NPic* pic = picMgr_getPic(mgr, image);
	pic_onDataEnd(pic, succeed);
}

void picMgr_stopAllDecoding(NPicMgr* mgr)
{
	NPic* pic;
	int i;
	for (i=0; i < mgr->ary->use; i++) {
		pic = (NPic*)mgr->ary->data[i];
		if (pic)
			pic_cancel(pic);
	}
}

void picMgr_onDecodeOver(NPic* pic, nbool succeed)
{
	sync_wait("Image Decode");
    if (!pic->stop)
	    imagePlayer_onDecodeOver(pic->player, pic->d->id, succeed);
	sync_post("Image Decode");
}

float picMgr_getZoom(NPicMgr* mgr)
{
	NBK_core* nbk = (NBK_core*)mgr->nbk;
	return NFloat_to_float(nbk->curZoom);
}

//void picMgr_addThread(NPicMgr* mgr, SDL_Thread* thread)
//{
//    int i;
//
//    SDL_LockMutex(mgr->mutex);
//
//    for (i=0; i < MAX_DECODE_THREAD; i++) {
//        if (mgr->threadList[i] == N_NULL) {
//            mgr->threadList[i] = thread;
//            break;
//        }
//    }
//
//    SDL_UnlockMutex(mgr->mutex);
//}
//
//void picMgr_removeThread(NPicMgr* mgr, SDL_Thread* thread)
//{
//    int i;
//
//    SDL_LockMutex(mgr->mutex);
//
//    for (i=0; i < MAX_DECODE_THREAD; i++) {
//        if (mgr->threadList[i] == thread) {
//            mgr->threadList[i] = N_NULL;
//            break;
//        }
//    }
//
//    SDL_UnlockMutex(mgr->mutex);
//}
//
//nbool picMgr_threadInWorking(NPicMgr* mgr, SDL_Thread* thread)
//{
//    int i;
//
//    SDL_LockMutex(mgr->mutex);
//
//    for (i=0; i < MAX_DECODE_THREAD; i++) {
//        if (mgr->threadList[i] == thread)
//            return N_TRUE;
//    }
//
//    SDL_UnlockMutex(mgr->mutex);
//
//    return N_FALSE;
//}
