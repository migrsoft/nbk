/*
 * wbxmlDec.c
 *
 *  Created on: 2011-10-9
 *      Author: wuyulun
 */

#include "wbxmlDec.h"
#include "../wbxml/wbxml.h"
#include "../wbxml/wbxml_parser.h"
#include "../tools/dump.h"
#include "../tools/str.h"

#define DEBUG_DECODER   0

#define BUFFER_GROW     4096

NWbxmlDecoder* wbxmlDecoder_create(void)
{
    NWbxmlDecoder* d = (NWbxmlDecoder*)NBK_malloc0(sizeof(NWbxmlDecoder));
    d->srcMax = -1;
    d->dstMax = -1;
    return d;
}

void wbxmlDecoder_delete(NWbxmlDecoder** decoder)
{
    NWbxmlDecoder* d = *decoder;
    if (d->src)
        NBK_free(d->src);
    if (d->dst)
        NBK_free(d->dst);
    NBK_free(d);
    *decoder = N_NULL;
}

void wbxmlDecoder_setSourceLength(NWbxmlDecoder* decoder, int length)
{
    int size = 0;
    
    if (length > 0)
        size = BUFFER_GROW * ((length / BUFFER_GROW) + 1);
    else
        size = BUFFER_GROW;
    
    decoder->src = (uint8*)NBK_malloc(size);
    decoder->dstMax = size;
    
#if DEBUG_DECODER
    dump_char(decoder->page, "init alloc", -1);
    dump_int(decoder->page, length);
    dump_int(decoder->page, decoder->srcMax);
    dump_return(decoder->page);
#endif
}

static void grow_buffer(uint8** buf, int* max, int length, void* page)
{
    int size = BUFFER_GROW * ((length * 2 / BUFFER_GROW) + 1);
    *max += size;
    *buf = (uint8*)NBK_realloc(*buf, *max);
    
#if DEBUG_DECODER
    dump_char(page, "grow buffer", -1);
    dump_int(page, *max);
    dump_return(page);
#endif
}

void wbxmlDecoder_addSourceData(NWbxmlDecoder* decoder, uint8* data, int length)
{
#if DEBUG_DECODER
    dump_char(decoder->page, "add data", -1);
    dump_int(decoder->page, length);
    dump_return(decoder->page);
#endif
    
    if (decoder->srcMax - decoder->srcPos < length)
        grow_buffer(&decoder->src, &decoder->srcMax, length, decoder->page);
    
    NBK_memcpy(&decoder->src[decoder->srcPos], data, length);
    decoder->srcPos += length;
}

static void wbxml_start_element(void *ctx, WBXMLTag *element, WBXMLAttribute **atts, WB_BOOL empty)
{
    NWbxmlDecoder* t = (NWbxmlDecoder*)ctx;
    char* tag = (char*)wbxml_tag_get_xml_name(element);
    int s, s2;
    
    s = nbk_strlen(tag);
    if (t->dstPos + s + 4 > t->dstMax)
        grow_buffer((uint8**)&t->dst, &t->dstMax, s, t->page);
    
    t->dst[t->dstPos++] = '<';
    nbk_strcpy(&t->dst[t->dstPos], tag);
    t->dstPos += s;
    
    if (atts) {
        int i = 0;
        char *name, *value;
        
        while (atts[i]) {
            name = (char*)wbxml_attribute_get_xml_name(atts[i]);
            value = (char*)wbxml_attribute_get_xml_value(atts[i]);
            i++;
            
            s = nbk_strlen(name);
            s2 = nbk_strlen(value);
            if (t->dstPos + s + s2 + 4 > t->dstMax)
                grow_buffer((uint8**)&t->dst, &t->dstMax, s + s2, t->page);
            
            t->dst[t->dstPos++] = ' ';
            nbk_strcpy(&t->dst[t->dstPos], name);
            t->dstPos += s;
            t->dst[t->dstPos++] = '=';
            t->dst[t->dstPos++] = '"';
            nbk_strcpy(&t->dst[t->dstPos], value);
            t->dstPos += s2;
            t->dst[t->dstPos++] = '"';
        }
    }
    
    if (empty)
        t->dst[t->dstPos++] = '/';
    t->dst[t->dstPos++] = '>';
}

static void wbxml_end_element(void *ctx, WBXMLTag *element, WB_BOOL empty)
{
    if (!empty) {
        NWbxmlDecoder* t = (NWbxmlDecoder*)ctx;
        char* tag = (char*)wbxml_tag_get_xml_name(element);
        int s = nbk_strlen(tag);
        
        if (t->dstPos + s + 4 > t->dstMax)
            grow_buffer((uint8**)&t->dst, &t->dstMax, s, t->page);
        
        t->dst[t->dstPos++] = '<';
        t->dst[t->dstPos++] = '/';
        nbk_strcpy(&t->dst[t->dstPos], tag);
        t->dstPos += s;
        t->dst[t->dstPos++] = '>';
    }
}

static void wbxml_characters(void *ctx, WB_UTINY *ch, WB_ULONG start, WB_ULONG length)
{
    NWbxmlDecoder* t = (NWbxmlDecoder*)ctx;
    int i;
    
    if (t->dstPos + (length - start) > t->dstMax)
        grow_buffer((uint8**)&t->dst, &t->dstMax, length - start, t->page);
    
    for (i=start; i < length; i++)
        t->dst[t->dstPos++] = ch[i];
}

nbool wbxmlDecoder_decode(NWbxmlDecoder* decoder)
{
    nbool result = N_FALSE;
    WBXMLParser *wbxml_parser = NULL;
    WBXMLContentHandler parse_handler = {
        NULL,
        NULL,
        wbxml_start_element,
        wbxml_end_element,
        wbxml_characters,
        NULL};
    
    if (decoder->srcPos <= 0)
        return N_FALSE;

    wbxml_parser = wbxml_parser_create();
    if (wbxml_parser) {
        WBXMLError ret = WBXML_OK;
        
        decoder->dstMax = decoder->srcMax * 2;
        decoder->dst = (char*)NBK_malloc(decoder->dstMax);
        decoder->dstPos = 0;
        
#if DEBUG_DECODER
        dump_char(decoder->page, "decoding...", -1);
        dump_return(decoder->page);
#endif
        wbxml_parser_set_user_data(wbxml_parser, decoder);
        wbxml_parser_set_content_handler(wbxml_parser, &parse_handler);
        ret = wbxml_parser_parse(wbxml_parser, decoder->src, decoder->srcPos);
        wbxml_parser_destroy(wbxml_parser);

#if DEBUG_DECODER
        dump_char(decoder->page, "decoded!", -1);
        dump_int(decoder->page, decoder->srcPos);
        dump_int(decoder->page, decoder->dstPos);
        dump_return(decoder->page);
#endif
        
        if (ret == WBXML_OK)
            result = N_TRUE;
    }
    
    return result;
}
