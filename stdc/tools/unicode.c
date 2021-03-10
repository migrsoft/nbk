/*
 * unicode.c
 *
 *  Created on: 2011-3-27
 *      Author: migr
 */

#include "../inc/config.h"
#include "unicode.h"
#include "str.h"

int uni_utf16_to_utf8(wchr hz, char* cc)
{
//    hz = N_SWAP_UINT16(hz);
    
    if (hz <= 0x7f) {
        cc[0] = (uint8)hz;
        cc[1] = 0;
        return 1;
    }
    else if (hz >= 0x800) {
        cc[0] = hz >> 12 & 0xf;
        cc[0] |= 0xe0;
        cc[1] = hz >> 6 & 0x3f;
        cc[1] |= 0x80;
        cc[2] = hz & 0x3f;
        cc[2] |= 0x80;
        cc[3] = 0;
        return 3;
    }
    else {
        cc[0] = hz >> 6 & 0x1f;
        cc[0] |= 0xc0;
        cc[1] = hz & 0x3f;
        cc[1] |= 0x80;
        cc[2] = 0;
        return 2;
    }
}

int uni_utf16_to_utf8_len(wchr* hzs, int hzlen)
{
    int i = 0;
    int len = 0;
    while (i < hzlen) {
        if (hzs[i] <= 0x7f)
            len += 1;
        else if (hzs[i] >= 0x800)
            len += 3;
        else
            len += 2;
        i++;
    }
    return len;
}

char* uni_utf16_to_utf8_str(wchr* hzs, int hzlen, int* mbLen)
{
    int space = uni_utf16_to_utf8_len(hzs, hzlen);
    char* utf8 = (char*)NBK_malloc(space + 1);
    char* p = utf8;
    int i = 0, n;
    
    while (i < hzlen) {
        n = uni_utf16_to_utf8(hzs[i], p);
        i++;
        p += n;
    }
    *p = 0;
    if (mbLen)
        *mbLen = space;
    return utf8;
}

wchr uni_utf8_to_utf16(uint8* mb, int8* offset)
{
    wchr hz = 0, t;
    uint8* p = mb;

    if (*p < 0x80) {
        if (*p == '&' && *(p+1) == '#') {
            p += 2;
            if (*p == 'x' || *p == 'X')
                hz = nbk_htoi((char*)p+1);
            else
                hz = NBK_atoi((char*)p);
            while (*p && *p != ';')
                p++;
            if (*p == ';')
                p++;
        }
        else {
            hz = (*p > 0x20) ? *p : 0x20;
            p++;
        }
    }
    else if (*p >= 0xe0) {
        t = *p++ & 0xf;
        t <<= 12;
        hz |= t;
        t = *p++ & 0x3f;
        t <<= 6;
        hz |= t;
        t = *p++ & 0x3f;
        hz |= t;
    }
    else {
        t = *p++ & 0x1f;
        t <<= 6;
        hz |= t;
        t = *p++ & 0x3f;
        hz |= t;
    }
    
    *offset = p - mb;
    
//    hz = N_SWAP_UINT16(hz);
    return hz;
}

int uni_utf8_to_utf16_len(uint8* mb, int len)
{
    int i, n;
    
    i = n = 0;
    while (i < len) {
        
        if (mb[i] < 0x80) {
            i++;
        }
        else if (mb[i] >= 0xe0) {
            i += 3;
        }
        else {
            i += 2;
        }
        n++;
    }
    
    return n;
}

wchr* uni_utf8_to_utf16_str(uint8* mb, int len, int* wcLen)
{
    int l = (len == -1) ? nbk_strlen((char*)mb) : len;
    int need = uni_utf8_to_utf16_len(mb, l);
    wchr* hz = (wchr*)NBK_malloc(sizeof(wchr) * (need + 1));
    uint8* p = mb;
    uint8* tooFar = p + l;
    int8 offset;
    int i = 0;
    
    while (p && p < tooFar && i < need) {
        hz[i] = uni_utf8_to_utf16(p, &offset);
        p += offset;
        i++;
    }
    hz[i] = 0;
    if (wcLen)
        *wcLen = i;
    return hz;
}
