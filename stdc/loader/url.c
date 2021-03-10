/*
 * url.c
 *
 *  Created on: 2011-2-16
 *      Author: wuyulun
 */

#include "url.h"
#include "../inc/nbk_limit.h"
#include "../tools/str.h"
#include "../tools/strList.h"
#include "../dom/char_ref.h"
#include "../dom/xml_helper.h"

#define MI_MAX_REF_LEN  8

#define IS_HEX(c) ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))

//#define WL_VER_LEN      4
//#define WL_ITEM_LEN     16

//NStrList* l_phoneSite = N_NULL;

//typedef struct _NWapWhiteList {
//    uint8*  dat;
//    int     num;
//} NWapWhiteList;
//
//static NWapWhiteList* l_wapWhiteList = N_NULL;

//static int compare_md5(uint8* v1, uint8* v2)
//{
//    int i;
//    for (i=0; i < WL_ITEM_LEN; i++) {
//        if (v1[i] < v2[i])
//            return -1;
//        else if (v1[i] > v2[i])
//            return 1;
//    }
//    return 0;
//}

//static void wap_wl_sort(uint8* lst, int num)
//{
//    uint8 t[16];
//    int i, j;
//    
//    for (i=0; i < num - 1; i++) {
//        for (j = i + 1; j < num; j++) {
//            if (compare_md5(&lst[j * 16], &lst[i * 16]) < 0) {
//                NBK_memcpy(t, &lst[j * 16], 16);
//                NBK_memcpy(&lst[j * 16], &lst[i * 16], 16);
//                NBK_memcpy(&lst[i * 16], t, 16);
//            }
//        }
//    }
//}

//uint8* url_getWL(int* num)
//{
//    if (l_wapWhiteList) {
//        *num = l_wapWhiteList->num;
//        return l_wapWhiteList->dat;
//    }
//    return N_NULL;
//}

//void url_initPhoneSite(void)
//{
//    const char* sites = "//m.,//3g.,//wap.,//i.";
//    l_phoneSite = strList_create(sites);
//}
//
//void url_delPhoneSite(void)
//{
//    strList_delete(&l_phoneSite);
//}
//
//nbool url_checkPhoneSite(const char* url)
//{
//    int i;
//    char* p;
//    
//    for (i=0; l_phoneSite->lst[i]; i++) {
//        if (nbk_strfind(url, l_phoneSite->lst[i]) != -1)
//            return N_TRUE;
//    }
//    
//    i = nbk_strfind(url, "://");
//    if (i > 0) {
//        p = (char*)url + i + 3;
//        i = 0;
//        while (*p && *p != '/') {
//            if (*p == '.')
//                i++;
//            p++;
//        }
//        if (i == 1) // ex. http://sina.cn/
//            return N_TRUE;
//    }
//    
//    return N_FALSE;
//}

//void url_parseWapWhiteList(uint8* data, int len)
//{
//    // wap white list format:
//    // version [4 bytes]
//    // url md5 string [16 bytes]
//    // ...
//    
//    int num = (len - WL_VER_LEN) / WL_ITEM_LEN;
//    
//    url_freeWapWhiteList();
//    
//    if (num > 0) {
//        l_wapWhiteList = (NWapWhiteList*)NBK_malloc0(sizeof(NWapWhiteList));
//        l_wapWhiteList->num = num;
//        l_wapWhiteList->dat = (uint8*)NBK_malloc(WL_ITEM_LEN * num);
//        NBK_memcpy(l_wapWhiteList->dat, data + WL_VER_LEN, WL_ITEM_LEN * num);
//        
////        wap_wl_sort(l_wapWhiteList->dat + WL_VER_LEN, num);
//    }
//}
//
//void url_freeWapWhiteList()
//{
//    if (l_wapWhiteList) {
//        NBK_free(l_wapWhiteList->dat);
//        NBK_free(l_wapWhiteList);
//        l_wapWhiteList = N_NULL;
//    }
//}
//
//nbool url_inWapWhiteList(const char* url)
//{
//    char* p = (char*)url;
//    char* q;
//    int s, l, r, m;
//    uint8 md5[16];
//    uint8* lst;
//    
//    if (l_wapWhiteList == N_NULL)
//        return N_FALSE;
//    
//    s = nbk_strfind(url, "://");
//    if (s < 0)
//        return N_FALSE;
//    
//    p += s + 3;
//    q = p;
//    while (*q && *q != '/')
//        q++;
//    
//    NBK_md5(p, q - p, md5);
//    
//    l = 0;
//    r = l_wapWhiteList->num - 1;
//    lst = l_wapWhiteList->dat;
//    while (l <= r) {
//        m = (l + r) / 2;
//        s = compare_md5(md5, &lst[m * WL_ITEM_LEN]);
//        if (s == 0)
//            return N_TRUE;
//        else if (s > 0)
//            l = m + 1;
//        else
//            r = m - 1;
//    }
//    
//    return N_FALSE;
//}

static nbool url_has_protocol(const char* url)
{
    char* p = (char*)url;
    int i = 0;
    
    while (p && i <= 5 && *p) {
        
        if (*p == ':' && *(p+1) == '/' && *(p+2) == '/')
            return N_TRUE;
        p++;
        i++;
    }
    
    return N_FALSE;
}

void url_unescape(char* dst, const char* src, nbool forUrl)
{
    char* p = (char*)src;
    char* q;
    char* tooFar;
    int i;
    nid id;
    
    tooFar = p + nbk_strlen(src) - 1;
    while (tooFar >= p && *tooFar == ' ')
        tooFar--;
    
    while (p <= tooFar) {
        if (*p == '&') {
            i = 0;
            q = p;
            while (*q && i < MI_MAX_REF_LEN && *q != ';') {
                q++;
                i++;
            }
            if (i < MI_MAX_REF_LEN && *q == ';') {
                *q = 0;
                id = binary_search_id(xml_getCharrefNames(), CHARREF_TOTAL, p+1);
                *q = ';';
                if (id != N_INVALID_ID) {
                    nbk_strcpy(dst, xml_getEntityNames()[id]);
                    dst += nbk_strlen(xml_getEntityNames()[id]);
                    p = q + 1;
                    continue;
                }
            }
        }
        else if (forUrl && *p == ' ') {
            nbk_strcpy(dst, "%20");
            dst += 3;
            p++;
            continue;
        }
        else if (forUrl && (uint8)*p < 0x20) {
            p++;
            continue;
        }
        *dst++ = *p++;
    }
    *dst = 0;
}

char* url_parse(const char* base, const char* url)
{
    char *url1, *url2;
    char *p, *q, *tooFar, *root, *b;
    
    if (!url_has_protocol(base) && !url_has_protocol(url))
        return N_NULL;
    
    url1 = (char*)NBK_malloc(NBK_MAX_URL_LEN);
    
    if (url_has_protocol(url)) {
        url_unescape(url1, url, N_TRUE);
        return url1;
    }
    
    nbk_strcpy(url1, base);
    
    // find root path
    p = url1;
    root = N_NULL;
    while (*p) {
        
        if (*p == ':' && *(p+1) == '/') // ://
            p += 3;
        else if (*p == '/') {
            root = p;
            break;
        }
        else if (*p == '?') // query
            break;
        else
            p++;
    }
    if (root == N_NULL) {
        *p = '/';
        root = p;
        *++p = 0;
    }
    
    if (*url == '/') {
        // begin with "/"
        nbk_strcpy(root, url);
    }
    else {
        tooFar = url1 + NBK_MAX_URL_LEN - 1;
        p = (char*)url;
        
        // find the last path
        q = url1;
        b = N_NULL;
        while (*q && *q != '?') {
            if (*q == '/')
                b = q;
            q++;
        }
        q = b;
        
        while (*p && q < tooFar) {
            
            if (*p == '.' && *(p+1) == '/') {
                p += 2;
            }
            else if (*p == '.' && *(p+1) == '.') {
                if (*q == '/' && q > root)
                    q--;
                while (*q != '/' && q > root)
                    q--;
                p += 3;
            }
            else {
                *++q = *p++;
            }
        }
        *++q = 0;
    }
    
    url2 = (char*)NBK_malloc(NBK_MAX_URL_LEN);
    url_unescape(url2, url1, N_TRUE);
    NBK_free(url1);
    
    return url2;
}

char* url_hex_to_str(char* url, char* value)
{
    uint8* p = (uint8*)url;
    uint8* v = (uint8*)value;
    int8 h;
    
    while (*v) {
        if (*v == 0x20) {
            *p++ = '+';
        }
        else if (*v == '&' && *(v+1) == 'a' && *(v+2) == 'm' && *(v+3) == 'p' && *(v+4) == ';') {
            *p++ = '%';
            *p++ = '2';
            *p++ = '6';
            v += 4;
        }
        else if (  (*v >= '0' && *v <= '9')
                || (*v >= 'a' && *v <= 'z')
                || (*v >= 'A' && *v <= 'Z')
                || *v == '$' || *v == '+' || *v == '-' || *v == '*'
                || *v == '.' || *v == ',' || *v == '!' || *v == '\''
                || *v == '(' || *v == ')' || *v == '_')
        {
            *p++ = *v;
        }
        else {
            if (*v == 0xa) {
                *p++ = '%';
                *p++ = '0';
                *p++ = 'd';
            }
            *p++ = '%';
            h = *v >> 4 & 0xf;
            *p++ = (h < 10) ? '0' + h : 'a' + h - 10;
            h = *v & 0xf;
            *p++ = (h < 10) ? '0' + h : 'a' + h - 10;
        }
        v++;
    }
    *p = 0;
    return (char*)p;
}

static char htoi_c(char ch)
{
    char c = 0;
    if (ch >= '0' && ch <= '9')
        c = ch - '0';
    else if (ch >= 'a' && ch <= 'f')
        c = ch - 'a' + 10;
    else if (ch >= 'A' && ch <= 'F')
        c = ch - 'A' + 10;
    return c;
}

char* url_str_to_hex(char* url, char* str)
{
    char* p = url;
    char* q = str;
    
    while (*q) {
        
        if (*q == '%' && *(q+1) == '%') {
            *p++ = *q++;
            q++;
        }
        else if (*q == '%' && IS_HEX(*(q+1)) && IS_HEX(*(q+2))) {
            char c;
            c = htoi_c(*++q) * 16;
            c += htoi_c(*++q);
            q++;
            *p++ = c;
        }
        else {
            *p++ = *q++;
        }
    }
    *p++ = 0;
    return p;
}

nbool url_hasQuery(const char* url)
{
    char* p = (char*)url + nbk_strlen(url) - 1;
    while (p > url && *p != '/') {
        if (*p == '?')
            return N_TRUE;
        p--;
    }
    return N_FALSE;
}

char* url_parseHostPort(char* uri, uint16* port)
{
	char* p = uri;
	char* h;
	char* host = N_NULL;

    N_ASSERT(uri);

	*port = 80;

	while (*p && *p != ':')
		p++;

	if (*p && *(p+1) == '/' && *(p+2) == '/') {
		p += 3;
		h = p;

		while (*p && *p != ':' && *p != '/' && *p != '?')
			p++;

		if (p - h > 3) {
			if (*p && *p == ':')
				*port = NBK_atoi(p + 1);

			host = (char*)NBK_malloc(p - h + 1);
			nbk_strncpy(host, h, p - h);
		}
	}

	return host;
}

char* url_getFileName(char* uri)
{
	int i;
	char *p, *q;

	i = nbk_strfind(uri, "://");
	p = uri + i + 3;
	while (*p && *p != '/') p++;
	if (*p == 0)
		return N_NULL;
	q = ++p;
	while (*p) {
		if (*p == '/')
			q = p + 1;
		p++;
	}

	if (*p == 0 && p > q) {
		p = q;
		while (*p && *p != '?') p++;
		if (p > q) {
			char* f = (char*)NBK_malloc(p - q + 1);
			nbk_strncpy(f, q, p - q);
			return f;
		}
	}
	return N_NULL;
}
