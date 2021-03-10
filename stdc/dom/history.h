/*
 * history.h
 *
 *  Created on: 2011-5-8
 *      Author: migr
 */

#ifndef __HISTORY_H__
#define __HISTORY_H__

#include "../inc/config.h"
#include "../inc/nbk_settings.h"
#include "../inc/nbk_gdi.h"
#include "../editor/formData.h"
#include "../tools/slist.h"
#include "page.h"

#define NBK_HISTORY_MAX     16

#ifdef __cplusplus
extern "C" {
#endif
    
typedef struct _NHistoryItem {

	nid		id; // page id
	nid		charset;

	float	zoom; // zoom level used
	char*	url;

	// last read info
	nbool	hasReadInfo;
	NPoint	lastPos;
	float	lastZoom;

	// request mode used in current page
	nid		reqMode;

	// save form data
	NFormData*	formData;

} NHistoryItem;

typedef struct _NHistory {

	NPage*			page;
	int16			cur;
	int16			max;
	NHistoryItem	list[NBK_HISTORY_MAX];
	nid				preId;

} NHistory;

NHistory* history_create(void* pfd);
void history_delete(NHistory** history);

NPage* history_curr(NHistory* history);
void history_add(NHistory* history);
nbool history_prev(NHistory* history);
nbool history_next(NHistory* history);
nbool history_goto(NHistory* history, int16 idx);
nid history_getPreId(NHistory* history);

void history_getRange(NHistory* history, int16* cur, int16* used);

void history_setZoom(NHistory* history, float zoom);
float history_getZoom(NHistory* history);
float history_getZoomForPrevPage(NHistory* history);

void history_setSettings(NHistory* history, NSettings* settings);
void history_enablePic(NHistory* history, nbool enable);

nbool history_isLastPage(NHistory* history);

void history_setUrl(NHistory* history, const char* url);
char* history_getUrl(NHistory* history, nid pageId);

void history_setPaintParams(NHistory* history, int maxTime, nbool drawPic, nbool drawBgimg);

NPoint history_getReadInfo(NHistory* history, float* zoom);
nid history_getCharset(NHistory* history);

void history_setRequestMode(NHistory* history, nid mode);
nbool history_getLastRequestMode(NHistory* history, nid* mode);

void history_restoreFormData(NHistory* history, NPage* page);

// Implemented by platform.

void NBK_resourceClean(void* pfd, nid id);

#ifdef __cplusplus
}
#endif

#endif /* __HISTORY_H__ */
