/*
 * strList.h
 *
 *  Created on: 2011-11-22
 *      Author: wuyulun
 */

#ifndef STRLIST_H_
#define STRLIST_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NStrList {
    
    char**  lst;
    char*   dat;
    
} NStrList;

NStrList* strList_create(const char* string);
void strList_delete(NStrList** slst);

#ifdef __cplusplus
}
#endif

#endif /* STRLIST_H_ */
