/*
 * ptrArray.c
 *
 *  Created on: 2011-2-21
 *      Author: wuyulun
 */

#include "ptrArray.h"

#define PTRARRAY_GROW   20

NPtrArray* ptrArray_create(void)
{
    NPtrArray* a = (NPtrArray*)NBK_malloc0(sizeof(NPtrArray));
    N_ASSERT(a);
    return a;
}

void ptrArray_delete(NPtrArray** array)
{
    NPtrArray* a = *array;
    ptrArray_reset(a);
    NBK_free(a);
    *array = N_NULL;
}

void ptrArray_reset(NPtrArray* array)
{
    if (array->data) {
        NBK_free(array->data);
        array->data = N_NULL;
        array->use = array->max = 0;
    }
}

void ptrArray_append(NPtrArray* array, void* ptr)
{
    if (array->use == array->max) {

        array->max += PTRARRAY_GROW;
        if (array->data)
            array->data = NBK_realloc(array->data, sizeof(void*) * array->max);
        else
            array->data = NBK_malloc(sizeof(void*) * array->max);
    }
    
    array->data[array->use++] = ptr;
}

void* ptrArray_remove(NPtrArray* array, int16 index)
{
    int16 i;
    void* p;
    
    if (index < 0 || index >= array->use)
        return N_NULL;

    p = array->data[index];
    for (i=index; i < array->use - 1; i++)
        array->data[i] = array->data[i+1];
    array->use--;
    
    return p;
}
