/*
 * Increasement Data Parser
 *
 *  Created on: 2011-3-24
 *      Author: wuyulun
 */

#ifndef INCPARSER_H_
#define INCPARSER_H_

#include "../inc/config.h"

#ifdef __cplusplus
extern "C" {
#endif
    
enum NEIncPrCmdType {
    
    NEIPCT_DELETE,
    NEIPCT_ADD,
    NEIPCT_REPLACE
};

typedef struct _NIncParser {
    
    void*   page;
    
    char*   url;
    int16   urlLen;
    int16   urlCur;
    
    nid     cmdType;
    
    char*   xpcmd;
    int16   xpcmdLen;
    int16   xpcmdCur;
    
    char*   buf;
    int16   bufMax;
    int16   bufCur;
    
} NIncParser;

NIncParser* incParser_create(void* page);
void incParser_delete(NIncParser** parser);

void incParser_reset(NIncParser* parser);
void incParser_write(NIncParser* parser, uint8* data, int length);
    
#ifdef __cplusplus
}
#endif

#endif /* INCPARSER_H_ */
