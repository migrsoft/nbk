#include "../inc/nbk_gdi.h"
#include "unicode.h"

coord nbk_gdi_getTextWidth_utf8(void* pfd, NFontId id, uint8* text, int length)
{
    coord width = 0;
    uint8* p = text;
    uint8* tooFar = (length == -1) ? (uint8*)N_MAX_UINT : p + length;
    wchr hz;
    int8 offset;
    
    while (*p && p < tooFar) {

        hz = uni_utf8_to_utf16(p, &offset);
        p += offset;
        width += NBK_gdi_getCharWidth(pfd, id, hz);
    }
    
    return width;
}

void nbk_gdi_drawText_utf8(void* pfd, NFontId id, const uint8* text, int length, NPoint* pos)
{
    coord w;
    uint8* p = (uint8*)text;
    uint8* tooFar = (length == -1) ? (uint8*)N_MAX_UINT : p + length;
    wchr hz;
    int8 offset;
    
    while (*p && p < tooFar) {
        
        hz = uni_utf8_to_utf16(p, &offset);
        NBK_gdi_drawText(pfd, &hz, 1, pos, 0);
        p += offset;
        w = NBK_gdi_getCharWidth(pfd, id, hz);
        pos->x += w;
    }
}
