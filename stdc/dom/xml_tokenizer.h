/*
 * xml_tokenizer.h
 *
 *  Created on: 2010-12-25
 *      Author: wuyulun
 */

#ifndef XML_TOKENIZER_H_
#define XML_TOKENIZER_H_

#include "../inc/config.h"
#include "token.h"
#include "miParser.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NXmlTokenizer {
    
    MiParser*   parser;

    void*       doc; // not own, type is Document

    nbool        checkUrl : 1;
    nbool        checkDoctype : 1;
    nbool        check : 2;
    nbool        keep : 1;
    nbool        space : 1; // merge white space
    nbool        skip : 1;
    nbool        ignoreSpace : 1; // skip white space
    nbool        checkJs : 1;
    nbool        hasJs : 1;
    
    int16       lv; // stack level
    nid         stack[MAX_TREE_DEPTH]; // for close tag fix
    
    nid         denyBlock; // which cannot contain block-level elements
    
} NXmlTokenizer;

#ifdef NBK_MEM_TEST
int xmltkn_memUsed(const NXmlTokenizer* tokenizer);
#endif

NXmlTokenizer* xmltkn_create(void);
void xmltkn_delete(NXmlTokenizer** tokenizer);
void xmltkn_write(NXmlTokenizer* tokenizer, const char* str, int32 length, nbool final);
void xmltkn_reset(NXmlTokenizer* tokenizer);
void xmltkn_abort(NXmlTokenizer* tokenizer);

#ifdef __cplusplus
}
#endif

#endif /* XML_TOKENIZER_H_ */
