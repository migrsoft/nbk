/*
 * xml_tokenizer.c
 *
 *  Created on: 2010-12-25
 *      Author: wuyulun
 */

#include "../inc/config.h"
#include "xml_tokenizer.h"
#include "node.h"
#include "document.h"
#include "xml_helper.h"
#include "xml_tags.h"
#include "xml_atts.h"
#include "attr.h"
#include "../tools/dump.h"
#include "../tools/str.h"
#include "../tools/unicode.h"
#include "../loader/url.h"

#define DUMP_DATA     0 // just output data parsed

static const char* str_space = " ";

#define ISSPACE(c) ((uint8)c <= 0x20)

#ifdef NBK_MEM_TEST
int xmltkn_memUsed(const NXmlTokenizer* tokenizer)
{
    int size = 0;
    if (tokenizer)
        size = sizeof(NXmlTokenizer);
    return size;
}
#endif

static nbool is_block_level_element(nid id)
{
    switch (id) {
    case TAGID_HEAD:
    case TAGID_META:
    case TAGID_TITLE:
    case TAGID_P:
    case TAGID_DIV:
    case TAGID_UL:
    case TAGID_OL:
    case TAGID_LI:
    case TAGID_TABLE:
    case TAGID_TR:
    case TAGID_TD:
    case TAGID_OBJECT:
    case TAGID_H1:
    case TAGID_H2:
    case TAGID_H3:
    case TAGID_H4:
    case TAGID_H5:
    case TAGID_H6:
    case TAGID_FORM:
    case TAGID_HEADER:
    case TAGID_SECTION:
    case TAGID_ARTICLE:
    case TAGID_ASIDE:
    case TAGID_FOOTER:
    case TAGID_NAV:
        return N_TRUE;
    default:
        return N_FALSE;
    }
}

#if !DUMP_DATA

static void send_text(NXmlTokenizer* tokenizer, NToken* token, const char* text, int length)
{
    // convert utf-8 to utf-16
    if (text == str_space) {
        token->d.text = (wchr*)NBK_malloc0(sizeof(wchr)*2);
        token->d.text[0] = 0x20;
        token->d.text[1] = 0;
        token->len = 1;
        token->space = N_TRUE;
    }
    else {
        int wcLen = 0;
        token->d.text = uni_utf8_to_utf16_str((uint8*)text, length, &wcLen);
        token->len = wcLen;
    }
    
    doc_processToken((NDocument*)tokenizer->doc, token);
    
    if (token->d.text) // if this string has not be using, delete it.
        NBK_free(token->d.text);
}

// for select control
static void add_internal_attribute(NToken* token)
{
    if (token->id == TAGID_OPTION) {
        int i;
        NAttr* atts;
        nbool b = 0;
        for (i = 0; token->atts && token->atts[i].id; i++) {
            if (token->atts[i].id == ATTID_SELECTED)
                b = 1;
        }
        atts = (NAttr*)NBK_malloc0(sizeof(NAttr) * (i + 2));
        if (token->atts) {
            NBK_memcpy(atts, token->atts, sizeof(NAttr) * i);
            NBK_free(token->atts);
        }
        atts[i].id = ATTID_SELECTED_P;
        atts[i].d.b = b;
        token->atts = atts;
    }
}

static void close_tags(NXmlTokenizer* tokenizer, nid tagId)
{
    int16 i, j;
    NToken token;
    NDocument* doc = (NDocument*)tokenizer->doc;
    nid id;
    
    for (i = tokenizer->lv; i >= 0; i--) {
        if (tokenizer->stack[i] == tagId)
            break;
    }
    if (i >= 0) {
        for (j = tokenizer->lv; j >= i; j--) {
            id = tokenizer->stack[j];
            
            if (id == TAGID_P || id == TAGID_LI || (id >= TAGID_H1 && id <= TAGID_H6))
                tokenizer->denyBlock = 0;
            
            if (id == TAGID_HEAD)
                tokenizer->check = 1;
            
            if (id == TAGID_STYLE || id == TAGID_SCRIPT)
                tokenizer->keep = 0;
            
            if (is_block_level_element(id) || id == TAGID_BR)
                tokenizer->ignoreSpace = N_TRUE;
            
            NBK_memset(&token, 0, sizeof(NToken));
            token.id = id + TAGID_CLOSETAG;
            doc_processToken(doc, &token);
        }
        tokenizer->lv = --i;
    }
}

static void handle_end_tag(void* user, const char* name)
{
    NXmlTokenizer* tokenizer = (NXmlTokenizer*)user;
    NDocument* doc = (NDocument*)tokenizer->doc;
    
    nid id = get_tag_id(name);
    if (id == N_INVALID_ID) // not found!
        return;
    
    // translate frame to link
    if (id == TAGID_FRAMESET)
        id = TAGID_BODY;
    if (id == TAGID_FRAME)
        id = TAGID_A;
    
    close_tags(tokenizer, id);
}

static nbool check_parent_exist(NXmlTokenizer* tokenizer, nid parentId, nbool add)
{
    nbool exist = tokenizer->stack[tokenizer->lv - 1] == parentId;
    
    if (!exist && add) {
        NDocument* doc = (NDocument*)tokenizer->doc;
        NToken token;
        
        tokenizer->stack[tokenizer->lv++] = parentId;
        NBK_memset(&token, 0, sizeof(NToken));
        token.id = parentId;
        doc_processToken(doc, &token);
    }
    
    return exist;
}

static void handle_start_tag(void* user, const char* name, const char** atts)
{
    NXmlTokenizer* tokenizer = (NXmlTokenizer*)user;
    NDocument* doc = (NDocument*)tokenizer->doc;
    NToken token;
    nid id;
    nbool emuLink = N_FALSE;
    char* url = N_NULL;
    
    // decide type from !DOCTYPE
    if (!tokenizer->checkDoctype) {
        if (nbk_strfind(name, "doctype") != -1) {
            int i;
            for (i=0; atts[i]; i += 2) {
                if (   (nbk_strfind(atts[i], "/xhtml1-strict") != -1)
                    || (nbk_strfind(atts[i], "/xhtml-mobile") != -1) ) {
                    doc_modeChange(doc, NEDOC_XHTML);
                    break;
                }
                else if (nbk_strfind(atts[i], "wml") != -1) {
                    doc_modeChange(doc, NEDOC_WML);
                    break;
                }
                else if (nbk_strfind(atts[i], "/xhtml1-transitional") != -1) {
                    doc_modeChange(doc, NEDOC_HTML);
                    break;
                }
            }

            //if (doc->type != NEDOC_XHTML && doc->type != NEDOC_WML)
            //    doc_modeChange(doc, NEDOC_HTML);
            
            tokenizer->checkUrl = 1;
            tokenizer->checkDoctype = 1;
        }
    }
    
    id = get_tag_id(name);
    if (id == N_INVALID_ID) // not found!
        return;

    // decide type from TAG
    if (!tokenizer->check && id == TAGID_BODY) {
        tokenizer->checkUrl = 1;
        tokenizer->checkDoctype = 1;
        tokenizer->check = 1;
    }
    if (!tokenizer->check && id == TAGID_WML) {
        doc_modeChange(doc, NEDOC_WML);
        tokenizer->checkUrl = 1;
        tokenizer->checkDoctype = 1;
        tokenizer->check = 1;
    }
    if (id == TAGID_HTML) {
        tokenizer->checkUrl = 1;
        tokenizer->checkDoctype = 1;
    }
    
    if (tokenizer->check == 1) {
        tokenizer->check = 2;
        if (doc->type != NEDOC_WML && tokenizer->checkJs && tokenizer->hasJs)
            doc->type = NEDOC_HTML;
        doc_acceptType(doc);
    }

    tokenizer->skip = 0;

    // translate frame to link
    if (id == TAGID_FRAMESET) {
        id = TAGID_BODY;
        tokenizer->check = 1;
        doc->type = NEDOC_XHTML;
    }
    else if (id == TAGID_FRAME) {
        id = TAGID_A;
        emuLink = N_TRUE;
    }
    
    // p and h1-h6 don't contain block-level element
    if (tokenizer->denyBlock && is_block_level_element(id)) {
        close_tags(tokenizer, tokenizer->denyBlock);
    }
    if (id == TAGID_P || id == TAGID_LI || (id >= TAGID_H1 && id <= TAGID_H6)) {
        tokenizer->denyBlock = id;
    }
    if (id == TAGID_A || id == TAGID_OPTION) {
        close_tags(tokenizer, id);
    }
    
    if (!node_is_single(id))
        tokenizer->stack[++tokenizer->lv] = id;
    
    if (id == TAGID_TR && !check_parent_exist(tokenizer, TAGID_TABLE, N_FALSE) ) {
        check_parent_exist(tokenizer, TAGID_TABLE, N_TRUE);
        tokenizer->stack[tokenizer->lv] = id;
    }
    if (id == TAGID_TD) {
        if (!check_parent_exist(tokenizer, TAGID_TR, N_FALSE)) {
            if (!check_parent_exist(tokenizer, TAGID_TABLE, N_FALSE))
                check_parent_exist(tokenizer, TAGID_TABLE, N_TRUE);
            check_parent_exist(tokenizer, TAGID_TR, N_TRUE);
            tokenizer->stack[tokenizer->lv] = id;
        }
    }
    
    if (tokenizer->space) {
        if (is_block_level_element(id) || id == TAGID_BR) {
            tokenizer->ignoreSpace = N_TRUE;
        }
        else {
            NBK_memset(&token, 0, sizeof(NToken));
            token.id = TAGID_TEXT;
            send_text(tokenizer, &token, str_space, 1);
        }
        tokenizer->space = 0;
    }
    else if (!is_block_level_element(id) && id != TAGID_BR)
        tokenizer->ignoreSpace = N_FALSE;
    
    if (id == TAGID_STYLE || id == TAGID_SCRIPT) {
        tokenizer->keep = 1;
        if (NBK_ENABLE_CBS && id == TAGID_SCRIPT)
            tokenizer->hasJs = 1;
    }

    NBK_memset(&token, 0, sizeof(NToken));
    token.id = id;
    token.atts = attr_create(doc->spool, atts);
    add_internal_attribute(&token);
    
    if (emuLink && token.atts)
        url = attr_getValueStr(token.atts, ATTID_SRC);
    
    doc_processToken(doc, &token);
    
    if (emuLink && url) {
        NToken fakeToken;
        NBK_memset(&fakeToken, 0, sizeof(NToken));
        fakeToken.id = TAGID_TEXT;
        send_text(tokenizer, &fakeToken, url, nbk_strlen(url));
    }
    
    if (token.atts)
        attr_delete(&token.atts);
}

static char* parse_text(char* text, int length, int* newLen)
{
    char* p = text;
    char* toofar = p + length;
    char* start;
    char* q = N_NULL;
    
    while (p < toofar) {
        if (ISSPACE(*p))
            *p = ' ';
        else
            break;
        p++;
    }
    if (p == toofar)
        return N_NULL; // all space
    
    start = (p > text) ? p - 1 : p; // if begin with space, keep only one
    
    // join spaces between words
    q = p - 1; // fixme
    while (p < toofar) {
        if (ISSPACE(*p)) {
            *p = ' ';
            if (*q != ' ')
                q++;
        }
        else {
            if (p - q > 1) {
                *++q = *p;
                *p = ' '; // remove char copied
            }
            else
                q++;
        }
        p++;
    }
    if (q < p)
        q++; // keep last one space if have
    
    *newLen = q - start;
    return start;
}

static void handle_text(void* user, const char* text, int length)
{
    NXmlTokenizer* tokenizer = (NXmlTokenizer*)user;
    NDocument* doc = (NDocument*)tokenizer->doc;
    NToken token;
    
    if (tokenizer->skip)
        return;
    
    NBK_memset(&token, 0, sizeof(NToken));
    token.id = TAGID_TEXT;

    if (tokenizer->keep) {
        token.d.str = (char*)text;
        token.len = length;
        doc_processToken((NDocument*)(tokenizer->doc), &token);
    }
    else {
        int len;
        char* t = parse_text((char*)text, length, &len);
        if (t == N_NULL)
            tokenizer->space = (tokenizer->ignoreSpace) ? 0 : 1;
        else
            send_text(tokenizer, &token, t, len);
    }
}

#else

static void handle_start_tag(void* user, const char* name, const char** atts)
{
    NXmlTokenizer* tokenizer = (NXmlTokenizer*)user;
    NDocument* doc = (NDocument*)tokenizer->doc;
    int i;
	nid id = get_tag_id(name);
    dump_char(doc->page, (char*)name, -1);
    dump_char(doc->page, ">>>", 3);
    dump_return(doc->page);
    for (i=0; atts[i]; i += 2) {
        dump_char(doc->page, " ", 1);
        dump_char(doc->page, (char*)atts[i], -1);
        if (atts[i+1])
            dump_char(doc->page, (char*)atts[i+1], -1);
        dump_return(doc->page);
    }
}

static void handle_end_tag(void* user, const char* name)
{
    NXmlTokenizer* tokenizer = (NXmlTokenizer*)user;
    NDocument* doc = (NDocument*)tokenizer->doc;
	nid id = get_tag_id(name);
    dump_char(doc->page, (char*)name, -1);
    dump_char(doc->page, "<<<", 3);
    dump_return(doc->page);
}

static void handle_text(void* user, const char* text, int length)
{
    NXmlTokenizer* tokenizer = (NXmlTokenizer*)user;
    NDocument* doc = (NDocument*)tokenizer->doc;
    uint8* p = (uint8*)text;
    int i;
    for (i=0; i < length; i++)
        if (p[i] == 0x1)
            p[i] = '1';
        else if (p[i] < 0x20)
            p[i] = '@';
        else if (p[i] == 0x20)
            p[i] = '_';
    dump_char(doc->page, "#text", -1);
    dump_int(doc->page, length);
    dump_char(doc->page, (char*)text, length);
    dump_return(doc->page);
}

#endif

NXmlTokenizer* xmltkn_create()
{
    NXmlTokenizer* tokenizer;
    MIPR_MEM mem;
    
    tokenizer = NBK_malloc0(sizeof(NXmlTokenizer));
    N_ASSERT(tokenizer);
        
    mem.malloc = NBK_malloc;
    mem.realloc = NBK_realloc;
    mem.free = NBK_free;
    tokenizer->parser = miParser_create(&mem);
    return tokenizer;
}

void xmltkn_delete(NXmlTokenizer** tokenizer)
{
    NXmlTokenizer* t = *tokenizer;
    miParser_delete(&t->parser);
    NBK_free(t);
    *tokenizer = N_NULL;
}

void xmltkn_write(NXmlTokenizer* tokenizer, const char* str, int32 length, nbool final)
{
    miParser_write(tokenizer->parser, str, length);
}

void xmltkn_reset(NXmlTokenizer* tokenizer)
{
    tokenizer->checkUrl = 0;
    tokenizer->checkDoctype = 0;
    tokenizer->check = 0;
    tokenizer->keep = 0;
    tokenizer->space = 0;
    tokenizer->ignoreSpace = 1;
    tokenizer->skip = 1;
    tokenizer->denyBlock = 0;
    tokenizer->lv = -1;
    tokenizer->hasJs = 0;

#if NBK_ENABLE_CBS
    tokenizer->checkJs = 1;
#else
    tokenizer->checkJs = 0;
#endif

    miParser_reset(tokenizer->parser);
    tokenizer->parser->user = tokenizer;
    tokenizer->parser->handle_start_tag = handle_start_tag;
    tokenizer->parser->handle_end_tag = handle_end_tag;
    tokenizer->parser->handle_text = handle_text;
}

void xmltkn_abort(NXmlTokenizer* tokenizer)
{
    miParser_abort(tokenizer->parser);
}
