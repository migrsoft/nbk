/*
 * A multiple lines text editor
 *
 *  Created on: 2011-4-9
 *      Author: migr
 */

#ifndef TEXTEDITOR_H_
#define TEXTEDITOR_H_

#include "../inc/config.h"
#include "../inc/nbk_gdi.h"
#include "../dom/event.h"
#include "../render/renderNode.h"

#ifdef __cplusplus
extern "C" {
#endif
    
typedef struct _NTextEditor {
    
    wchr*   text;
    
    int16   max;
    int16   use;
    int16   cur;
    int16   anchorPos;

    int8    top; // line be display at top
    int8    vis; // lines in visible

    int16*  bps; // list of break points
    int16   lmax;
    int16   luse;
    int16   ln; // current line
    
    NFontId fontId;
    coord   width; // in screen
    
} NTextEditor;

#ifdef NBK_MEM_TEST
int textEditor_memUsed(const NTextEditor* edit);
#endif

NTextEditor* textEditor_create(void);
void textEditor_delete(NTextEditor** edit);

void textEditor_prepare(NTextEditor* edit, NFontId font, coord w, coord h, coord lh, void* pfd);
void textEditor_setText(NTextEditor* edit, wchr* text, int len, void* pfd);
char* textEditor_getText(NTextEditor* edit);
int textEditor_getTextLen(NTextEditor* edit);
nbool textEditor_handleKey(NTextEditor* edit, NEvent* event, NRect* vr, NFontId fontId);
void textEditor_draw(NTextEditor* edit, NRect* pr, nbool op, NStyle* style);
coord textEditor_getYOfEditing(NTextEditor* edit, void* pfd);
//void textEditor_delSelText(NTextEditor* edit, void* pfd);
void textEditor_clearSelText(NTextEditor* edit);
void textEditor_SeleAllText(NTextEditor* edit, void* pfd);

#ifdef __cplusplus
}
#endif

#endif /* TEXTEDITOR_H_ */
