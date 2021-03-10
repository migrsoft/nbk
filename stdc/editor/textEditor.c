/*
 * textEditor.c
 *
 *  Created on: 2011-4-9
 *      Author: migr
 */

#include "textEditor.h"
#include "../dom/page.h"
#include "../tools/unicode.h"
#include "../dom/page.h"

#define MAX_EDIT_CHAR           512
#define LINE_SPACING            2
#define SCROLLBAR_WIDTH         3
#define SCROLLBAR_SPACING       2
#define SCROLLBAR_MIN_HEIGHT    8

#ifdef NBK_MEM_TEST
int textEditor_memUsed(const NTextEditor* edit)
{
    int size = 0;
    if (edit) {
        size = sizeof(NTextEditor);
        size += (edit->max + 1) * 2;
        size += (edit->lmax + 1) * 2;
    }
    return size;
}
#endif

NTextEditor* textEditor_create(void)
{
    NTextEditor* e = (NTextEditor*)NBK_malloc0(sizeof(NTextEditor));
    N_ASSERT(e);
    
    e->text = (wchr*)NBK_malloc0(sizeof(wchr) * MAX_EDIT_CHAR);
    e->bps = (int16*)NBK_malloc0(sizeof(int16) * MAX_EDIT_CHAR);
    e->max = MAX_EDIT_CHAR - 1;
    e->lmax = MAX_EDIT_CHAR - 1;
    e->luse = 1;
    return e;
}

void textEditor_delete(NTextEditor** edit)
{
    NTextEditor* e = *edit;
    NBK_free(e->text);
    NBK_free(e->bps);
    NBK_free(e);
    *edit = N_NULL;
}

char* textEditor_getText(NTextEditor* edit)
{
    if (edit->use)
        return uni_utf16_to_utf8_str(edit->text, edit->use, N_NULL);
    else
        return N_NULL;
}

int textEditor_getTextLen(NTextEditor* edit)
{
    if (edit->use) {
        int i, c;
        for (i=0, c=0; i < edit->use; i++) {
            if (edit->text[i] == 0xa)
                c++;
        }
        return uni_utf16_to_utf8_len(edit->text, edit->use) + c;
    }
    else
        return 0;
}

static void move_left(NTextEditor* edit)
{
    int16 i;
    
    if (edit->cur == 0)
        return;
    
    i = edit->cur - 1;
    if (i < edit->bps[edit->ln]) {
        edit->ln--;
        if (edit->ln < edit->top)
            edit->top--;
        edit->cur = i;
    }
    else {
        edit->cur = i;
    }
    
    textEditor_clearSelText(edit);
}

static void move_right(NTextEditor* edit)
{
    int16 i;
    
    if (edit->cur == edit->use)
        return;
    
    i = edit->cur + 1;
    if (i < edit->bps[edit->ln + 1] || edit->ln + 1 == edit->luse) {
        edit->cur = i;
    }
    else {
        edit->ln++;
        if (edit->ln >= edit->top + edit->vis)
            edit->top++;
        edit->cur = edit->bps[edit->ln];
    }
    
    textEditor_clearSelText(edit);
}

static nbool move_up(NTextEditor* edit)
{
    int16 i, j;
    
    if (edit->ln == 0)
        return N_FALSE;
    
    i = edit->cur - edit->bps[edit->ln];
    edit->ln--;
    j = edit->bps[edit->ln + 1] - edit->bps[edit->ln];

    if (i >= j)
        i = j - 1;
    edit->cur = edit->bps[edit->ln] + i;
    
    if (edit->ln < edit->top)
        edit->top--;
    
    textEditor_clearSelText(edit);

    return N_TRUE;
}

static nbool move_down(NTextEditor* edit)
{
    int16 i, j;
    
    if (edit->ln + 1 == edit->luse)
        return N_FALSE;
    
    i = edit->cur - edit->bps[edit->ln];
    edit->ln++;
    j = edit->bps[edit->ln + 1] - edit->bps[edit->ln];
    
    if (i >= j)
        i = j - 1;
    if (i < 0)
        i = 0;
    edit->cur = edit->bps[edit->ln] + i;
    
    if (edit->ln >= edit->top + edit->vis)
        edit->top++;
    
    textEditor_clearSelText(edit);

    return N_TRUE;
}

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

static void adjust_lines(NTextEditor* edit, void* pfd)
{
    int16 i, j;
    coord tw, w;
    
    if (edit->width <= 0)
        return;
    
    i = edit->bps[edit->ln];
    if (i > edit->use)
        i = edit->bps[--edit->ln];
    tw = 0;
    while (i < edit->use && edit->ln < edit->lmax - 1) {
        if (edit->text[i] == 0xa) {
            i++;
            edit->bps[++edit->ln] = i;
            tw = 0;
        }
        else {
            w = NBK_gdi_getCharWidthByEditor(pfd, edit->fontId, edit->text[i]);
            if (tw + w > edit->width) {
                edit->bps[++edit->ln] = i;
                tw = 0;
            }
            else {
                tw += w;
                i++;
            }
        }
    }
    
    edit->luse = edit->ln + 1;
    edit->bps[edit->luse] = edit->use;
    
    for (j=0; j < edit->luse; j++) {
        if (edit->cur >= edit->bps[j] && edit->cur < edit->bps[j+1]) {
            edit->ln = j;
            break;
        }
    }
    
    if (edit->ln < edit->top)
        edit->top--;
    else if (edit->ln >= edit->top + edit->vis)
        edit->top++;
}

static void do_backspace(NTextEditor* edit, void* pfd)
{
    /*int16 selLen = 0;  
    selLen = N_ABS((edit->cur - edit->anchorPos));

    if (selLen) {
        textEditor_delSelText(edit,pfd);
    }
    else*/ if (edit->cur > 0){
        shift_text(edit->text + edit->cur - 1, edit->text + edit->cur, edit->use - edit->cur);
        edit->cur--;
        edit->use--;
        adjust_lines(edit, pfd);
    }
    textEditor_clearSelText(edit);
}

static void append_char(NTextEditor* edit, wchr hz, void* pfd)
{
//    int16 selLen = 0;
    if (edit->cur == edit->max)
        return;
    
//    selLen = N_ABS((edit->cur - edit->anchorPos));
//    if (selLen) {
//        textEditor_delSelText(edit,pfd);
//    }     
    shift_text(edit->text + edit->cur + 1, edit->text + edit->cur, edit->use - edit->cur);
    edit->text[edit->cur++] = hz;
    edit->use++;
    adjust_lines(edit, pfd);
    
    textEditor_clearSelText(edit);
}

void textEditor_setText(NTextEditor* edit, wchr* text, int len, void* pfd)
{    
    int i, j;
    i = j = 0;
    while (i < len && j < edit->max) {
        if (text[i] == 0xd) {
            edit->text[j++] = 0xa;
            if (text[i+1] == 0xa)
                i += 2;
            else
                i++;
        }
        else
            edit->text[j++] = text[i++];
    }
    edit->text[j] = 0;
    
    edit->use = j;
    edit->cur = 0;
    edit->ln = 0;
    edit->top = 0;
    
    if (pfd)
        adjust_lines(edit, pfd);
    else
        edit->ln = -1;
    
    edit->cur = 0;
    edit->anchorPos = 0;
}

void textEditor_prepare(NTextEditor* edit, NFontId font, coord w, coord h, coord lh, void* pfd)
{
    edit->fontId = font;
    edit->width = w - SCROLLBAR_SPACING - SCROLLBAR_WIDTH;
    edit->vis = h / (lh + LINE_SPACING);
    if (edit->vis < 1)
        edit->vis = 1;
    edit->ln = 0;

    adjust_lines(edit, pfd);
}

nbool textEditor_handleKey(NTextEditor* edit, NEvent* event, NRect* vr, NFontId fontId)
{
    nbool handled = N_TRUE;
    NPage* page = (NPage*)event->page;
    
    switch (event->type) {
    case NEEVENT_KEY:
    {
        switch (event->d.keyEvent.key) {
        case NEKEY_LEFT:
            move_left(edit);
            break;
        case NEKEY_RIGHT:
            move_right(edit);
            break;
        case NEKEY_UP:
            handled = move_up(edit);
            break;
        case NEKEY_DOWN:
            handled = move_down(edit);
            break;
        case NEKEY_BACKSPACE:
            do_backspace(edit, page->platform);
            break;
        case NEKEY_ENTER:
            append_char(edit, 0xa, page->platform);
            break;
        case NEKEY_CHAR:
            append_char(edit, event->d.keyEvent.chr, page->platform);
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
            int fh = NBK_gdi_getFontHeightByEditor(page->platform, edit->fontId);
            int i, ln = -1;
            coord x, y;
            
            i = edit->top;
            y = vr->t;
            fh += LINE_SPACING;
            for (; i < edit->top + edit->vis && i < edit->luse; i++) {
                if (   event->d.penEvent.pos.y >= y
                    && event->d.penEvent.pos.y < y + fh ) {
                    ln = i;
                    break;
                }
                y += fh;
            }
            if (ln == -1)
                break;
            edit->ln = ln;
       
            i = edit->bps[edit->ln];
            x = vr->l;
            for (; i <= edit->bps[edit->ln + 1] && x < event->d.penEvent.pos.x; i++)
                x += NBK_gdi_getCharWidthByEditor(page->platform, fontId, edit->text[i]);
            
            if (edit->use > edit->bps[edit->ln + 1]) {
                edit->cur = (i <= edit->bps[edit->ln + 1] ? i - 1 : edit->bps[edit->ln + 1] - 1);
            }
            else {
                edit->cur = (i <= edit->bps[edit->ln + 1] ? i - 1 : edit->bps[edit->ln + 1]);
            }
            if (edit->cur < 0)
                edit->cur = 0;
            
            textEditor_clearSelText(edit);
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

static void draw_scrollbar(NTextEditor* edit, NRect* rect, void* pfd)
{
    NPoint pos;
    NSize sz;
    coord h, y;
    
    if (edit->luse <= edit->vis)
        return;
    
    pos.x = rect->l;
    pos.y = rect->t;
    
    h = rect_getHeight(rect) * edit->vis / edit->luse;
    h = (h < SCROLLBAR_MIN_HEIGHT) ? SCROLLBAR_MIN_HEIGHT : h;
    sz.w = SCROLLBAR_WIDTH;
    sz.h = h;
    
    y = (rect_getHeight(rect) - h) / (edit->luse - edit->vis) * edit->top;
    
    if (edit->top + edit->vis == edit->luse)
        y = rect_getHeight(rect) - h;
    
    NBK_gdi_drawEditorScrollbar(pfd, &pos, edit->width + SCROLLBAR_SPACING, y, &sz);
}

void textEditor_draw(NTextEditor* edit, NRect* pr, nbool op, NStyle* style)
{
    NPage* page = (NPage*)((NView*)style->view)->page;
    int i, j, l;
    int fh = NBK_gdi_getFontHeightByEditor(page->platform, edit->fontId);
    NPoint pt, cp;
    coord y;
    
    if (edit->use > 0 && edit->ln == -1) {
        edit->ln = 0;
        adjust_lines(edit, page->platform);
    }
    
    NBK_gdi_setPenColor(page->platform, &style->ctlTextColor);
    NBK_gdi_setBrushColor(page->platform, &style->ctlEditColor);
    
    NBK_gdi_useFont(page->platform, edit->fontId);
    
    pt.x = pr->l;
    y = pr->t;

    i = edit->top;
    while (i < edit->top + edit->vis && i < edit->luse) {
        if (i == edit->ln) {
            cp.x = pt.x;
            cp.y = y;
        }

        j = edit->bps[i + 1];
        if (j == edit->bps[i])
            break;
        
        while (edit->text[j - 1] == 0xa && j > edit->bps[i])
            j--;
        l = j - edit->bps[i];
        if (l > 0) {
            pt.y = y;
            NBK_gdi_drawEditorText(page->platform, &edit->text[edit->bps[i]], l, &pt, 0);
        }
        i++;
        y += fh + LINE_SPACING;
    }
    
    NBK_gdi_releaseFont(page->platform);
    
    if (op) {
        coord w = NBK_gdi_getTextWidthByEditor(page->platform, edit->fontId,
            &edit->text[edit->bps[edit->ln]], edit->cur - edit->bps[edit->ln]);
        
        NBK_gdi_drawEditorCursor(page->platform, &cp, w, fh);
        
        draw_scrollbar(edit, pr, page->platform);
    }    
}

coord textEditor_getYOfEditing(NTextEditor* edit, void* pfd)
{
    int i, j;
    int fh = NBK_gdi_getFontHeight(pfd, edit->fontId);
    coord y;

    y = 0;
    i = edit->top;
    while (i < edit->top + edit->vis && i < edit->luse) {
        if (i == edit->ln) {
            return y;
        }
        
        j = edit->bps[i+1];
        if (j == edit->bps[i])
            break;
        i++;
        y += fh + 2;
    }
    
    return y;
}

//void textEditor_delSelText(NTextEditor* edit,void* pfd)
//{   
//    int16 selLen = N_ABS((edit->cur - edit->anchorPos));
//    int delPos = N_MAX(edit->cur,edit->anchorPos);
//    while (selLen) {
//        
//        shift_text(edit->text + delPos - 1, edit->text + delPos, edit->use - delPos);
//
//        if (delPos == edit->cur) {
//            edit->cur--;
//            delPos = edit->cur;
//        }
//        else if (delPos == edit->anchorPos) {
//            edit->anchorPos--;
//            delPos = edit->anchorPos;
//        }
//        edit->use--;
//        adjust_lines(edit, pfd);
//        selLen--;
//    }
//}

void textEditor_clearSelText(NTextEditor* edit)
{
    int16 selLen = N_ABS((edit->cur - edit->anchorPos));

    if (selLen) {       
        edit->anchorPos = edit->cur;
    }
}

void textEditor_SeleAllText(NTextEditor* edit, void* pfd)
{
    edit->anchorPos = 0;
    edit->cur = edit->use;
    
    if (pfd)
        adjust_lines(edit, pfd);
    else
        edit->ln = -1;
}
