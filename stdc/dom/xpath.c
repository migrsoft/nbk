/*
 * xpath.c
 *
 *  Created on: 2011-3-22
 *      Author: wuyulun
 */

#include "xpath.h"
#include "xml_helper.h"
#include "xml_tags.h"
#include "../tools/str.h"

NXPathCell* xpath_parse(const char* path, int len)
{
    char* p = (char*)path;
    char* tooFar = (len == -1) ? (char*)N_MAX_UINT : p + len;
    char* q;
    NXPathCell* xpath;
    int i;
    nid id;
    
    i = 0;
    while (*p && p < tooFar) {
        if (*p == '/')
            i++;
        p++;
    }
    if (i == 0)
        return N_NULL;
    
    // xpath sample: /div[1]/div[5]/p[2]/a[1]
    
    xpath = (NXPathCell*)NBK_malloc0(sizeof(NXPathCell) * (i + 1));
    i = -1;
    p = (char*)path;
    while (*p && p < tooFar) {
        
        if (*p == '/') {
            // for node name
            q = ++p;
            while (*p != '[') p++;
            *p = 0;
            id = binary_search_id(xml_getTagNames(), TAGID_LASTTAG, q);
            *p = '[';
            if (id == N_INVALID_ID) {
                i = -1;
                break;
            }
            i++;
            xpath[i].id = id;
            
            // for index
            q = ++p;
            while (*p != ']') p++;
            *p = 0;
            xpath[i].idx = NBK_atoi(q);
            *p = ']';
        }
        p++;
    }
    
    if (i == -1) {
        NBK_free(xpath);
        xpath = N_NULL;
    }
    
    return xpath;
}

void xpath_free(NXPathCell** xpath)
{
    NXPathCell* x = (NXPathCell*)*xpath;
    NBK_free(x);
    *xpath = N_NULL;
}
