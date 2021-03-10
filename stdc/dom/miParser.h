/*
 * miParser: A mini xml/html parser develop by migr at MIC.
 *
 *  Created on: 2011-3-5
 *      Author: wuyulun
 */

#ifndef _MI_PARSER_H
#define _MI_PARSER_H

typedef unsigned char mi_uint8;
typedef unsigned short mi_uint16;

#define MI_NULL     0L

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _MIPR_MEM {
    
    void*(*malloc)(unsigned int size);
    void*(*realloc)(void* ptr, unsigned int size);
    void(*free)(void* ptr);
    
} MIPR_MEM;

typedef struct _MiParser {
    
    void*       user;
    
    MIPR_MEM    mem;
    char*       buf;
    int         buf_size;
    char*       h;      // head
    char*       t;      // tail
    
    mi_uint16   state;
    mi_uint16   comment;
    char*       text;
    char*       tag;
    char**      attrs;
    short       aidx;
    short       pass;
    
    mi_uint8    de;
    
    mi_uint8    first : 1;
    mi_uint8    close : 1;
    mi_uint8    flat : 1;
    mi_uint8    abort : 1;
    mi_uint8    skipJs : 1;
    
    void(*handle_start_tag)(void* user, const char* name, const char** attrs);
    void(*handle_end_tag)(void* user, const char* name);
    void(*handle_text)(void* user, const char* name, int length);
    
} MiParser;

MiParser* miParser_create(MIPR_MEM* mem);
void miParser_delete(MiParser** parser);
void miParser_reset(MiParser* parser);
void miParser_write(MiParser* parser, const char* data, int length);
void miParser_abort(MiParser* parser);
    
#ifdef __cplusplus
}
#endif

#endif /* _MI_PARSER_H */
