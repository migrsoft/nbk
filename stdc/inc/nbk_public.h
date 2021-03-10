/*
 * nbk_public.h
 *
 *  Created on: 2011-11-14
 *      Author: wuyulun
 */

#ifndef NBK_PUBLIC_H_
#define NBK_PUBLIC_H_

#ifdef __cplusplus
extern "C" {
#endif
    
enum NEPubDataType {
    NEPUBDATA_UNKNOWN,
    NEPUBDATA_NODE,
    NEPUBDATA_ATTR
};

typedef struct _NBK_NODE {
    nid     type;
    uint16  opt;
    void*   p;
} NBK_NODE;

#ifdef __cplusplus
}
#endif

#endif /* NBK_PUBLIC_H_ */
