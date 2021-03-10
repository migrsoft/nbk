/*
 * wbxmlDec.h
 *
 *  Created on: 2011-10-9
 *      Author: wuyulun
 */

#ifndef WBXMLDEC_H_
#define WBXMLDEC_H_

#include "../inc/config.h"

#ifdef __cplusplus
extern "C" {
#endif
    
typedef struct _NWbxmlDecoder {
    
    uint8*  src;
    int     srcMax;
    int     srcPos;
    
    char*   dst;
    int     dstMax;
    int     dstPos;
    
    void*   page;
    
} NWbxmlDecoder;

NWbxmlDecoder* wbxmlDecoder_create(void);
void wbxmlDecoder_delete(NWbxmlDecoder** decoder);

void wbxmlDecoder_setSourceLength(NWbxmlDecoder* decoder, int length);
void wbxmlDecoder_addSourceData(NWbxmlDecoder* decoder, uint8* data, int length);
nbool wbxmlDecoder_decode(NWbxmlDecoder* decoder);

#ifdef __cplusplus
}
#endif

#endif /* WBXMLDEC_H_ */
