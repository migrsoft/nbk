/*
 * token.h
 *
 *  Created on: 2010-12-25
 *      Author: wuyulun
 */

#ifndef TOKEN_H_
#define TOKEN_H_

#include "../inc/config.h"
#include "attr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NToken {
    
    uint16  id;
    int16   len;
    
    union {
        wchr*   text;
        char*   str;
    } d;
    
    NAttr*  atts;
    
    // reference to origial node
    void*   nodeRef;
    
    nbool    space : 1;
    
} NToken;
    
#ifdef __cplusplus
}
#endif

#endif /* TOKEN_H_ */
