/*
 * ptrArray.h
 *
 *  Created on: 2011-2-21
 *      Author: wuyulun
 */

#ifndef PTRARRAY_H_
#define PTRARRAY_H_

#include "../inc/config.h"

#ifdef __cplusplus
extern "C" {
#endif
    
typedef struct _NPtrArray {
    
    void**  data;
    int16   use;
    int16   max;
    
} NPtrArray;

NPtrArray* ptrArray_create(void);
void ptrArray_delete(NPtrArray** array);
void ptrArray_reset(NPtrArray* array);

void ptrArray_append(NPtrArray* array, void* ptr);
void* ptrArray_remove(NPtrArray* array, int16 index);

#ifdef __cplusplus
}
#endif

#endif /* PTRARRAY_H_ */
