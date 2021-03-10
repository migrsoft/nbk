/*
 * strDict.h
 *
 *  Created on: 2011-1-19
 *      Author: wuyulun
 */

#ifndef STRDICT_H_
#define STRDICT_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NSDCell {
    
    char*   k;
    void*   v;
    
} NSDCell;

typedef struct _NStrDict {
    
    NSDCell*    a;
    int         use;
    int         max;
    int         grow;
    
} NStrDict;

NStrDict* strDict_create(int grow);
void strDict_delete(NStrDict** dict);
void strDict_reset(NStrDict* dict);
void* strDict_append(NStrDict* dict, char* key, void* data);
void* strDict_find(NStrDict* dict, char* key);

#ifdef __cplusplus
}
#endif

#endif /* STRDICT_H_ */
