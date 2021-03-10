/*
 * renderInput.h
 *
 *  Created on: 2011-3-24
 *      Author: wuyulun
 */

#ifndef RENDERINPUT_H_
#define RENDERINPUT_H_

#include "../inc/config.h"
#include "../inc/nbk_gdi.h"
#include "../dom/event.h"
#include "../editor/editBox.h"
#include "renderNode.h"

#ifdef __cplusplus
extern "C" {
#endif
    
enum NEInputType {
    
    NEINPUT_UNKNOWN,
    NEINPUT_BUTTON,
    NEINPUT_CHECKBOX,
    NEINPUT_FILE,
    NEINPUT_HIDDEN,
    NEINPUT_IMAGE,
    NEINPUT_PASSWORD,
    NEINPUT_RADIO,
    NEINPUT_RESET,
    NEINPUT_SUBMIT,
    NEINPUT_TEXT,
    NEINPUT_GO
};

typedef struct _NRenderInput {
    
    NRenderNode d;
    
    nid         type;
    NFontId     fontId;
    
    nbool        inEdit : 1;
    nbool        checked : 1;
    nbool        picFail : 1;
    nbool        picGot : 1;
    nbool        cssWH : 1; // use width/height from css
    
    NEditBox*   editor;
    
    NCssBox     border;
    
    char*       text;
    wchr*       textU; // for display only
    wchr*       altU;
    
    int16       iid;
    
    NSize       editorSize; // in screen coordinate
    
} NRenderInput;

#ifdef NBK_MEM_TEST
int renderInput_memUsed(const NRenderInput* ri);
#endif

NRenderInput* renderInput_create(void* node);
void renderInput_delete(NRenderInput** ri);

void renderInput_layout(NLayoutStat* stat, NRenderNode* rn, NStyle*, nbool force);
void renderInput_paint(NRenderNode* rn, NStyle*, NRenderRect* rect);

nbool renderInput_isVisible(NRenderInput* ri);
nbool renderInput_isEditing(NRenderInput* ri);

void renderInput_edit(NRenderInput* ri, void* doc);
nbool renderInput_processKey(NRenderNode* rn, NEvent* event);
char* renderInput_getEditText(NRenderInput* ri);
int renderInput_getEditTextLen(NRenderInput* ri);
int renderInput_getEditLength(NRenderInput* ri);
int renderInput_getEditMaxLength(NRenderInput* ri);
void renderInput_getRectOfEditing(NRenderInput* ri, NRect* rect, NFontId* fontId);

wchr* renderInput_getEditText16(NRenderInput* ri, int* len);
void renderInput_setEditText16(NRenderInput* ri, wchr* text, int len);

void renderInput_getSelPos(NRenderInput* ri, int* start, int* end);
void renderInput_setSelPos(NRenderInput* ri, int start, int end);

void renderInput_setText(NRenderInput* ri, char* text);

int16 renderInput_getDataSize(NRenderNode* rn, NStyle* style);

#ifdef __cplusplus
}
#endif

#endif /* RENDERINPUT_H_ */
