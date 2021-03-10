/*
 * strDict.c
 *
 *  Created on: 2011-1-19
 *      Author: wuyulun
 */

#include "../inc/config.h"
#include "strDict.h"
#include "str.h"

NStrDict* strDict_create(int grow)
{
    NStrDict* dict = (NStrDict*)NBK_malloc0(sizeof(NStrDict));
    N_ASSERT(dict);
    
    dict->grow = (grow > 0) ? grow : 10;
    return dict;
}

void strDict_delete(NStrDict** dict)
{
    NStrDict* d = *dict;
    strDict_reset(d);
    NBK_free(d);
    *dict = N_NULL;
}

void strDict_reset(NStrDict* dict)
{
    if (dict->a) {
        NBK_free(dict->a);
        dict->a = N_NULL;
    }
    dict->use = 0;
    dict->max = 0;
    dict->grow = 10;
}

// return non null if data has not be used.
void* strDict_append(NStrDict* dict, char* key, void* data)
{
    int l, r, m, rt;
    void* found = N_NULL;
    
    if (dict->max == 0) {
        dict->a = (NSDCell*)NBK_malloc0(sizeof(NSDCell) * dict->grow);
        dict->max = dict->grow;
    }
    else if (dict->use == dict->max) {
        NSDCell* t = dict->a;
        dict->max += dict->grow;
        dict->a = (NSDCell*)NBK_malloc0(sizeof(NSDCell) * dict->max);
        NBK_memcpy(dict->a, t, sizeof(NSDCell) * dict->use);
        NBK_free(t);
    }
    
    if (dict->use == 0) {
        dict->a[0].k = key;
        dict->a[0].v = data;
        dict->use++;
    }
    else {
        l = 0;
        r = dict->use - 1;
        while (l <= r) {
            m = (l + r) / 2;
            rt = nbk_strcmp(dict->a[m].k, key);
            if (rt == 0) {
                found = dict->a[m].v;
                break;
            }
            else if (rt < 0)
                l = m + 1;
            else
                r = m - 1;
        }

        if (!found) {
            int i, j;
            j = (rt < 0) ? m + 1 : m;
            for (i = dict->use - 1; i >= j; i--) {
                dict->a[i + 1].k = dict->a[i].k;
                dict->a[i + 1].v = dict->a[i].v;
            }
            
            dict->a[j].k = key;
            dict->a[j].v = data;
            dict->use++;
        }
    }
    
    return found;
}

void* strDict_find(NStrDict* dict, char* key)
{
    int l, r, m, rt;
    
    l = 0;
    r = dict->use - 1;
    while (l <= r) {
        m = (l + r) / 2;
        rt = nbk_strcmp(dict->a[m].k, key);
        if (rt == 0)
            return dict->a[m].v;
        else if (rt < 0)
            l = m + 1;
        else
            r = m - 1;
    }
    return N_NULL;
}
