// create by wuyulun, 2011.12.25

#include "../stdc/inc/config.h"
#include "../stdc/inc/nbk_gdi.h"
#include "../stdc/inc/nbk_ctlPainter.h"
#include "../stdc/css/color.h"
#include "../stdc/tools/str.h"
#include "nbk.h"

#include <stdio.h>

nbool NBK_paintText(void* pfd, const NRect* rect, NECtrlState state, const NRect* clip)
{
	return N_FALSE;
}

nbool NBK_paintCheckbox(void* pfd, const NRect* rect, NECtrlState state, nbool checked, const NRect* clip)
{
	return N_FALSE;
}

nbool NBK_paintRadio(void* pfd, const NRect* rect, NECtrlState state, nbool checked, const NRect* clip)
{
	return N_FALSE;
}

nbool NBK_paintButton(void* pfd, const NFontId fontId, const NRect* rect, const wchr* text, int len, NECtrlState state, const NRect* clip)
{
	return N_FALSE;
}

nbool NBK_paintSelectNormal(void* pfd, const NFontId fontId, const NRect* rect, const wchr* text, int len, NECtrlState state, const NRect* clip)
{
	return N_FALSE;
}

nbool NBK_paintSelectExpand(void* pfd, const wchr* items, int num, int* sel)
{
	return N_FALSE;
}

nbool NBK_paintTextarea(void* pfd, const NRect* rect, NECtrlState state, const NRect* clip)
{
	return N_FALSE;
}

nbool NBK_paintBrowse(void* pfd, const NFontId fontId, const NRect* rect, const wchr* text, int len, NECtrlState state, const NRect* clip)
{
	return N_FALSE;
}

nbool NBK_paintFoldBackground(void* pfd, const NRect* rect, NECtrlState state, const NRect* clip)
{
	return N_FALSE;
}

// for editor
nbool NBK_editInput(void* pfd, NFontId fontId, NRect* rect, const wchr* text, int len, int maxlen, nbool password)
{
	return N_FALSE;
}

nbool NBK_editTextarea(void* pfd, NFontId fontId, NRect* rect, const wchr* text, int len, int maxlen)
{
	return N_FALSE;
}

// open a file select dialog by framework
nbool NBK_browseFile(void* pfd, const char* oldFile, char** newFile)
{
	return N_FALSE;
}

nbool NBK_dlgAlert(void* pfd, const wchr* text, int len)
{
    fprintf(stderr, "Alert!\n");
	return N_TRUE;
}

nbool NBK_dlgConfirm(void* pfd, const wchr* text, int len, int* ret)
{
    fprintf(stderr, "Confirm!\n");
    *ret = 0;
	return N_TRUE;
}

nbool NBK_dlgPrompt(void* pfd, const wchr* text, int len, int* ret, char** input)
{
    fprintf(stderr, "Prompt!\n");
	return N_FALSE;
}

void NBK_drawStockImage(void* pfd, const char* res, const NRect* rect, NECtrlState state, const NRect* clip)
{
    if (nbk_strcmp(res, RES_TSEL_DOWN) == 0) {
        NRect r = *rect;
        NBK_gdi_setPenColor(pfd, &colorRed);
        NBK_gdi_drawRect(pfd, rect);
        r.l = r.l + rect_getWidth(&r) / 2 - 1;
        r.r = r.l + 2;
        NBK_gdi_setPenColor(pfd, &colorBlue);
        NBK_gdi_drawRect(pfd, &r);
    }
    else if (nbk_strcmp(res, RES_TSEL_UP) == 0) {
        NRect r = *rect;
        NBK_gdi_setPenColor(pfd, &colorRed);
        NBK_gdi_drawRect(pfd, rect);
        r.l = r.l + rect_getWidth(&r) / 2 - 1;
        r.r = r.l + 2;
        NBK_gdi_setPenColor(pfd, &colorBlue);
        NBK_gdi_drawRect(pfd, &r);
    }
    else if (nbk_strcmp(res, "res://icon/collapse_plus") == 0) {
        NRect r = *rect;
        NBK_gdi_setPenColor(pfd, &colorBlue);
        NBK_gdi_drawRect(pfd, &r);
        r.t = rect->t + rect_getHeight(rect) / 2;
        r.b = r.t + 1;
        NBK_gdi_drawRect(pfd, &r);
        r = *rect;
        r.l = rect->l + rect_getWidth(rect) / 2;
        r.r = r.l + 1;
        NBK_gdi_drawRect(pfd, &r);
    }
    else if (nbk_strcmp(res, "res://icon/collapse_minus") == 0) {
        NRect r = *rect;
        NBK_gdi_setPenColor(pfd, &colorBlue);
        NBK_gdi_drawRect(pfd, &r);
        r.t = rect->t + rect_getHeight(rect) / 2;
        r.b = r.t + 1;
        NBK_gdi_drawRect(pfd, &r);
    }
    else {
        fprintf(stderr, "draw stock image: %s\n", res);
    }
}
