/*
 * xpath.h
 *
 *  Created on: 2011-3-22
 *      Author: wuyulun
 */

#ifndef XPATH_H_
#define XPATH_H_

#include "../inc/config.h"

#ifdef __cplusplus
extern "C" {
#endif
    
typedef struct _NXPathCell {
    
    nid     id;
    int16   idx; // start from 1
    
} NXPathCell;

NXPathCell* xpath_parse(const char* path, int len);
void xpath_free(NXPathCell** xpath);
    
#ifdef __cplusplus
}
#endif

#endif /* XPATH_H_ */
