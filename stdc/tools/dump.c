/*
 * probe.c
 *
 *  Created on: 2011-2-18
 *      Author: wuyulun
 */

#include "dump.h"
#include "../inc/nbk_cbDef.h"
#include "../inc/nbk_callback.h"
#include "../dom/page.h"
#include "str.h"
#include "../css/cssSelector.h"

void dump_int(void* page, int32 i)
{
    NBK_DbgInfo d;
    d.t = NBKDBG_INT;
    d.d.si = i;
    nbk_cb_call(NBK_EVENT_DEBUG1, &((NPage*)page)->cbEventNotify, &d);
}

void dump_uint(void* page, uint32 i)
{
    NBK_DbgInfo d;
    d.t = NBKDBG_UINT;
    d.d.ui = i;
    nbk_cb_call(NBK_EVENT_DEBUG1, &((NPage*)page)->cbEventNotify, &d);
}

void dump_char(void* page, char* mb, int length)
{
    NBK_DbgInfo d;
    d.t = NBKDBG_CHAR;
    d.d.cp = mb;
    d.len = length;
    nbk_cb_call(NBK_EVENT_DEBUG1, &((NPage*)page)->cbEventNotify, &d);
}

void dump_wchr(void* page, wchr* wc, int length)
{
    NBK_DbgInfo d;
    d.t = NBKDBG_WCHR;
    d.d.wp = wc;
    d.len = length;
    nbk_cb_call(NBK_EVENT_DEBUG1, &((NPage*)page)->cbEventNotify, &d);
}

void dump_return(void* page)
{
    NBK_DbgInfo d;
    d.t = NBKDBG_RETURN;
    nbk_cb_call(NBK_EVENT_DEBUG1, &((NPage*)page)->cbEventNotify, &d);
}

void dump_time(void* page)
{
    NBK_DbgInfo d;
    d.t = NBKDBG_TIME;
    nbk_cb_call(NBK_EVENT_DEBUG1, &((NPage*)page)->cbEventNotify, &d);
}

void dump_flush(void* page)
{
    NBK_DbgInfo d;
    d.t = NBKDBG_FLUSH;
    nbk_cb_call(NBK_EVENT_DEBUG1, &((NPage*)page)->cbEventNotify, &d);
}

void dump_tab(void* page, nbool on)
{
    NBK_DbgInfo d;
    d.t = NBKDBG_TAB;
    d.d.on = on;
    nbk_cb_call(NBK_EVENT_DEBUG1, &((NPage*)page)->cbEventNotify, &d);
}

//static void val_2_str(char* str, int val, int size, int base)
//{
//    int len = 0;
//    int t = 1;
//    int i, j, n, v;
//
//    if (base == 10) {
//        if (size == 8)
//            len = 3;
//        else if (size == 16)
//            len = 5;
//    }
//    else if (base == 16) {
//        if (size == 8)
//            len = 2;
//    }
//    if (len == 0) {
//        str[0] = 0;
//        return;
//    }
//
//    if (val < 0) {
//        v = -val;
//        str[0] = '-';
//        j = 0;
//    }
//    else {
//        v = val;
//        j = -1;
//    }
//    
//    for (i=1; i < len; i++)
//        t *= base;
//    for (i=0; i < len; i++) {
//        n = v / t;
//        if (n || (val > 0 && j != -1) || (val < 0 && j != 0) || base == 16) {
//            if (n < 10)
//                str[++j] = '0' + n;
//            else
//                str[++j] = 'a' + (n - 10);
//        }
//        v -= n * t;
//        t /= base;
//    }
//    if (j == -1)
//        str[++j] = '0';
//    str[++j] = 0;
//}

void dump_NColor(void* page, const NColor* color)
{
    char cs[32];
    int p = sprintf(cs, "%02x%02x%02x(%02x)", color->r, color->g, color->b, color->a);
    dump_char(page, cs, p);
}

void dump_NRect(void* page, const NRect* rect)
{
    char cs[64];
    int p = sprintf(cs, "%d,%d,%d,%d (%d x %d)",
        rect->l, rect->t, rect->r, rect->b, rect_getWidth(rect), rect_getHeight(rect));
    dump_char(page, cs, p);
}

void dump_NRectSides(void* page, const NRect* rect)
{
    char cs[64];
    int p = sprintf(cs, "%d,%d,%d,%d", rect->l, rect->t, rect->r, rect->b);
    dump_char(page, cs, p);
}

void dump_NCssBox(void* page, const void* box)
{
    NCssBox* cbox = (NCssBox*)box;
    char cs[64];
    int p = sprintf(cs, "[%d,%d,%d,%d]", cbox->l, cbox->t, cbox->r, cbox->b);
    dump_char(page, cs, p);
}

void dump_NCssValue(void* page, const void* cssVal)
{
    NCssValue* cv = (NCssValue*)cssVal;
    char buf[32];
    int len;
    
    switch (cv->type) {
    case NECVT_INT:
        len = sprintf(buf, "%d", cv->d.i32);
        break;
    case NECVT_PERCENT:
        len = sprintf(buf, "%d%%", cv->d.perc);
        break;
    case NECVT_EM:
        len = sprintf(buf, "%fem", cv->d.flo);
        break;
    default:
        nbk_strcpy(buf, "-");
        len = nbk_strlen(buf);
        break;
    }
    
    dump_char(page, buf, len);
}
