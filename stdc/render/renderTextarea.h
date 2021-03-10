/*
 * renderTextarea.h
 *
 *  Created on: 2011-4-9
 *      Author: migr
 */

#ifndef RENDERTEXTAREA_H_
#define RENDERTEXTAREA_H_

#include "../inc/config.h"
#include "../inc/nbk_gdi.h"
#include "../dom/event.h"
#include "../editor/textEditor.h"
#include "renderNode.h"

#ifdef __cplusplus
extern "C" {
#endif
    
typedef struct _NRenderTextarea {

	NRenderNode		d;

	NFontId			fontId;
	nbool			inEdit : 1;
	NTextEditor*	editor;
	void*			pfd;

	NSize			editorSize; // in screen coordinate
	coord			trick; // track zoom change

	wchr*			altU;

} NRenderTextarea;

#ifdef NBK_MEM_TEST
int renderTextarea_memUsed(const NRenderTextarea* rt);
#endif

NRenderTextarea* renderTextarea_create(void* node);
void renderTextarea_delete(NRenderTextarea** area);

void renderTextarea_layout(NLayoutStat* stat, NRenderNode* rn, NStyle*, nbool force);
void renderTextarea_paint(NRenderNode* rn, NStyle*, NRenderRect* rect);

int renderTextarea_getEditTextLen(NRenderTextarea* rt);
char* renderTextarea_getEditText(NRenderTextarea* rt);
int renderTextarea_getEditLength(NRenderTextarea* rt);
int renderTextarea_getEditMaxLength(NRenderTextarea* rt);
void renderTextarea_getRectOfEditing(NRenderTextarea* rt, NRect* rect, NFontId* fontId);

void renderTextarea_edit(NRenderTextarea* rt, void* doc);
nbool renderTextarea_processKey(NRenderNode* rn, NEvent* event);

nbool renderTextarea_isEditing(NRenderTextarea* rt);

wchr* renderTextarea_getText16(NRenderTextarea* rt, int* len);
void renderTextarea_setText16(NRenderTextarea* rt, wchr* text, int len);

void renderTextarea_getSelPos(NRenderTextarea* rt, int* start, int* end);
void renderTextarea_setSelPos(NRenderTextarea* rt, int start, int end);

int16 renderTextarea_getDataSize(NRenderNode* rn, NStyle* style);

#ifdef __cplusplus
}
#endif

#endif /* RENDERTEXTAREA_H_ */
