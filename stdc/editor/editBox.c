/*
 * editbox : a simple editor
 *
 *  Created on: 2011-3-27
 *      Author: migr
 */

#include "editBox.h"
#include "../css/color.h"
#include "../dom/page.h"
#include "../tools/unicode.h"
#include "../tools/str.h"

#define MAX_EDIT_TEXT_LEN   64

#ifdef NBK_MEM_TEST
int editBox_memUsed(const NEditBox* box)
{
    int size = 0;
    if (box) {
        size = sizeof(NEditBox);
        size += box->max * 2;
    }
    return size;
}
#endif

static void shift_text(wchr* dst, wchr* src, int length)
{
    int i;
    if (dst < src) {
        for (i=0; i < length; i++)
            dst[i] = src[i];
    }
    else if (dst > src) {
        for (i = length-1; i >= 0; i--)
            dst[i] = src[i];
    }
}

static void editbox_move_left(NEditBox* edit)
{
    if (edit->cur > 0) {
        edit->cur--;
        editBox_clearSelText(edit);
    }
    else {
        edit->cur = edit->use;
        editBox_clearSelText(edit);
    }
}

static void editbox_move_right(NEditBox* edit)
{
    if (edit->cur < edit->use) {
        edit->cur++;
        editBox_clearSelText(edit);
    }
    else {
        edit->cur = 0;
        editBox_clearSelText(edit);
    }
}

static void editbox_backspace(NEditBox* edit)
{
    /*int16 selLen = N_ABS((edit->cur - edit->anchorPos));

    if (selLen) {
        editBox_delSelText(edit);
    }
    else*/ if (edit->cur > 0) {
        shift_text(edit->text + edit->cur - 1, edit->text + edit->cur, edit->use - edit->cur);
        if (edit->password)
            shift_text(edit->mask + edit->cur - 1, edit->mask + edit->cur, edit->use - edit->cur);
        edit->cur--;
        edit->use--;
//        if (edit->password && edit->cur > 0)
//            edit->mask[edit->cur - 1] = edit->text[edit->cur - 1];
    }
    editBox_clearSelText(edit);
}

static void editbox_append_char(NEditBox* edit, wchr hz)
{
    if (edit->cur < edit->max - 1) {       
//        int16 selLen = N_ABS((edit->cur - edit->anchorPos));
//        if (selLen) {
//            editBox_delSelText(edit);
//        }              
        shift_text(edit->text + edit->cur + 1, edit->text + edit->cur, edit->use - edit->cur);
        edit->text[edit->cur] = hz;
        if (edit->password) {
            shift_text(edit->mask + edit->cur + 1, edit->mask + edit->cur, edit->use - edit->cur);
            edit->mask[edit->cur] = '*';
//            edit->mask[edit->cur] = hz;
//            if (edit->cur > 0)
//                edit->mask[edit->cur - 1] = '*';
        }
        edit->cur++;
        edit->use++;
        editBox_clearSelText(edit);
    }
}

NEditBox* editBox_create(nbool masked)
{
    NEditBox* edit = (NEditBox*)NBK_malloc0(sizeof(NEditBox));
    edit->password = masked;
    editBox_setMaxLength(edit, MAX_EDIT_TEXT_LEN);
    return edit;
}

void editBox_delete(NEditBox** edit)
{
    NEditBox* e = *edit;
    if (e->text)
        NBK_free(e->text);
    if (e->mask)
        NBK_free(e->mask);
    NBK_free(e);
    *edit = N_NULL;
}

void editBox_setMaxLength(NEditBox* edit, int16 length)
{
    if (edit->text)
        NBK_free(edit->text);
    if (edit->mask)
        NBK_free(edit->mask);
    
    edit->max = length + 1;
    edit->use = 0;
    edit->cur = 0;
    edit->text = (wchr*)NBK_malloc(sizeof(wchr) * length);
    if (edit->password)
        edit->mask = (wchr*)NBK_malloc(sizeof(wchr) * length);
}

void editBox_setText(NEditBox* edit, uint8* text, int16 length)
{
    int8 offset;
    uint8* p = text;
    int len = (length == -1) ? nbk_strlen((char*)text) : length;
    uint8* tooFar = p + len;

    len = N_MAX(len, MAX_EDIT_TEXT_LEN);
    if (edit->max <= len)
        editBox_setMaxLength(edit, len);
    
    edit->use = 0;
    while (*p && p < tooFar && edit->use < edit->max - 1) {
        edit->text[edit->use] = uni_utf8_to_utf16(p, &offset);
        if (edit->password)
            edit->mask[edit->use] = '*';
        edit->use++;
        p += offset;
    }
    edit->cur = 0;
    edit->anchorPos = 0;
}

void editBox_setText16(NEditBox* edit, wchr* text, int16 len)
{
    int16 i;
    
    if (edit->text == N_NULL)
        editBox_setMaxLength(edit, MAX_EDIT_TEXT_LEN);
   
    edit->use = 0;
    for (i=0; i < len && i < edit->max; i++) {
        if (edit->password)
            edit->mask[edit->use] = '*';
        edit->text[edit->use++] = text[i];
    }
    edit->cur = 0;
    edit->anchorPos = 0;
}

char* editBox_getText(NEditBox* edit)
{
    if (edit->use)
        return uni_utf16_to_utf8_str(edit->text, edit->use, N_NULL);
    else
        return N_NULL;
}

int editBox_getTextLen(NEditBox* edit)
{
    if (edit->use)
        return uni_utf16_to_utf8_len(edit->text, edit->use);
    else
        return 0;
}

nbool editBox_handleKey(NEditBox* edit, NEvent* event, NRect* vr, NFontId fontId)
{
    nbool handled = N_TRUE;
    
    if (edit->text == N_NULL)
        editBox_setMaxLength(edit, MAX_EDIT_TEXT_LEN);
    
    switch (event->type) {
    case NEEVENT_KEY:
    {
        switch (event->d.keyEvent.key) {
        case NEKEY_LEFT:
            editbox_move_left(edit);
            break;
        case NEKEY_RIGHT:
            editbox_move_right(edit);
            break;
        case NEKEY_BACKSPACE:
            editbox_backspace(edit);
            break;
        case NEKEY_CHAR:
            editbox_append_char(edit, event->d.keyEvent.chr);
            break;
        default:
            handled = N_FALSE;
            break;
        }
        break;
    }
    case NEEVENT_PEN:
    {
        switch (event->d.penEvent.type) {
        case NEPEN_DOWN:
        {
            NPage* page = (NPage*)event->page;
            int16 i = 0;
            coord x = vr->l + edit->xoff;
            wchr* text = (edit->password) ? edit->mask : edit->text;
            for (; i < edit->use && x < event->d.penEvent.pos.x; i++)
                x += NBK_gdi_getCharWidthByEditor(page->platform, fontId, text[i]);
            edit->cur = (x < event->d.penEvent.pos.x) ? edit->use : i - 1;
            editBox_clearSelText(edit);
            NBK_fep_updateCursor(page->platform);
            break;
        }
        case NEPEN_MOVE:
            break;
        case NEPEN_UP:
            break;
        default:
            break;
        }
        break;
    }
    default:
        handled = N_FALSE;
        break;
    }
    
    return handled;
}

void editBox_draw(NEditBox* edit, NFontId fontId, NRect* pr, nbool op, NStyle* style)
{
    NPage* page = (NPage*)((NView*)style->view)->page;
    NPoint pt;
    coord maxw, w, xOffset;
    wchr* text = (edit->password) ? edit->mask : edit->text;
       
    if (edit->use) {        
        maxw = NBK_gdi_getTextWidthByEditor(page->platform, fontId, text, edit->use);
        w = NBK_gdi_getTextWidthByEditor(page->platform, fontId, text, edit->cur);
    }
    else {
        maxw = w = 0;
    }

    if (maxw <= rect_getWidth(pr) || w <= rect_getWidth(pr))
        xOffset = 0;
    else
        xOffset = -(w - rect_getWidth(pr));
    
    pt.x = pr->l;
    pt.y = pr->t;
    
    if (edit->use) {
        NBK_gdi_setPenColor(page->platform, &style->ctlTextColor);    
        NBK_gdi_drawEditorText(page->platform, text, edit->use, &pt, xOffset);
    }
    
    edit->xoff = xOffset;
    
    if (op) { // draw cursor
        if (maxw <= rect_getWidth(pr) || w <= rect_getWidth(pr))
            xOffset = w;
        else
            xOffset = xOffset + w;
        
        pt.x = pr->l;
        pt.y = pr->t;

        NBK_gdi_setBrushColor(page->platform, &style->ctlEditColor);
        NBK_gdi_drawEditorCursor(page->platform, &pt, xOffset, rect_getHeight(pr));
    }
}

//void editBox_delSelText(NEditBox* edit)
//{
//    int16 selLen = N_ABS(edit->cur - edit->anchorPos);
//    int delPos = N_MAX(edit->cur,edit->anchorPos);
//    
//    while (selLen) {
//        shift_text(edit->text + delPos - 1, edit->text + delPos, edit->use - delPos);
//        if (edit->password)
//            shift_text(edit->mask + delPos - 1, edit->mask + delPos, edit->use - delPos);
//        
//       if (delPos == edit->cur) {
//            edit->cur--;
//            delPos = edit->cur;
//        }
//        else if (delPos == edit->anchorPos) {
//            edit->anchorPos--;
//            delPos = edit->anchorPos;
//        }
//       
//       edit->use--;
//        if (edit->password && delPos > 0)
//            edit->mask[delPos - 1] = edit->text[delPos - 1];
//
//        selLen--;
//    }
//}

void editBox_clearSelText(NEditBox* edit)
{
    int16 selLen = N_ABS(edit->cur - edit->anchorPos);

    if (selLen) {       
        edit->anchorPos = edit->cur;
    }
}

void editBox_maskAll(NEditBox* edit)
{
    if (edit->password) {
        int i = 0;
        for (i=0; i < edit->use; i++)
            edit->mask[i] = '*';
    }
}
