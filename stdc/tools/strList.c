/*
 * strList.c
 *
 *  Created on: 2011-11-22
 *      Author: wuyulun
 */

#include "../inc/config.h"
#include "strList.h"
#include "str.h"

static int get_substr_num(const char* string, int* len)
{
    int num = 1;
    char* p = (char*)string;
    
    while (*p) {
        if (*p == ',')
            num++;
        p++;
    }

    *len = p - string;
    return num;
}

static void build_list(char** lst, char* string)
{
    char* p = string;
    int i = 0;
    
    while (*p) {
        if (lst[i] == N_NULL)
            lst[i] = p;
        if (*p == ',') {
            *p = 0;
            i++;
        }
        p++;
    }
}

NStrList* strList_create(const char* string)
{
    NStrList* lst = (NStrList*)NBK_malloc0(sizeof(NStrList));
    int len;
    int num = get_substr_num(string, &len) + 1;
    
    lst->lst = (char**)NBK_malloc0(sizeof(char*) * num);
    lst->dat = (char*)NBK_malloc0(len + 1);
    nbk_strcpy(lst->dat, string);
    build_list(lst->lst, lst->dat);

    return lst;
}

void strList_delete(NStrList** slst)
{
    NStrList* lst = *slst;
    
    NBK_free(lst->lst);
    NBK_free(lst->dat);
    NBK_free(lst);
    *slst = N_NULL;
}
