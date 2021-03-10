#ifndef __NBK_PICMGR_H__
#define __NBK_PICMGR_H__

#include "../stdc/inc/config.h"
#include "../stdc/render/imagePlayer.h"
#include "../stdc/tools/ptrArray.h"
#include <SDL.h>
#include "nbkpic.h"
#include "nbkgdi.h"

#define MAX_DECODE_THREAD   2

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NPicMgr {

	NPtrArray*	ary;
	void*		nbk;

    //SDL_Thread* threadList[MAX_DECODE_THREAD];
    //SDL_mutex*  mutex;

} NPicMgr, NBK_picMgr;

NPicMgr* picMgr_create(void);
void picMgr_delete(NPicMgr** mgr);

void picMgr_createImage(NPicMgr* mgr, NImagePlayer* player, NImage* image);
void picMgr_deleteImage(NPicMgr* mgr, NImage* image);
void picMgr_decodeImage(NPicMgr* mgr, NImage* image);

void picMgr_drawImage(NPicMgr* mgr, NImage* image, const NRect* rect, NBK_gdi* gdi);
void picMgr_drawBackground(NPicMgr* mgr, NImage* image, const NRect* rect, int rx, int ry, NBK_gdi* gdi);

// call by NImagePlayer
void picMgr_onImageData(NPicMgr* mgr, NImage* image, void* data, int len);
void picMgr_onImageDataEnd(NPicMgr* mgr, NImage* image, nbool succeed);
// call by NPic
void picMgr_onDecodeOver(NPic* pic, nbool succeed);

void picMgr_stopAllDecoding(NPicMgr* mgr);

float picMgr_getZoom(NPicMgr* mgr);

//void picMgr_addThread(NPicMgr* mgr, SDL_Thread* thread);
//void picMgr_removeThread(NPicMgr* mgr, SDL_Thread* thread);
//nbool picMgr_threadInWorking(NPicMgr* mgr, SDL_Thread* thread);

#ifdef __cplusplus
}
#endif

#endif
