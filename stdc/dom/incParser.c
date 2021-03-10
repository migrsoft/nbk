/*
 * incParser.c
 *
 *  Created on: 2011-3-24
 *      Author: wuyulun
 */

#include "incParser.h"

NIncParser* incParser_create(void* page)
{
    NIncParser* parser = (NIncParser*)NBK_malloc0(sizeof(NIncParser));
    N_ASSERT(parser);
    parser->page = page;
    return parser;
}

void incParser_delete(NIncParser** parser)
{
    NIncParser* p = *parser;
    NBK_free(p);
    *parser = N_NULL;
}

void incParser_reset(NIncParser* parser)
{
    if (parser->url) {
        NBK_free(parser->url);
        parser->url = N_NULL;
        parser->urlLen = parser->urlCur = 0;
    }
    
    if (parser->xpcmd) {
        NBK_free(parser->xpcmd);
        parser->xpcmd = N_NULL;
        parser->xpcmdLen = parser->xpcmdCur = 0;
    }
}

/*
 * format:
 *   header + op * x
 * header format:
 *   url len (2) + op len (4) + url (x) + op (x)
 * op format:
 *   type (2) + xpath len (2) + xml len (4) + xpath (x) + xml (x)
 * 
 */

void incParser_write(NIncParser* parser, uint8* data, int length)
{
    
}
