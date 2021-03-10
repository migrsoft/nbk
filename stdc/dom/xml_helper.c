/*
 * xml_helper.c
 *
 *  Created on: 2010-12-26
 *      Author: wuyulun
 */

#include "xml_helper.h"
#include "xml_tags.h"
#include "xml_atts.h"
#include "../tools/str.h"
#include "../inc/nbk_limit.h"

nid binary_search_id(const char** set, nid last, const char* name)
{
    nid l = 1, r = last, m;
    int rt;
    
    while (l <= r) {
        m = (l + r) / 2;
        rt = nbk_strcmp(set[m], name);
        if (rt == 0)
            return m;
        else if (rt < 0)
            l = m + 1;
        else
            r = m - 1;
    }
    
    return N_INVALID_ID; // not found
}

nid get_tag_id(const char* tag_name)
{
    return binary_search_id(xml_getTagNames(), TAGID_LASTTAG, tag_name);
}

nid get_att_id(const char* att_name)
{
    return binary_search_id(xml_getAttNames(), ATTID_LASTATT, att_name);
}
