/*
 * imagePlayer.h
 *
 *  Created on: 2011-2-21
 *      Author: wuyulun
 */

#ifndef IMAGEPLAYER_H_
#define IMAGEPLAYER_H_

#include "../inc/config.h"
#include "../inc/nbk_gdi.h"
#include "../inc/nbk_timer.h"
#include "../css/cssSelector.h"
#include "../render/renderNode.h"
#include "../loader/loader.h"
#include "../tools/ptrArray.h"
#include "../tools/slist.h"

#ifdef __cplusplus
extern "C" {
#endif
    
#define IMAGE_CUTE_ID   30000
    
enum NEImageState {

    NEIS_NONE,
    NEIS_HAS_CACHE,
    NEIS_PENDING,
    NEIS_LOADING,
    NEIS_MP_LOADING,
    NEIS_LOAD_NO_CACHE,
    NEIS_LOAD_FINISH,
    NEIS_LOAD_FAILED,
    NEIS_DECODING,
    NEIS_DECODE_FINISH,
    NEIS_DECODE_FAILED,
    NEIS_DELETE,
    NEIS_CANCEL
};

enum NEImageType {
    
    NEIT_NONE,
    NEIT_IMAGE,
    NEIT_CSS_IMAGE,
    NEIT_FLASH_IMAGE
};

typedef struct _NImage {    

    int8    curr;   // current frame
    int8    total;  // total of frame
    int8    state;
    int8    type;
    
    nbool    notify : 1; // 是否已通知render更新
    nbool    mustDecode : 1; // 该图必须解码
    
    nid     pageId;
    int16   id;
    
    nid     mime;
    char*   src;
    NSize   size;
    int     length;
    int     received;
    
    uint32	time; // track last used time.
    
} NImage;

typedef struct _NImagePlayer {

	NSmartPtr	_smart;
   
    nid         pageId;
    
    void*       view;
    NPtrArray*  list;
    
    NTimer*     aniTimer; // for animation
    
	nbool		disable : 1;
	nbool		loadByInvoke : 1;
	nbool		stop : 1;
	nbool		cache : 1;
	nbool		dlFinish : 1; // don't send progrss
	nbool		loadCache : 1;
    
    uint32		ticks;
    uint32		time;
    uint32		interval; // update interval
    int         numDecoded;
    
} NImagePlayer;

typedef struct _NImageRequest {
    
    NImage*         image;
    NImagePlayer*   player;
    
} NImageRequest;

typedef struct _NImageDrawInfo {
    
    int16   id;
    
    NRect   r; // tc_rect
    NRect*  pr; // paint rect
    NRect*  vr; // view rect
    
    uint8   repeat;
    uint8   ox_t;
    uint8   oy_t;
    coord   ox;
    coord   oy;

    coord   iw;
    coord   ih;
    
    float	zoom;
    
} NImageDrawInfo;

NImagePlayer* imagePlayer_create(void* view);
void imagePlayer_delete(NImagePlayer** player);
void imagePlayer_reset(NImagePlayer* player);
void imagePlayer_disable(NImagePlayer* player, nbool disable);
void imagePlayer_stopAll(NImagePlayer* player);

int16 imagePlayer_getId(NImagePlayer* player, const char* src, int8 type);
nbool imagePlayer_getSize(NImagePlayer* player, int16 id, NSize* size, nbool* fail);
void imagePlayer_draw(NImagePlayer* player, int16 id, const NRect* rect);
void imagePlayer_drawBg(NImagePlayer* player, const NImageDrawInfo di);
nbool imagePlayer_isGif(NImagePlayer* player, int16 id);
void imagePlayer_setDraw(NImagePlayer* player, int16 id);
nid imagePlayer_getState(NImagePlayer* player, int16 id);
void imagePlayer_invalidateId(NImagePlayer* player, int16 id);

void imagePlayer_onLoadResponse(NEResponseType type, void* user, void* data, int, int);
void imagePlayer_onDecodeOver(NImagePlayer* player, int16 id, nbool succeed);

void imagePlayer_start(NImagePlayer* player);
void imagePlayer_startDecode(NImagePlayer* player);
void imagePlayer_aniPause(NImagePlayer* player);
void imagePlayer_aniResume(NImagePlayer* player);

int imagePlayer_total(NImagePlayer* player);
void imagePlayer_progress(NImagePlayer* player);

// implemented by platform

void NBK_imageCreate(void* pfd, NImagePlayer* player, NImage* image);
void NBK_imageDelete(void* pfd, NImagePlayer* player, NImage* image);
void NBK_imageDecode(void* pfd, NImagePlayer* player, NImage* image);
void NBK_imageDraw(void* pfd, NImagePlayer* player, NImage* image, const NRect* dest);
void NBK_imageDrawBg(void* pfd, NImagePlayer* player, NImage* image, const NRect* dest, int rx, int ry);

void NBK_onImageData(void* pfd, NImage* image, void* data, int length);
void NBK_onImageDataEnd(void* pfd, NImage* image, nbool succeed);

void NBK_imageStopAll(void* pfd);

#ifdef __cplusplus
}
#endif

#endif /* IMAGEPLAYER_H_ */
