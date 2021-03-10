/*
 * document.c
 *
 *  Created on: 2010-12-25
 *      Author: wuyulun
 */

#include "../inc/config.h"
#include "../inc/nbk_limit.h"
#include "../inc/nbk_callback.h"
#include "../inc/nbk_cbDef.h"
#include "../inc/nbk_settings.h"
#include "../inc/nbk_timer.h"
#include "../inc/nbk_ctlPainter.h"
#include "document.h"
#include "xml_tags.h"
#include "xml_atts.h"
#include "page.h"
#include "history.h"
#include "attr.h"
#include "../css/cssSelector.h"
#include "../css/css_val.h"
#include "../loader/url.h"
#include "../loader/loader.h"
#include "../tools/str.h"
#include "../tools/dump.h"
#include "../tools/unicode.h"
#include "../render/imagePlayer.h"
#include "../render/renderNode.h"
#include "../render/renderInput.h"
#include "../render/renderSelect.h"
#include "../render/renderTextarea.h"
#include "../render/renderText.h"
#include "../render/renderBlock.h"
#include "../cpconv/conv.h"

#define DUMP_FUNCTIONS      1
#define DUMP_DOM_TREE       0
#define DUMP_TOKEN          0

#define CREATE_RENDER_TREE  1
#define DISABLE_LAYOUT      0

#define DEBUG_INC_OP        0

#define WIDTH_FOR_MOBILE    360

#define MAX_TREE_DEPTH_LIMIT    (MAX_TREE_DEPTH - 2)

#ifdef NBK_MEM_TEST

typedef struct _NDocMemChecker {
    int     nodeTotal;
    int     nodeSize;
    int     imgTotal;
    int     textTotal;
    int     textSize;
    int     inlineStyleTotal;
    int     inlineStyleSize;
    int     renderTotal;
    int     renderSize;
} NDocMemChecker;

int doc_mem_used_checker(NNode* node, void* user, nbool* ignore)
{
    NDocMemChecker* chk = (NDocMemChecker*)user;
    int txtLen, styLen;
    
    chk->nodeTotal++;
    chk->nodeSize += node_memUsed(node, &txtLen, &styLen);
    
    if (node->id == TAGID_IMG)
        chk->imgTotal++;
    
    if (node->id == TAGID_TEXT) {
        chk->textTotal++;
        chk->textSize += txtLen;
    }
    
    if (styLen > 0) {
        chk->inlineStyleTotal++;
        chk->inlineStyleSize += styLen;
    }
    
    if (node->render) {
        chk->renderTotal++;
        chk->renderSize += renderNode_memUsed((NRenderNode*)node->render);
    }
    
    return 0;
}

int doc_memUsed(const NDocument* doc)
{
    int size = 0;
    if (doc) {
        size = sizeof(NDocument);
        size += xmltkn_memUsed(doc->tokenizer);
        
        if (doc->root) {
            NDocMemChecker chk;
            NPage* page = (NPage*)doc->page;
            
            NBK_memset(&chk, 0, sizeof(NDocMemChecker));
            node_traverse_depth(doc->root, doc_mem_used_checker, N_NULL, &chk);
            
            dump_char(page, "node count:", -1);
            dump_int(page, chk.nodeTotal);
            dump_return(page);
            
            dump_char(page, "text count:", -1);
            dump_int(page, chk.textTotal);
            dump_return(page);
            
            dump_char(page, "text total size:", -1);
            dump_int(page, chk.textSize);
            dump_return(page);
            
            dump_char(page, "image count:", -1);
            dump_int(page, chk.imgTotal);
            dump_return(page);
            
            dump_char(page, "inline style count:", -1);
            dump_int(page, chk.inlineStyleTotal);
            dump_return(page);
            
            dump_char(page, "inline style total size:", -1);
            dump_int(page, chk.inlineStyleSize);
            dump_return(page);
            
            dump_char(page, "dom total size:", -1);
            dump_int(page, chk.nodeSize);
            dump_return(page);

            dump_char(page, "render count:", -1);
            dump_int(page, chk.renderTotal);
            dump_return(page);
            
            dump_char(page, "rtree total size:", -1);
            dump_int(page, chk.renderSize);
            dump_return(page);
            
            size += chk.nodeSize + chk.renderSize;
            
            if (doc->ownStrPool) {
                int s = strPool_memUsed(doc->spool);
                
                dump_char(page, "string total size:", -1);
                dump_int(page, s);
                dump_return(page);
                
                size += s;
            }
        }
    }
    return size;
}
#endif

// -----------------------------------------------------------------------------
// 文档处理
// -----------------------------------------------------------------------------

static void doc_do_load(void* user)
{
    NLoadCmd* cmd = (NLoadCmd*)user;
    NPage* page = (NPage*)cmd->doc->page;

	if (cmd->body) {
        // Post with standard protocol
        page_loadUrlInternal(page, cmd->url, cmd->enc, cmd->body, &cmd->files, cmd->submit);
    }
    else if (cmd->via == NEREV_TF) {
        page->settings->via = NEREV_TF;
        page_loadUrlStandard(page, cmd->url, 0, N_NULL, N_NULL);
    }
    else {
        page_loadUrlInternal(page, cmd->url, 0, N_NULL, N_NULL, cmd->submit);
    }

	smartptr_release(cmd);
}

static char* get_onevent_go_url(NNode* card, char* typeStr)
{
    char* url = N_NULL;
    nid tags[] = {TAGID_ONEVENT, 0};
    NSList* lst = node_getListByTag(card, tags);
    NNode* n = (NNode*)sll_first(lst);
    while (n) {
        char* type = attr_getValueStr(n->atts, ATTID_TYPE);
        if (type && nbk_strcmp(type, typeStr) == 0)
            break;
        n = (NNode*)sll_next(lst);
    }
    sll_delete(&lst);
    if (n) {
        if ((n = node_getByTag(n, TAGID_GO)) != N_NULL) {
            url = attr_getValueStr(n->atts, ATTID_HREF);
        }
    }
    return url;
}

static nbool doc_do_redirect(NDocument* doc)
{
    NPage* page = (NPage*)doc->page;
    
    if (doc->delayLoad > 0) {
        int delay = doc->delayLoad;
        
        doc->delayLoad = -1;
        
        if (history_isLastPage((NHistory*)page->history)) {
            // must be the last in history
            char* base = doc_getUrl(doc);
            char* url = N_NULL;
            nbool jump = N_FALSE;
            
            //dump_char(page, "enter", -1);
            //dump_char(page, base, -1);
            //dump_return(page);
            //doc_dumpDOM(doc);

            if (doc->delayLoadType == NEREDIR_META_REFRESH) {
                url = doc_getUrl(doc);
                jump = N_TRUE;
            }
            else if (   doc->delayLoadType == NEREDIR_WML_ONTIMER
                     || doc->delayLoadType == NEREDIR_WML_ONEVENT )
            {
                NNode* card = node_getByTag(doc->root, TAGID_CARD);
                if (card) {
                    if (doc->delayLoadType == NEREDIR_WML_ONEVENT) {
                        url = get_onevent_go_url(card, "onenterforward");
                    }
                    else {
                        url = attr_getValueStr(card->atts, ATTID_ONTIMER);
                        if (url == N_NULL)
                            url = get_onevent_go_url(card, "ontimer");
                    }
                }
                if (url)
                    jump = (base == N_NULL || nbk_strcmp(base, url)) ? N_TRUE : N_FALSE;
            }
            else if (doc->delayLoadType == NEREDIR_WML_ONEVENTFORWARD) {
                NNode* card = node_getByTag(doc->root, TAGID_CARD);
                if (card)
                    url = attr_getValueStr(card->atts, ATTID_ONENTERFORWARD);
                if (url)
                    jump = (base == N_NULL || nbk_strcmp(base, url)) ? N_TRUE : N_FALSE;
            }
            
            //dump_char(page, "redirect", -1);
            //dump_char(page, url, -1);
            //dump_int(page, jump);
            //dump_return(page);
            
            if (url && jump) {
                NLoadCmd* cmd = loadCmd_create();
                cmd->url = url_parse(base, url);
                cmd->doc = doc;
				NBK_callLater(page->platform, page->id, doc_do_load, cmd);
                return N_TRUE;
            }
        }
    }
    
    return N_FALSE;
}

static void doc_do_layout(void* user)
{
    NDocument* doc = (NDocument*)user;
    NPage* page = (NPage*)doc->page;
    nbool force = N_FALSE;

	smartptr_release(doc);
    
    if (doc->stop)
        return;
    if (!page->attach)
        return;
    
	// 样式数据接收完成，解析样式
    if (doc->extCssMgr && extCssMgr_ready(doc->extCssMgr)) {
        nbool begin = N_TRUE;
        char* data = N_NULL;
        int len = 0, offset;
        uint8* p;
        while (extCssMgr_get(doc->extCssMgr, &data, &len, begin)) {
            begin = N_FALSE;
            p = (uint8*)data;
            offset = (*p == 0xef && *(p+1) == 0xbb && *(p+2) == 0xbf) ? 3 : 0; // 跳过 BOM
            sheet_parseStyle(page->sheet, data + offset, len - offset);
        }
        extCssMgr_delete(&doc->extCssMgr);
        force = N_TRUE;
        //sheet_dump(page->sheet);
    }
    
#if DISABLE_LAYOUT
    view_dump_render_tree_2(page->view, page->view->root);
#else
    view_layout(page->view, page->view->root, force);
#endif
    
    if (doc->finish) {
        NBK_Event_RePosition evt;
        
        doc->layoutFinish = 1;

        NBK_memset(&evt, 0, sizeof(NBK_Event_RePosition));

        if (page->main) {
            NHistory* history = (NHistory*)page->history;
            NPoint pos = {0, 0};
            NRect va;

            NBK_helper_getViewableRect(page->platform, &va);

            if (doc->picUpdate) {
                pos.x = va.l;
                pos.y = va.t;
                evt.zoom = history_getZoom(history);
            }
            else {
                pos = history_getReadInfo(history, &evt.zoom);
            }

            view_normalizePoint(page->view, &pos, va, evt.zoom);
            evt.x = pos.x;
            evt.y = pos.y;
        }

        nbk_cb_call(NBK_EVENT_LAYOUT_FINISH, &page->cbEventNotify, &evt);
        
        if (doc_do_redirect(doc))
            return;
        
        if (doc->picUpdate) {
            doc->picUpdate = 0;
        }
        else {
            nbool loadImage = N_TRUE;
            NPage* mp = page_getMainPage(page);
            if (loadImage && !doc->disable) {
                if (imagePlayer_total(mp->view->imgPlayer) > 0) {
                    imagePlayer_start(mp->view->imgPlayer);
                }
                imagePlayer_progress(mp->view->imgPlayer);
            }
        }
    }
    else if (N_TRUE/*!doc->firstShow*/) {
        nbk_cb_call(NBK_EVENT_UPDATE, &page->cbEventNotify, N_NULL);
    }
    doc->firstShow = 1;
}

void doc_scheduleLayout(NDocument* doc)
{
	NPage* p = (NPage*)doc->page;

	if (doc->extCssMgr && !extCssMgr_ready(doc->extCssMgr))
		return;

	smartptr_get(doc);
	NBK_callLater(p->platform, p->id, doc_do_layout, doc);
}

NDocument* doc_create(void* page)
{
	NPage* p = (NPage*)page;
	NDocument* doc = (NDocument*)NBK_malloc0(sizeof(NDocument));
	N_ASSERT(doc);

	smartptr_init(doc, (SP_FREE_FUNC)doc_delete);
	doc->page = page;
	doc->tokenizer = xmltkn_create();
	doc->tokenizer->doc = doc;
	doc->spool = strPool_create();
	doc->ownStrPool = N_TRUE;

    return doc;
}

void doc_delete(NDocument** doc)
{
    NDocument* d = *doc;
    NPage* p = (NPage*)d->page;
    
    doc_clear(d);

    if (d->stack.dat) {
        NBK_free(d->stack.dat);
        d->stack.dat = N_NULL;
    }
    
    if (d->url) {
        NBK_free(d->url);
        d->url = N_NULL;
    }
    
    if (d->title) {
        NBK_free(d->title);
        d->title = N_NULL;
    }
    
    if (d->extCssMgr)
        smartptr_release(d->extCssMgr);
    
    if (d->ownStrPool && d->spool)
        strPool_delete(&d->spool);
    
    xmltkn_delete(&d->tokenizer);
    
    doc_detachWbxmlDecoder(d);
    
    NBK_free(d);
    *doc = N_NULL;
}

void doc_begin(NDocument* doc)
{
    NPage* page = (NPage*)doc->page;
        
    if (doc->stack.dat == N_NULL) {
        // allocate once
        doc->stack.dat = (NNode**)NBK_malloc0(sizeof(NNode*) * MAX_TREE_DEPTH);
        N_ASSERT(doc->stack.dat);
    }
    
    doc_clear(doc);
    
    doc->type = NEDOC_HTML;
    doc->stack.top = -1;
    doc->lastChild = N_NULL;
    doc->finish = 0;
    doc->layoutFinish = 0;
    doc->inTitle = N_FALSE;
    doc->inStyle = N_FALSE;
    doc->inScript = N_FALSE;
    doc->inCtrl = N_FALSE;
    doc->inTable = N_FALSE;
    doc->words = 0;
    doc->wordProcessed = 0;
    doc->wordCount = 500;
    doc->firstShow = 0;
    doc->picUpdate = 0;
    doc->hasSpace = N_FALSE;
    doc->stop = 0;
    doc->delayLoad = -1;
    doc->fix = N_NULL;
	doc->exceptNode = 0;
    doc->delayLoadType = NEREDIR_NONE;
    doc->encoding = NEENC_UNKNOWN;
    doc->gbRest = 0;
    doc->disable = 0;
    
    xmltkn_reset(doc->tokenizer);
    if (doc->ownStrPool && doc->spool)
        strPool_reset(doc->spool);
    
    doc_detachWbxmlDecoder(doc);
    
    if (doc->url)
        doc->url[0] = 0;
    
    if (doc->title) {
        NBK_free(doc->title);
        doc->title = N_NULL;
    }
    
    if (doc->extCssMgr)
        extCssMgr_delete(&doc->extCssMgr);
    
    view_begin(page->view);
    page->workState = NEWORK_IDLE;

    doc->__n_no = 0;
	doc->__time_start = 0;
}

static void dom_delete_tree(NDocument* doc, NNode* root)
{
    int16 top = -1;
    NNode* n = root;
    NNode* t = N_NULL;
    nbool ignore = 0;
    
    while (n) {
        if (!ignore && n->child) {
            doc->stack.dat[++top] = n;
            n = n->child;
        }
        else {
            if (n == root)
                break;
            ignore = 0;
            t = n;
            if (n->next)
                n = n->next;
            else {
                n = doc->stack.dat[top--];
                n->child = N_NULL;
            }

            node_delete(&t);
        }
    }
    if (n)
        node_delete(&n);
}

//#define DOC_TEST_RELEASE
void doc_clear(NDocument* doc)
{
#ifdef DOC_TEST_RELEASE
    uint32 t_consume = NBK_currentMilliSeconds();
#endif
    
    dom_delete_tree(doc, doc->root);
    doc->root = N_NULL;
    
#ifdef DOC_TEST_RELEASE
    dump_char(doc->page, "doc clear", -1);
    dump_uint(doc->page, NBK_currentMilliSeconds() - t_consume);
    dump_return(doc->page);
#endif
}

#if DUMP_FUNCTIONS
static void dump_dom_tree(NPage* page, NNode* root)
{
    NNode* sta[MAX_TREE_DEPTH];
    char* level = (char*)NBK_malloc(512);
    int lv = -1;
    NNode* n = root;
    nbool ignore = N_FALSE;
    int i, j, k, s;
    
    dump_return(page);
    dump_char(page, "===== dom tree =====", -1);
    dump_return(page);
    
    while (n) {
        if (!ignore) {
            sta[lv+1] = n;
            for (i=1, j=0; i <= lv+1; i++) {
                
                if (sta[i]->next)
                    level[j++] = '|';
                else {
                    if (sta[i] == n)
                        level[j++] = '`';
                    else
                        level[j++] = ' ';
                }
                
                if (sta[i] == n) {
                    level[j++] = '-';
                    level[j++] = '-';
                }
                else {
                    level[j++] = ' ';
                    level[j++] = ' ';
                }
                
                level[j++] = ' ';
            }

#if DOM_TEST_INFO
            s = sprintf(&level[j], "[%d]%s", n->__no, xml_getTagNames()[n->id]);
#else
            s = sprintf(&level[j], "%s", xml_getTagNames()[n->id]);
#endif
            j += s;

            for (k=0; n->atts && n->atts[k].id; k++) {
                
                if (   n->atts[k].id == ATTID_ID
                    || n->atts[k].id == ATTID_CLASS
                    || n->atts[k].id == ATTID_TYPE
                    || n->atts[k].id == ATTID_NAME
                    || n->atts[k].id == ATTID_VALUE
                    || n->atts[k].id == ATTID_TC_NAME ) {
                    s = sprintf(&level[j], " %s=%s", xml_getAttNames()[n->atts[k].id], n->atts[k].d.ps);
                    j += s;
                }
                else if (n->atts[k].id == ATTID_SELECTED) {
                    s = sprintf(&level[j], " %s", xml_getAttNames()[n->atts[k].id]);
                    j += s;
                }
            }
            
            dump_char(page, level, -1);
            
            if (n->id == TAGID_TEXT)
                dump_wchr(page, n->d.text, n->len);
            
            dump_return(page);
        }
        
        if (!ignore && n->child) {
            sta[++lv] = n;
            n = n->child;
        }
        else {
            ignore = N_FALSE;
            if (n == root)
                break;
            if (n->next)
                n = n->next;
            else {
                n = sta[lv--];
                ignore = N_TRUE;
            }
        }
    }
    
    NBK_free(level);
    
    dump_flush(page);
}
#endif

void doc_dumpDOM(NDocument* doc)
{
#if DUMP_FUNCTIONS
    NPage* page = (NPage*)doc->page;
    dump_dom_tree(page, doc->root);
#endif
}

void doc_end(NDocument* doc)
{
    if (doc->finish)
        return;

	//printf("document write: %d\n", NBK_currentMilliSeconds() - doc->__time_start);
    
    if (doc->wbxmlDecoder) {
        NWbxmlDecoder* decoder = doc->wbxmlDecoder;
        doc->wbxmlDecoder = N_NULL;
        if (wbxmlDecoder_decode(decoder)) {
            doc_write(doc, (char*)decoder->dst, decoder->dstPos, N_TRUE);
        }
        wbxmlDecoder_delete(&decoder);
    }
    
#if DUMP_DOM_TREE
    dump_dom_tree((NPage*)doc->page, doc->root);
#endif
    
    doc->finish = N_TRUE;
    xmltkn_abort(doc->tokenizer);
    doc_scheduleLayout(doc);
}

void doc_stop(NDocument* doc)
{
    doc->stop = 1;
    
    if (doc->extCssMgr) {
        NExtCssMgr* mgr = doc->extCssMgr;
		extCssMgr_stop(mgr);
    }
}

static nid check_encoding(const char* str, int length)
{
    nid enc = NEENC_UTF8;
    int scan = 400;
    int len = (length > scan) ? scan : length;
    int tmp = 8;//length of "encoding"
    int startPos = -1;

    startPos = nbk_strnfind_nocase(str, "encoding", len);
    if (startPos == -1) {
        startPos = nbk_strnfind_nocase(str, "charset", len);
        tmp = 7;// length of "charset="
    }
    
    if (startPos != -1) {
        const char* str1 = str + startPos + tmp;
        len = 10;// max length of "\"gb2312\"","gb2312","\"gbk\"","gbk"

        if (   nbk_strnfind_nocase(str1, "\"gb2312\"", len) != -1
            || nbk_strnfind_nocase(str1, "gb2312", len) != -1 ) {
            enc = NEENC_GB2312;
        }
        else if (   nbk_strnfind_nocase(str1, "\"gbk\"", len) != -1
                 || nbk_strnfind_nocase(str1, "gbk", len) != -1 ) {
            enc = NEENC_GB2312;
        }
    }
    
    return enc;
}

static char* convert_gb_to_utf8(NDocument* doc, char* data, int len, int* mbLen)
{
    char* mb = data;
    uint8* toofar = (uint8*)data + len;
    uint8* p;
    int mbl = 0, wcl = 0;
    nbool free = N_FALSE;
    wchr* unicode = N_NULL;
    
    if (doc->gbRest) {
        mb = (char*)NBK_malloc(len + 8);
        *mb = doc->gbRest; doc->gbRest = 0;
        nbk_strncpy(mb + 1, data, len);
        free = N_TRUE;
        toofar++;
    }
    
    p = (uint8*)mb;
    while (p < toofar) {
        if (*p <= 0x80) {
            mbl++;
            wcl++;
            p++;
        }
        else if (p + 1 < toofar) {
            mbl += 2;
            wcl++;
            p += 2;
        }
        else {
            doc->gbRest = *p;
            break;
        }
    }
    
    unicode = (wchr*)NBK_malloc(sizeof(wchr) * (wcl + 4));
    
    //wcl = NBK_conv_gbk2unicode(mb, mbl, unicode, wcl);
	wcl = conv_gbk_to_unicode(mb, mbl, unicode, wcl);
    
    if (free)
        NBK_free(mb);
    
    mb = uni_utf16_to_utf8_str(unicode, wcl, mbLen);
    
    NBK_free(unicode);
    
    return mb;
}

void doc_write(NDocument* doc, const char* str, int length, nbool final)
{
    NPage* page = (NPage*)doc->page;
    char* data = (char*)str;
    int len = length;
	//uint32 __time = 0;

    N_ASSERT(doc);
    
    if (doc->finish)
        return;
    
    if (NBK_currentFreeMemory() < (uint32)(length * 4))
        return;
    
    if (doc->wbxmlDecoder) {
        wbxmlDecoder_addSourceData(doc->wbxmlDecoder, (uint8*)str, length);
        return;
    }
    
    if (doc->encoding == NEENC_UNKNOWN)
        doc->encoding = check_encoding(str, length);
    
    if (page->workState != NEWORK_IDLE)
        return;
    
    if (doc->encoding == NEENC_GB2312)
        data = convert_gb_to_utf8(doc, (char*)str, length, &len);
    
    page->workState = NEWORK_PARSE;
	//if (doc->__time_start == 0)
	//	doc->__time_start = NBK_currentMilliSeconds();
    xmltkn_write(doc->tokenizer, data, len, final);
    page->workState = NEWORK_IDLE;
    
    if (doc->encoding == NEENC_GB2312)
        NBK_free(data);
    
    //dump_char(page, "write", -1);
    //dump_int(page, doc->words);
    //dump_return(page);

    if (   page->main
        && !final
        && !doc->disable
        && !doc->finish
        && doc->words - doc->wordProcessed >= doc->wordCount )
    {
        //doc_scheduleLayout(doc);
        doc->wordProcessed = doc->words;
        doc->wordCount *= 2;
    }
}

static NNode* dom_insert_as_child(NNode** parent, NNode* lastChild, NNode* node)
{
    if (*parent == N_NULL) {
        *parent = node;
    }
    else {
        if (lastChild == N_NULL) {
            // first child
            (*parent)->child = node;
            node->parent = *parent;
        }
        else {
            lastChild->next = node;
            node->parent = *parent;
            node->prev = lastChild;
        }
    }
    
    return node;
}

#if DUMP_FUNCTIONS
static void dom_dump_level(const NDocument* doc, const NNode* node)
{
    char level[64];
    char ch;
    int16 i, lv;
    lv = doc->stack.top;
    if (node->id == TAGID_TEXT)
        lv++;
    for (i=0; i <= lv; i++) {
        ch = (i == lv) ? '+' : ' ';
        level[i] = ch;
    }
    level[i++] = '-';
    level[i++] = ' ';
    nbk_strcpy(&level[i], xml_getTagNames()[node->id]);
    dump_char(doc->page, level, -1);
    if (node->id == TAGID_TEXT && !doc->inStyle)
        dump_wchr(doc->page, node->d.text, node->len);
    dump_return(doc->page);
}
#endif

static void dom_push_block(NDocument* doc, NNode* newNode)
{
    N_ASSERT(doc->stack.top < MAX_TREE_DEPTH_LIMIT);
    doc->stack.dat[++doc->stack.top] = newNode;
    doc->lastChild = newNode->child;
    
    //dom_dump_level(doc, newNode);
}

static nbool dom_pop_block(NDocument* doc, NNode* node)
{
    NNode* n;
    nid id = node->id - TAGID_CLOSETAG;
    N_ASSERT(doc->stack.top >= 0);
    n = doc->stack.dat[doc->stack.top];
    if (n->id != id)
        return N_FALSE;
    doc->lastChild = doc->stack.dat[doc->stack.top--];
    return N_TRUE;
}

static void doc_process_single_node(NDocument* doc, NNode* node)
{
    //dom_dump_level(doc, node);

    dom_insert_as_child(&doc->stack.dat[doc->stack.top], doc->lastChild, node);
    doc->lastChild = node;
    if (node->id == TAGID_TEXT && !node->space)
        doc->words += node->len;
}

#if DUMP_FUNCTIONS
static void doc_dump_token(const NDocument* doc, const NToken* token)
{
    NPage* page = (NPage*)doc->page;
    char tag[32];

    if (token->id > TAGID_CLOSETAG)
        sprintf(tag, "</%s>", xml_getTagNames()[token->id - TAGID_CLOSETAG]);
    else
        sprintf(tag, "<%s>", xml_getTagNames()[token->id]);
    
    dump_char(page, tag, -1);
    if (token->id == TAGID_TEXT) {
        if (doc->inStyle)
            dump_char(page, token->d.str, token->len);
        else
            dump_wchr(page, token->d.text, token->len);
    }
    dump_return(page);
}
#endif

static nbool handle_document(nid type)
{
    switch (type) {
    case NEDOC_UNKNOWN:
    //case NEDOC_HTML:
        return N_FALSE;
    }
    
    return N_TRUE;
}

static nid l_fix_select_has[] = {
    TAGID_OPTION, TAGID_OPTION + TAGID_CLOSETAG,
    TAGID_TEXT,
    TAGID_SELECT + TAGID_CLOSETAG,
    0};

static nid l_fix_table_has[] = {
    TAGID_TR, TAGID_TR + TAGID_CLOSETAG,
    TAGID_TD, TAGID_TD + TAGID_CLOSETAG,
    0};

// Building document tree
void doc_processToken(NDocument* doc, NToken* token)
{
    NPage* page = (NPage*)doc->page;
    NNode* node = N_NULL;
    
    if (doc->finish)
        return;

    if (   doc->root == N_NULL
        && token->id != TAGID_HTML
        && token->id != TAGID_WML
        && token->id != TAGID_BODY ) {
        // 创建默认顶级结点
        NToken bt;
        NBK_memset(&bt, 0, sizeof(NToken));
        bt.id = TAGID_HTML;
		node = node_create(&bt, ++doc->__n_no);
        dom_insert_as_child(&doc->root, N_NULL, node);
        dom_push_block(doc, node);
        node = N_NULL;
    }

	// 强制插入 BODY
	if (token->id == TAGID_HEAD + TAGID_CLOSETAG) {
		doc->exceptNode = TAGID_BODY;
	}
	else if (doc->exceptNode) {
		if (token->id != doc->exceptNode) {
			NToken tk;
			NBK_memset(&tk, 0, sizeof(NToken));
			tk.id = doc->exceptNode;
			node = node_create(&tk, ++doc->__n_no);
			dom_insert_as_child(&doc->stack.dat[doc->stack.top], doc->lastChild, node);
			dom_push_block(doc, node);
#if CREATE_RENDER_TREE
	        view_processNode(page->view, node);
#endif
			node = N_NULL;
		}
		doc->exceptNode = 0;
	}
    
    if (token->id == TAGID_TITLE) {
        doc->inTitle = 1;
        return;
    }
    else if (token->id == TAGID_TITLE + TAGID_CLOSETAG) {
        doc->inTitle = 0;
        return;
    }
    
    if (token->id == TAGID_STYLE) {
        doc->inStyle = N_TRUE;
        return;
    }
    else if (token->id == TAGID_STYLE + TAGID_CLOSETAG) {
        doc->inStyle = N_FALSE;
        sheet_parse(page->sheet);
        //sheet_dump(page->sheet);
        return;
    }
    
    if (token->id == TAGID_SCRIPT) {
        doc->inScript = N_TRUE;
        return;
    }
    else if (token->id == TAGID_SCRIPT + TAGID_CLOSETAG) {
        doc->inScript = N_FALSE;
        return;
    }

#if DUMP_TOKEN
    doc_dump_token(doc, token);
    return;
#endif

    if (token->id == TAGID_LINK) {
        nbool load = N_FALSE;
        char*  rel = attr_getValueStr(token->atts, ATTID_REL);
        char* type = attr_getValueStr(token->atts, ATTID_TYPE);
        char* href = attr_getValueStr(token->atts, ATTID_HREF);

        if (rel && nbk_strcmp(rel, "stylesheet") == 0)
            load = N_TRUE;
        if (type && nbk_strcmp(type, "text/css") == 0)
            load = N_TRUE;

        if (load && href) {
            char* url = url_parse(doc_getUrl(doc), href);
            if (doc->extCssMgr == N_NULL)
                doc->extCssMgr = extCssMgr_create(doc);
            if (!extCssMgr_addExtCss(doc->extCssMgr, url))
                NBK_free(url);
        }
        return;
    }
    else if (token->id == TAGID_LINK + TAGID_CLOSETAG)
        return;
    
    if (doc->inCtrl) {
        if (token->id == TAGID_SELECT + TAGID_CLOSETAG)
            doc->fix = N_NULL;
    }
    
    switch (token->id) {
    case TAGID_INPUT + TAGID_CLOSETAG:
    case TAGID_BUTTON + TAGID_CLOSETAG:
    case TAGID_SELECT + TAGID_CLOSETAG:
    case TAGID_TEXTAREA + TAGID_CLOSETAG:
        doc->inCtrl = N_FALSE;
        break;
    case TAGID_TABLE + TAGID_CLOSETAG:
        doc->inTable = N_FALSE;
        break;
    case TAGID_TR + TAGID_CLOSETAG:
    case TAGID_TD + TAGID_CLOSETAG:
        doc->inTable = N_TRUE;
        break;
    }
    
    if (token->id == TAGID_BASE) {
        char* url = attr_getValueStr(token->atts, ATTID_HREF);
        if (url) {
            // base 的使用
            // 规范规定base地址用于拼装页面相对地址。
            // base地址仅用于拼装；刷新时采用请求回应地址
            doc_setUrl(doc, url);
            if (page->main && history_getUrl((NHistory*)page->history, page->id) == N_NULL)
                history_setUrl((NHistory*)page->history, url);
        }
        return;
    }
    
    if (doc->inTitle && doc->title == N_NULL) {
        doc->title = token->d.text;
        token->d.text = N_NULL;
        return;
    }
    if (token->id == TAGID_CARD) {
        char* url = N_NULL;
        // 检测标题
        if (doc->title == N_NULL) {
            char* t = attr_getValueStr(token->atts, ATTID_TITLE);
            if (t)
                doc->title = uni_utf8_to_utf16_str((uint8*)t, -1, N_NULL);
        }
        // 检测 onenterforward
        url = attr_getValueStr(token->atts, ATTID_ONENTERFORWARD);
        if (url) {
            doc->delayLoadType = NEREDIR_WML_ONEVENTFORWARD;
            doc->delayLoad = 1000;
        }
    }
    
    if (doc->inStyle) { // 处理内部样式文本
#if CREATE_RENDER_TREE
        if (doc->extCssMgr)
            extCssMgr_addInnCss(doc->extCssMgr, token->d.str, token->len);
        else
            sheet_addData(page->sheet, token->d.str, token->len);
#endif
        return;
    }
    
    if (doc->inScript)
        return;

    if (token->id == TAGID_META) {

        char *v1, *v2;
        
        // MS spec. <meta name="MobileOptimized" content="240"/>
        v1 = attr_getValueStr(token->atts, ATTID_NAME);
        if (v1 && nbk_strcmp(v1, "MobileOptimized") == 0) {
            if (doc->type == NEDOC_UNKNOWN || doc->type == NEDOC_HTML)
                doc->type = NEDOC_XHTML;
            v2 = attr_getValueStr(token->atts, ATTID_CONTENT);
            if (v2) {
                //coord ow = NBK_atoi(v2);
                //if (ow >= 176)
                //    page->view->optWidth = ow;
            }
        }
        else if (v1 && nbk_strcmp(v1, "viewport") == 0) {
            v2 = attr_getValueStr(token->atts, ATTID_CONTENT);
            if (v2) {
                int pos = nbk_strfind(v2, "width=");
                if (pos != -1) {
                    char* p = v2 + pos + 6;
                    coord dw;
                    if (nbk_strncmp(p, "device-width", 12) == 0)
                        dw = page->view->scrWidth;
                    else
                        dw = NBK_atoi(p);
                    if (   dw <= WIDTH_FOR_MOBILE
                        && (doc->type == NEDOC_UNKNOWN || doc->type == NEDOC_HTML))
                        doc->type = NEDOC_XHTML;
                }
            }
        }

        // <meta http-equiv="Refresh" content="5;url=http://www.badiu.com" />
        v1 = attr_getValueStr(token->atts, ATTID_HTTP_EQUIV);
        if (   v1 
            && handle_document(doc->type)
            && nbk_strncmp_nocase(v1, "Refresh", 7) == 0 ) {
            v2 = attr_getValueStr(token->atts, ATTID_CONTENT);
            if (v2) {
                int urlPos = nbk_strfind_nocase(v2, "url=");
                if (urlPos >= 0) {
                    char *url, *u;
                    int delay;
                    
                    delay = NBK_atoi(v2);
                    if (delay <= 0) delay = 1;
                    doc->delayLoad = delay * 1000;
                    
                    u = v2 + urlPos + 4;
                    if (nbk_strlen(u) > 3) { // g.cn
                        url = url_parse(doc_getUrl(doc), u);
                        if (nbk_strcmp(url, doc_getUrl(doc))) { // 当跳转地址与本页地址不同时，启动跳转
                            doc_setUrl(doc, url); // replace document url
                            doc->delayLoadType = NEREDIR_META_REFRESH;
                        }
                        else
                            doc->delayLoad = 0;
                        NBK_free(url);
                    }
                    else
                        doc->delayLoad = 0;
                }
            }
        }
        return; // fixme: not create in DOM
    }
    
    // fix form controls
    if (doc->inCtrl && doc->fix) {
        int i;
        for (i=0; doc->fix[i]; i++)
            if (token->id == doc->fix[i])
                break;
        if (doc->fix[i] == 0) {
            NNode t;
            NBK_memset(&t, 0, sizeof(NNode));
            if (doc->fix == l_fix_select_has) {
                t.id = TAGID_SELECT + TAGID_CLOSETAG;
                dom_pop_block(doc, &t);
                view_processNode(page->view, &t);
            }
            doc->fix = N_NULL;
            doc->inCtrl = N_FALSE;
        }
    }
    
    // fix table
    if (doc->inTable) {
        int i;
        for (i=0; l_fix_table_has[i]; i++)
            if (token->id == l_fix_table_has[i])
                break;
        if (l_fix_table_has[i] == 0)
            return; // not allow in table
    }

    if (node_is_single(token->id)) {
        if (token->id < TAGID_CLOSETAG) {
            
            if (doc->stack.top == -1) {
                doc_end(doc);
                return;
            }
            
            if (token->id == TAGID_TIMER) {
                char* v = attr_getValueStr(token->atts, ATTID_VALUE);
                if (v) {
                    int s = NBK_atoi(v); // 1/10 second
                    if (s <= 0) s = 10;
                    doc->delayLoad = s * 100;
                    doc->delayLoadType = NEREDIR_WML_ONTIMER;
                }
                return;
            }
                   
			node = node_create(token, ++doc->__n_no);
            doc_process_single_node(doc, node);
#if CREATE_RENDER_TREE
            if (!doc->inCtrl)
                view_processNode(page->view, node);
#endif
        }
        return; // ignore close tag of single node
    }
    
    if (token->id > TAGID_CLOSETAG) {
        NNode* t = (NNode*)token; // id must be the first member of Token and Node.
        nbool ret = dom_pop_block(doc, t);
        
#if CREATE_RENDER_TREE
        if (!doc->inCtrl && ret)
            view_processNode(page->view, t);
#endif
        
        if (doc->stack.top == -1) {
            // reach the end
            doc_end(doc);
        }
        return;
    }
    
    if (token->id == TAGID_TEXT && token->len == 1 && token->d.text[0] == 0x20) {
        doc->hasSpace = 1;
        return;
    }

    if (doc->hasSpace && !doc->inCtrl) {
        doc->hasSpace = 0;
        
        switch (token->id) {
        case TAGID_CARD:
        case TAGID_BODY:
        case TAGID_DIV:
        case TAGID_P:
        case TAGID_UL:
        case TAGID_OL:
        case TAGID_LI:
            break;
            
        default:
        {
            NToken tok;
            NBK_memset(&tok, 0, sizeof(NToken));
            tok.id = TAGID_TEXT;
            tok.d.text = (wchr*)NBK_malloc(sizeof(wchr) * 2);
            tok.d.text[0] = 0x20; tok.d.text[1] = 0;
            tok.len = 1;
			node = node_create(&tok, ++doc->__n_no);
            doc_process_single_node(doc, node);
#if CREATE_RENDER_TREE
            if (!doc->inCtrl)
                view_processNode(page->view, node);
#endif
        }
            break;
        }
    }

	node = node_create(token, ++doc->__n_no);
    
    if (token->id == TAGID_TEXT) {
        doc_process_single_node(doc, node);
#if CREATE_RENDER_TREE
        if (!doc->inCtrl)
            view_processNode(page->view, node);
#endif
        return;
    }
    
    if (node->id == TAGID_ONEVENT) {
        char* type = attr_getValueStr(node->atts, ATTID_TYPE);
        if (type && nbk_strcmp(type, "onenterforward") == 0) {
            doc->delayLoad = 1000;
            doc->delayLoadType = NEREDIR_WML_ONEVENT;
        }
    }
    
    if (doc->root == N_NULL) {
        dom_insert_as_child(&doc->root, N_NULL, node);
    }
    else if (doc->stack.top == MAX_TREE_DEPTH_LIMIT) {
        doc_end(doc);
        return;
    }
    else {
        dom_insert_as_child(&doc->stack.dat[doc->stack.top], doc->lastChild, node);
    }
    dom_push_block(doc, node);
    
#if CREATE_RENDER_TREE
    if (!doc->inCtrl)
        view_processNode(page->view, node);
#endif
    
    switch (token->id) {
    case TAGID_INPUT:
    case TAGID_BUTTON:
    case TAGID_SELECT:
    case TAGID_TEXTAREA:
        doc->inCtrl = N_TRUE;
        break;
    case TAGID_TABLE:
    case TAGID_TR:
        doc->inTable = N_TRUE;
        break;
    case TAGID_TD:
        doc->inTable = N_FALSE;
        break;
    }
    
    if (token->id == TAGID_SELECT)
        doc->fix = l_fix_select_has;
}

void doc_acceptType(NDocument* doc)
{
    NPage* page = (NPage*)doc->page;
    
    if (handle_document(doc->type)) {
        if (doc->extCssMgr)
            extCssMgr_schedule(doc->extCssMgr);
    }
    else { // 不处理该文档，请求转码服务
        doc->finish = 1;
        xmltkn_abort(doc->tokenizer);
        if (doc_getUrl(doc)) {
            page->settings->support = NENotSupportByNbk;
            doc_doGet(doc, doc_getUrl(doc), N_FALSE, N_TRUE);
        }
        else if (!NBK_handleError(NELERR_NBK_NOT_SUPPORT)){
            doc_doGet(page->doc, NBK_getStockPage(NESPAGE_ERROR), N_FALSE, N_FALSE);
        }
    }
}

static NRenderNode* doc_get_first_render_node(NNode* node)
{
    NNode* sta[MAX_TREE_DEPTH];
    NRenderNode* r;
    NNode* n = node;
    int lv = -1;
    nbool ignore = N_FALSE;
        
    while (n) {
            
        r = (NRenderNode*)n->render;
        if (!ignore && r && !r->needLayout && r->type != RNT_INLINE) {
            if (r->type == RNT_TEXT) {
                if (r->child)
                    return r->child;
            }
            else
                return r;
        }
            
        if (!ignore && n->child) {
            sta[++lv] = n;
            n = n->child;
        }
        else {
            ignore = N_FALSE;
            if (n == node)
                break;
            if (n->next)
                n = n->next;
            else {
                n = sta[lv--];
                ignore = N_TRUE;
            }
        }
    }

    return N_NULL;
}

NNode* doc_getFocusNode(NDocument* doc, const NRect* area)
{
    NPage* page = (NPage*)doc->page;
    NView* view = page->view;
    NNode* sta[MAX_TREE_DEPTH];
    int16 idx = -1;
    NNode* n = doc->root;
    nbool ignore = N_FALSE, abort = N_FALSE;
    NRenderNode* r;
    NRect rr, vw;
    
    if (n == N_NULL)
        return N_NULL;
    
    vw = *area;
    rect_toDoc(&vw, history_getZoom((NHistory*)page->history));
    
    while (n) {
        
        if (!ignore) {
            switch (n->id) {
            case TAGID_FRAME:
                ignore = N_TRUE;
                break;
            case TAGID_A:
            case TAGID_ANCHOR:
            {
                r = doc_get_first_render_node(n);
                if (r) {
                    renderNode_absRect(r, &rr);
                    if (rr.l >= vw.l && rr.t >= vw.t)
                        return n;
                    if (rr.t >= vw.b)
                        abort = N_TRUE;
                }
                ignore = N_TRUE;
                break;
            }
            case TAGID_BUTTON:
            case TAGID_INPUT:
            {
                r = (NRenderNode*)n->render;
                if (!r->needLayout && renderInput_isVisible((NRenderInput*)r)) {
                    renderNode_absRect(r, &rr);
                    if (rect_isContain(&vw, &rr))
                        return n;
                    if (rr.t >= vw.b)
                        abort = N_TRUE;
                }
                break;
            }
            case TAGID_TEXTAREA:
            case TAGID_SELECT:
            {
                r = (NRenderNode*)n->render;
                if (!r->needLayout) {
                    renderNode_absRect(r, &rr);
                    if (rect_isContain(&vw, &rr))
                        return n;
                    if (rr.t >= vw.b)
                        abort = N_TRUE;
                }
                break;
            }
            case TAGID_DIV:
            {
                r = (NRenderNode*)n->render;
                if (r->display == CSS_DISPLAY_NONE)
                    ignore = N_TRUE;
                else if (doc->type == NEDOC_NARROW) {
                    if (   n->Click
                        && attr_getValueInt32(n->atts, ATTID_TC_IA) > 0
                        && rect_isIntersect(&vw, &r->r) )
                        return n;
                    else if (((NRenderBlock*)r)->ne_display && rect_isContain(&vw, &r->r))
                        return n;
                }
                break;
            }
            default:
                if (   doc->type == NEDOC_DXXML
                    || doc->type == NEDOC_NARROW ) {
                    if (n->Click && attr_getValueInt32(n->atts, ATTID_TC_IA) > 0) {
                        r = doc_get_first_render_node(n);
                        if (r) {
                            renderNode_absRect(r, &rr);
                            if (rr.l >= vw.l && rr.t >= vw.t)
                                return n;
                            if (rr.t >= vw.b)
                                abort = N_TRUE;
                        }
                        ignore = N_TRUE;
                    }
                }
                break;
            }
        }
        
        if (abort)
            break;
        
        if (!ignore && n->child) {
            sta[++idx] = n;
            n = n->child;
        }
        else {
            ignore = N_FALSE;
            if (n == doc->root)
                break;
            if (n->next)
                n = n->next;
            else {
                n = sta[idx--];
                ignore = N_TRUE;
            }
        }
    }
    
    return N_NULL;
}

static nbool doc_child_include_pos(NNode* node, coord x, coord y, nbool abs)
{
    NNode* sta[MAX_TREE_DEPTH];
    NNode* n = node;
    int lv = -1;
    nbool ignore = N_FALSE;
    NRect rect;
    NPoint pt;
    
    pt.x = x; pt.y = y;
    
    while (n) {
        
        if (!ignore && n->render) {
            NRenderNode* r = (NRenderNode*)n->render;
            if (!r->needLayout && (r->type == RNT_TEXT || r->type == RNT_IMAGE)) {
                if (abs) {
                    if (rect_hasPt(&r->r, x, y))
                        return N_TRUE;
                }
                else {
                    if (r->type == RNT_TEXT) {
                        if (renderText_hasPt((NRenderText*)r, pt))
                            return N_TRUE;
                    }
                    else if (renderNode_getAbsRect(r, &rect) && rect_hasPt(&rect, x, y))
                        return N_TRUE;
                }
            }
        }
        
        if (!ignore && n->child) {
            sta[++lv] = n;
            n = n->child;
        }
        else {
            ignore = N_FALSE;
            if (n == node)
                break;
            if (n->next)
                n = n->next;
            else {
                n = sta[lv--];
                ignore = N_TRUE;
            }
        }
    }
    
    return N_FALSE;
}

static NNode* doc_get_focus_node_by_pos(NDocument* doc, coord x, coord y)
{
    NPage* page = (NPage*)doc->page;
    NView* view = page->view;
    NNode* sta[MAX_TREE_DEPTH];
    int16 idx = -1;
    NNode* n = doc->root;
    NRenderNode* r;
    nbool ignore = N_FALSE;
    NRect rect;
    
    if (n == N_NULL)
        return N_NULL;
    
    while (n) {

        if (!ignore) {
            
            switch (n->id) {
            case TAGID_A:
            case TAGID_ANCHOR:
            {
                r = (NRenderNode*)n->render;
                if (doc_child_include_pos(n, x, y, N_FALSE)) {
                    return n;
                }
                
                ignore = N_TRUE;
                break;
            }
            case TAGID_BUTTON:
            case TAGID_INPUT:
            {
                r = (NRenderNode*)n->render;
                if (!r->needLayout && renderInput_isVisible((NRenderInput*)r) && renderNode_getAbsRect(r, &rect)) {
                    if (rect_hasPt(&rect, x, y))
                        return n;
                }
                break;
            }
            case TAGID_TEXTAREA:
            case TAGID_SELECT:
            {
                r = (NRenderNode*)n->render;
                if (!r->needLayout && renderNode_getAbsRect(r, &rect)) {
                    if (rect_hasPt(&rect, x, y))
                        return n;
                }
                ignore = N_TRUE;
                break;
            }
            case TAGID_DIV:
            {
                NRenderBlock* rb = (NRenderBlock*)n->render;
                r = (NRenderNode*)n->render;
                if (r->display == CSS_DISPLAY_NONE) {
                    ignore = N_TRUE;
                }
                else if (   !r->needLayout
                         && rb->ne_display
                         && renderNode_getAbsRect(r, &rect)) {
                    if (rect_hasPt(&rect, x, y))
                        return n;
                }
                break;
            }
            } // switch
        }
        
        if (!ignore && n->child) {
            sta[++idx] = n;
            n = n->child;
        }
        else {
            ignore = N_FALSE;
            if (n == doc->root)
                break;
            if (n->next)
                n = n->next;
            else {
                n = sta[idx--];
                ignore = N_TRUE;
            }
        }
    }

    return N_NULL;
}

static NNode* doc_get_focus_node_by_pos_ff(NDocument* doc, coord x, coord y)
{
    NPage* page = (NPage*)doc->page;
    NView* view = page->view;
    NNode* sta[MAX_TREE_DEPTH];
    int16 idx = -1;
    NNode* n = doc->root;
    NRenderNode* r;
    nbool ignore = N_FALSE;
    NNode* last = N_NULL; // FIXME: search all dom to get node focused.
    
    if (n == N_NULL)
        return N_NULL;
    
    while (n) {
        r = (NRenderNode*)n->render;
        if (r && !r->display)
            ignore = N_TRUE;

        if (!ignore) {
            switch (n->id) {
            case TAGID_A:
            {
                if (doc_child_include_pos(n, x, y, N_TRUE)) {
                    last = n;
                }
                
                ignore = N_TRUE;
                break;
            }
            case TAGID_BUTTON:
            case TAGID_INPUT:
            {
                if (!r->needLayout && renderInput_isVisible((NRenderInput*)r) && rect_hasPt(&r->r, x, y)) {
                    last = n;
                }
                break;
            }
            case TAGID_TEXTAREA:
            case TAGID_SELECT:
            {
                if (!r->needLayout && rect_hasPt(&r->r, x, y)) {
                    last = n;
                }
                ignore = N_TRUE;
                break;
            }
            case TAGID_DIV:
            {
                if (!r->needLayout && attr_getValueInt32(n->atts, ATTID_TC_IA) > 0) {
                    if (rect_hasPt(&r->r, x, y)) {
                        last = n; // FIXME: temp
                    }
                }
                break;
            }
            }
        }
        
        if (!ignore && n->child) {
            sta[++idx] = n;
            n = n->child;
        }
        else {
            ignore = N_FALSE;
            if (n == doc->root)
                break;
            if (n->next)
                n = n->next;
            else {
                n = sta[idx--];
                ignore = N_TRUE;
            }
        }
    }

    return last;
}

NNode* doc_getFocusNodeByPos(NDocument* doc, coord x, coord y)
{
    NPage* page = (NPage*)doc->page;
    float zoom = history_getZoom((NHistory*)page->history);
    x = (coord)float_idiv(x, zoom);
    y = (coord)float_idiv(y, zoom);
    return doc_get_focus_node_by_pos(doc, x, y);
}

typedef struct _NTaskFindImage {
    NNode*  image;
    nbool    absLayout;
    coord   x;
    coord   y;
} NTaskFindImage;

static int task_find_image_by_pos(NNode* node, void* user, nbool* ignore)
{
    NRenderNode* r;
    
    if (node->id == TAGID_DIV) {
        r = (NRenderNode*)node->render;
        if (r && !r->display)
            *ignore = N_TRUE;
    }
    else if (node->id == TAGID_IMG) {
        r = (NRenderNode*)node->render;
        if (r && r->display) {
            NTaskFindImage* task = (NTaskFindImage*)user;
            NRect rect;
            if (task->absLayout)
                rect = r->r;
            else
                renderNode_getAbsRect(r, &rect);
            if (rect_hasPt(&rect, task->x, task->y)) {
                task->image = node;
                return 1;
            }
            
        }
    }
    return 0;
}

NNode* doc_getImageNodeByPos(NDocument* doc, coord x, coord y)
{
    NPage* page = (NPage*)doc->page;
    float zoom = history_getZoom((NHistory*)page->history);
    NTaskFindImage task;
    task.image = N_NULL;
    task.absLayout = N_FALSE;
    task.x = (coord)float_idiv(x, zoom);
    task.y = (coord)float_idiv(y, zoom);
    node_traverse_depth(doc->root, task_find_image_by_pos, N_NULL, &task);
    return task.image;
}

char* doc_getFocusUrl(NDocument* doc, NNode* focus)
{
    NNode* n = focus;
    
    while (n && n->id != TAGID_A && n->id != TAGID_ANCHOR)
        n = n->parent;
    
    if (n) {
        char* href = N_NULL;
        
        if (n->id == TAGID_A) {
            href = attr_getValueStr(n->atts, ATTID_HREF);
        }
        else if (n->id == TAGID_ANCHOR) {
            NPage* page = (NPage*)doc->page;
            NNode* go = node_getByTag(n, TAGID_GO);
            if (go)
                href = attr_getValueStr(go->atts, ATTID_HREF);
        }
        
        if (href)
            return url_parse(doc->url, href);
    }
    
    return N_NULL;
}

static int doc_build_path(NNode* node, NNode** sta)
{
    NNode* sta2[MAX_TREE_DEPTH];
    NNode* n = node;
    int i = -1, idx = -1;
    
    while (n->parent) {
        n = n->parent;
        sta2[++i] = n;
    }

    for (; i > -1; i--) {
        sta[++idx] = sta2[i];
    }
    
    return idx;
}

NNode* doc_getPrevFocusNode(NDocument* doc, NNode* curr)
{
    NPage* page = (NPage*)doc->page;
    NView* view = page->view;
    NNode* sta[MAX_TREE_DEPTH];
    int16 idx = -1;
    NNode* n = doc->root;
    NRenderNode* r;
    nbool ignore = N_FALSE;
    NNode* last = N_NULL;
    
    if (n == N_NULL)
        return n;
    
    while (n) {
        if (!ignore) {
            switch (n->id) {
            case TAGID_FRAME:
                ignore = N_TRUE;
                break;
            case TAGID_A:
            case TAGID_ANCHOR:
            {
                if (doc_get_first_render_node(n)) {
                    if (n == curr)
                        return last;
                    else
                        last = n;
                }
                ignore = N_TRUE;
                break;
            }
            case TAGID_BUTTON:
            case TAGID_INPUT:
            {
                r = (NRenderNode*)n->render;
                if (!r->needLayout && renderInput_isVisible((NRenderInput*)r)) {
                    if (n == curr)
                        return last;
                    else
                        last = n;
                }
                break;
            }
            case TAGID_TEXTAREA:
            case TAGID_SELECT:
            {
                r = (NRenderNode*)n->render;
                if (!r->needLayout) {
                    if (n == curr)
                        return last;
                    else
                        last = n;
                }
                break;
            }
            case TAGID_DIV:
            {
                r = (NRenderNode*)n->render;
                if (r->display == CSS_DISPLAY_NONE)
                    ignore = N_TRUE;
                else if (doc->type == NEDOC_NARROW) {
                    if (   (n->Click && attr_getValueInt32(n->atts, ATTID_TC_IA) > 0)
                        || ((NRenderBlock*)r)->ne_display ) {
                        if (n == curr)
                            return last;
                        else
                            last = n;
                    }
                }
                break;
            }
            default:
                if (   doc->type == NEDOC_DXXML
                    || doc->type == NEDOC_NARROW )
                {
                    if (n->Click && attr_getValueInt32(n->atts, ATTID_TC_IA) > 0) {
                        if (doc_get_first_render_node(n)) {
                            if (n == curr)
                                return last;
                            else
                                last = n;
                        }
                        ignore = (n->id == TAGID_DIV || n->id == TAGID_SPAN) ? N_FALSE : N_TRUE;
                    }
                }
                break;
            }
        }
        
        if (!ignore && n->child) {
            sta[++idx] = n;
            n = n->child;
        }
        else {
            ignore = N_FALSE;
            if (n == doc->root)
                break;
            if (n->next)
                n = n->next;
            else {
                n = sta[idx--];
                ignore = N_TRUE;
            }
        }
    }
    
    return N_NULL;
}

NNode* doc_getNextFocusNode(NDocument* doc, NNode* curr)
{
    NPage* page = (NPage*)doc->page;
    NView* view = page->view;
    NNode* sta[MAX_TREE_DEPTH];
    int idx;
    NNode* n = curr;
    NRenderNode* r;
    nbool ignore = N_TRUE;
    
    if (n == N_NULL)
        return n;
    
    idx = doc_build_path(n, sta);
    if ((n->id == TAGID_DIV || n->id == TAGID_SPAN) && n->child) {
        sta[++idx] = n;
        n = n->child;
        ignore = N_FALSE;
    }
    
    while (n) {
        if (!ignore) {
            switch (n->id) {
            case TAGID_FRAME:
                ignore = N_TRUE;
                break;
            case TAGID_A:
            case TAGID_ANCHOR:
            {
                if (doc_get_first_render_node(n)) {
                    return n;
                }
                ignore = N_TRUE;
                break;
            }
            case TAGID_BUTTON:
            case TAGID_INPUT:
            {
                r = (NRenderNode*)n->render;
                if (!r->needLayout && renderInput_isVisible((NRenderInput*)r))
                    return n;
                break;
            }
            case TAGID_TEXTAREA:
            case TAGID_SELECT:
            {
                r = (NRenderNode*)n->render;
                if (!r->needLayout)
                    return n;
                break;
            }
            case TAGID_DIV:
            {
                r = (NRenderNode*)n->render;
                if (r->display == CSS_DISPLAY_NONE)
                    ignore = N_TRUE;
                else if (doc->type == NEDOC_NARROW) {
                    if (   (n->Click && attr_getValueInt32(n->atts, ATTID_TC_IA) > 0)
                        || ((NRenderBlock*)r)->ne_display )
                        return n;
                }
				break;
            }
            default:
                if (   doc->type == NEDOC_DXXML
                    || doc->type == NEDOC_NARROW )
                {
                    if (n->Click && attr_getValueInt32(n->atts, ATTID_TC_IA) > 0) {
                        if (doc_get_first_render_node(n)) {
                            return n;
                        }
                        ignore = N_TRUE;
                    }
                }
                break;
            }
        }
        
        if (!ignore && n->child) {
            sta[++idx] = n;
            n = n->child;
        }
        else {
            ignore = N_FALSE;
            if (n == doc->root)
                break;
            if (n->next)
                n = n->next;
            else {
                n = sta[idx--];
                ignore = N_TRUE;
            }
        }
    }
    
    return N_NULL;
}

void doc_getFocusNodeRect(NDocument* doc, NNode* focus, NRect* rect)
{
    NPage* page = (NPage*)doc->page;
    *rect = view_getNodeRect(page->view, focus);
}

void doc_doGet(NDocument* doc, const char* url, nbool submit, nbool noDeal)
{
    NPage* page = (NPage*)doc->page;
    NLoadCmd* cmd;
    char* base = doc_getUrl(doc);
    char* u = N_NULL;
    NUpCmdSet settings;
    
    u = url_parse(base, url);
	if (u == N_NULL)
		return;

    settings = page_decideRequestMode(page, u, submit);
    
	cmd = loadCmd_create();
    cmd->url = u;
    cmd->doc = doc;
    cmd->noDeal = noDeal;
    cmd->via = settings.via;
    cmd->submit = submit;

	NBK_callLater(page->platform, page->id, doc_do_load, cmd);
}

void doc_doPost(NDocument* doc, const char* url, nid enc, char* body, NFileUpload* files)
{
    NPage* page = (NPage*)doc->page;
    NLoadCmd* cmd;
	char* u = url_parse(doc->url, url);

	if (u == N_NULL)
		return;
    
	cmd = loadCmd_create();
	cmd->url = u;
    cmd->doc = doc;
    cmd->body = body;
    cmd->enc = enc;
    cmd->files = files;
    cmd->via = NEREV_STANDARD;
    cmd->submit = 1;

	NBK_callLater(page->platform, page->id, doc_do_load, cmd);
}

//void doc_doIa(NDocument* doc, NUpCmd* upcmd, NFileUpload* files)
//{
//    NPage* page = (NPage*)doc->page;
//    NLoadCmd* cmd = (NLoadCmd*)NBK_malloc0(sizeof(NLoadCmd));
//    
//    cmd->doc = doc;
//    cmd->upcmd = upcmd;
//    cmd->files = files;
//    cmd->via = page->mode;
//    cmd->submit = 1;
//    
//    tim_stop(doc->loadTimer);
//    delete_load_timer_user_data(doc->loadTimer);
//    tim_setCallback(doc->loadTimer, doc_do_load, cmd);
//    tim_start(doc->loadTimer, 5, 5000);
//}

static void doc_do_back(void* user)
{
    NLoadCmd* cmd = (NLoadCmd*)user;
    history_prev((NHistory*)((NPage*)cmd->doc->page)->history);
	loadCmd_delete(&cmd);
}

void doc_doBack(NDocument* doc)
{
	NPage* page = (NPage*)doc->page;
    NLoadCmd* cmd = loadCmd_create();
    cmd->doc = doc;
	NBK_callLater(page->platform, page->id, doc_do_back, cmd);
}

void doc_setUrl(NDocument* doc, const char* url)
{
    int len = nbk_strlen(url) + 1;
    int size = doc->urlMaxLen;

    if (len > size) {
        while (len > size) {
            if (size == 0)
                size = 128;
            else
                size <<= 1;
        }
        
        if (doc->url)
            NBK_free(doc->url);
        doc->url = (char*)NBK_malloc(size);
        doc->urlMaxLen = (int16)size;
    }
    
    nbk_strcpy(doc->url, url);
}

char* doc_getUrl(NDocument* doc)
{
    if (doc->url && doc->url[0])
        return doc->url;
    else
        return N_NULL;
}

wchr* doc_getTitle(NDocument* doc)
{
    return doc->title;
}

nid doc_getType(NDocument* doc)
{
    return doc->type;
}

void doc_clickFocusNode(NDocument* doc, NNode* focus)
{
	if (focus && focus->Click)
		focus->Click(focus, doc);
}

void doc_viewChange(NDocument* doc)
{
    //dump_char(g_dp, "=== update pictures with layout ===", -1);
    //dump_return(g_dp);
    doc->picUpdate = 1;
    doc->layoutFinish = 0;
    doc_scheduleLayout(doc);
}

void doc_modeChange(NDocument* doc, NEDocType type)
{
    NPage* page = (NPage*)doc->page;
    
    doc->type = type;
	page->mode = NEREV_STANDARD;
	if (page->main) {
		page->view->imgPlayer->interval = 2000;
		page->doc->wordCount = 200;
	}
}

// when path exists except the leaf, sibling set to last valid sibling
NNode* dom_query(NDocument* doc, NXPathCell* xpath, NNode** sibling, NNode** parent, nbool ignoreTag)
{
    NNode *body, *n, *t, *l;
    int i, j;
    nbool found;

    if (doc->root == N_NULL || doc->root->child == N_NULL)
        return N_NULL;
    
    if (sibling)
        *sibling = N_NULL;
    if (parent)
        *parent = N_NULL;
    
    body = node_getByTag(doc->root, TAGID_BODY);
    if (body == N_NULL)
        return N_NULL;
    if (body->child == N_NULL) {
        if (ignoreTag && xpath[0].id && !xpath[1].id)
            *parent = body;
        return N_NULL;
    }
    
    i = j = 0;
    n = body->child;
    t = l = N_NULL;
    while (n && xpath[i].id) {
        
        if (ignoreTag && xpath[i+1].id == 0) {
            xpath[i].id = N_INVALID_ID;
        }
        
        j = 0;
        found = 0;
        while (n) {
            if (xpath[i].id == N_INVALID_ID || n->id == xpath[i].id) {
                j++;
//                n->test = j;
                l = n;
                if (j == xpath[i].idx) {
                    found = 1;
                    break;
                }
            }
            n = n->next;
        }
        
        if (found) {
            t = n;
            n = n->child;
            i++;
        }
    }
    
    if (t && xpath[i].id == 0)
        return t;
    
    if (!n && (j == xpath[i].idx - 1) && (xpath[i + 1].id == 0) && sibling)
        *sibling = l;
    
    if (ignoreTag && xpath[i+1].id == 0 && t && parent)
        *parent = t;

    return N_NULL;
}

static void dom_delete_node(NDocument* doc, NNode* node)
{
    NNode* parent = node->parent;
    NNode* prev = node->prev;
    NNode* next = node->next;
    
    if (parent && parent->child == node)
        parent->child = next;
    if (prev)
        prev->next = next;
    if (next)
        next->prev = prev;
    
    dom_delete_tree(doc, node);
}

static nbool dom_replace_node(NDocument* doc, NNode* old, NNode* novice)
{
    NNode* parent = old->parent;
    NNode* prev = old->prev;
    NNode* next = old->next;
    
    novice->parent = parent;
    novice->prev = prev;
    novice->next = next;
    
    if (parent->child == old)
        parent->child = novice;
    if (prev)
        prev->next = novice;
    if (next)
        next->prev = novice;
    
    dom_delete_tree(doc, old);
    
    return N_TRUE;
}

static nbool dom_insert_node(NNode* node, NNode* novice, nbool before)
{
    if (node == N_NULL || novice == N_NULL)
        return N_FALSE;
    
    novice->parent = node->parent;
    
    if (before) {
        novice->prev = node->prev;
        novice->next = node;
        if (node->prev)
            node->prev->next = novice;
        else
            node->parent->child = novice;
        node->prev = novice;
    }
    else {
        novice->prev = node;
        novice->next = node->next;
        if (node->next)
            node->next->prev = novice;
        node->next = novice;
    }
    
    return N_TRUE;
}

static nbool dom_insert_node_as_child(NNode* parent, NNode* novice)
{
    if (parent == N_NULL || novice == N_NULL)
        return N_FALSE;
    
    parent->child = novice;
    novice->parent = parent;
    
    return N_TRUE;
}

nbool doc_processKey(NEvent* event)
{
    NPage* page = (NPage*)event->page;
    nbool ret = N_FALSE;
    
    if (page->view->focus) {
        NRenderNode* r = (NRenderNode*)page->view->focus->render;
        if (r == N_NULL)
            return N_FALSE;
        
        switch (r->type) {
        case RNT_INPUT:
            ret = renderInput_processKey(r, event);
            if (ret)
                nbk_cb_call(NBK_EVENT_PAINT_CTRL, &page->cbEventNotify, N_NULL);
            break;
            
        case RNT_TEXTAREA:
            ret = renderTextarea_processKey(r, event);
            if (ret)
                nbk_cb_call(NBK_EVENT_PAINT_CTRL, &page->cbEventNotify, N_NULL);
            break;
            
        case RNT_SELECT:
            ret = renderSelect_processKey(r, event);
            if (ret)
                nbk_cb_call(NBK_EVENT_UPDATE_PIC, &page->cbEventNotify, N_NULL);
            break;
        }
    }

    return ret;
}

// event position in view coordinate
nbool doc_processPen(NEvent* event)
{
    return doc_processKey(event);
}

char* doc_getFrameName(NDocument* doc)
{
    NPage* page = (NPage*)doc->page;
    NNode* n = node_getByTag(doc->root, TAGID_BODY);
    if (n) {
        char* name = attr_getValueStr(n->atts, ATTID_NAME);
        return name;
    }
    else
        return N_NULL;
}

typedef struct _NTaskJumpFindRet {
    char*   name;
    NNode*  node;
    coord   x;
    coord   y;
} NTaskJumpFindRet;

static int jump_find_anchor(NNode* node, void* user, nbool* ignore)
{
    NTaskJumpFindRet* find = (NTaskJumpFindRet*)user;
    
    if (find->node) {
        if (node->render) {
            NRenderNode* r = (NRenderNode*)node->render;
            NRect rr;
            renderNode_absRect(r, &rr);
            find->x = rr.l;
            find->y = rr.t;
            return 1;
        }
    }
    else if (node->id == TAGID_A) {
        char* name = attr_getValueStr(node->atts, ATTID_NAME);
        if (name == N_NULL)
            name = attr_getValueStr(node->atts, ATTID_ID);
        if (name && nbk_strcmp(find->name, name) == 0)
            find->node = node;
    }
    
    return 0;
}

void doc_jumpTo(NDocument* doc, const char* name)
{
    NPage* page = (NPage*)doc->page;
    NTaskJumpFindRet find;

    if (nbk_strcmp(name, "top") == 0) {
        find.node = doc->root;
        find.x = find.y = 0;
    }
    else {
        find.name = (char*)name;
        find.node = N_NULL;
        find.x = find.y = -1;
        node_traverse_depth(doc->root, jump_find_anchor, N_NULL, &find);
    }
    
    if (find.node && find.x >= 0 && find.y >= 0) {
        NBK_Event_RePosition evt;
        evt.x = find.x;
        evt.y = find.y;
        evt.zoom = history_getZoom((NHistory*)page->history);
        nbk_cb_call(NBK_EVENT_REPOSITION, &page->cbEventNotify, &evt);
    }
}

NStrPool* doc_getStrPool(NDocument* doc)
{
    return doc->spool;
}

void doc_setStrPool(NDocument* doc, NStrPool* spool, nbool own)
{
    if (doc->ownStrPool && doc->spool) {
        strPool_reset(doc->spool);
        strPool_delete(&doc->spool);
    }
    
    doc->spool = spool;
    doc->ownStrPool = own;
}

//static void doc_do_msgwin(void* user)
//{
//    NLoadCmd* cmd = (NLoadCmd*)user;
//    NPage* page = (NPage*)cmd->doc->page;
//    
//    tim_stop(cmd->doc->loadTimer);
//    tim_setCallback(cmd->doc->loadTimer, doc_do_load, N_NULL); // restore
//    
//    if (cmd->dlg_type == 1) {
//        // alert
//        NBK_dlgAlert(page->platform, cmd->dlg_msg, cmd->dlg_msg_len);
//    }
//    else if (cmd->dlg_type == 2) {
//        // confirm
//        int ret = 0;
//        if (NBK_dlgConfirm(page->platform, cmd->dlg_msg, cmd->dlg_msg_len, &ret)) {
//            NNode* focus = (NNode*)view_getFocusNode(page->view);
//            if (focus) {
//                NEvent evt;
//                NBK_memset(&evt, 0, sizeof(NEvent));
//                evt.type = NEEVENT_DOM;
//                evt.page = page;
//                evt.d.domEvent.type = NEDOM_DIALOG;
//                evt.d.domEvent.node = focus;
//                evt.d.domEvent.d.dlg.type = 2;
//                evt.d.domEvent.d.dlg.choose = ret;
//                node_active_with_ia(page->doc, &evt, N_NULL);
//            }
//        }
//    }
//    else if (cmd->dlg_type == 3) {
//        // prompt
//        int ret = 0;
//        char* input = N_NULL;
//        if (NBK_dlgPrompt(page->platform, cmd->dlg_msg, cmd->dlg_msg_len, &ret, &input)) {
//            NNode* focus = (NNode*)view_getFocusNode(page->view);
//            if (focus) {
//                NEvent evt;
//                NBK_memset(&evt, 0, sizeof(NEvent));
//                evt.type = NEEVENT_DOM;
//                evt.page = page;
//                evt.d.domEvent.type = NEDOM_DIALOG;
//                evt.d.domEvent.node = focus;
//                evt.d.domEvent.d.dlg.type = 3;
//                evt.d.domEvent.d.dlg.choose = ret;
//                if (ret)
//                    evt.d.domEvent.d.dlg.input = input;
//                node_active_with_ia(page->doc, &evt, N_NULL);
//            }
//            if (input)
//                NBK_free(input);
//        }
//    }
//
//    delete_load_cmd(cmd);
//}
//
//void doc_doMsgWin(NDocument* doc, int type, char* message)
//{
//    NLoadCmd* cmd = (NLoadCmd*)NBK_malloc0(sizeof(NLoadCmd));
//    
//    cmd->doc = doc;
//    cmd->dlg_type = type;
//    if (message)
//        cmd->dlg_msg = uni_utf8_to_utf16_str((uint8*)message, -1, &cmd->dlg_msg_len);
//    
//    delete_load_timer_user_data(doc->loadTimer);
//    tim_setCallback(doc->loadTimer, doc_do_msgwin, cmd);
//    tim_start(doc->loadTimer, 5, 10000);
//}

#if DUMP_FUNCTIONS
static int output_xml_start_tag(NNode* node, void* user, nbool* ignore)
{
    if (node->id != TAGID_TEXT) {
        NPage* page = (NPage*)user;
        char tag[MAX_TAG_LEN + 1];
        int i;
        tag[0] = '<';
        nbk_strcpy(&tag[1], xml_getTagNames()[node->id]);
        i = nbk_strlen(tag);
        if (node_is_single(node->id))
            tag[i++] = '/';
        tag[i++] = '>';
        tag[i] = 0;
        dump_char(page, tag, i);
    }
    return 0;
}

static int output_xml_end_tag(NNode* node, void* user)
{
    NPage* page = (NPage*)user;
    char tag[MAX_TAG_LEN + 1];
    int i;
    tag[0] = '<';
    tag[1] = '/';
    nbk_strcpy(&tag[2], xml_getTagNames()[node->id]);
    i = nbk_strlen(tag);
    tag[i++] = '>';
    tag[i] = 0;
    dump_char(page, tag, i);
    return 0;
}
#endif

void doc_outputXml(NDocument* doc)
{
#if DUMP_FUNCTIONS
    NPage* page = (NPage*)doc->page;
    dump_tab(page, N_FALSE);
    dump_return(page);
    dump_return(page);
    node_traverse_depth(doc->root, output_xml_start_tag, output_xml_end_tag, page);
    dump_return(page);
    dump_return(page);
    dump_tab(page, N_TRUE);
    dump_flush(page);
#endif
}

void doc_attachWbxmlDecoder(NDocument* doc, int length)
{
    if (doc->wbxmlDecoder)
        doc_detachWbxmlDecoder(doc);
    
    doc->wbxmlDecoder = wbxmlDecoder_create();
    doc->wbxmlDecoder->page = doc->page; // for log
    wbxmlDecoder_setSourceLength(doc->wbxmlDecoder, length);
}

void doc_detachWbxmlDecoder(NDocument* doc)
{
    if (doc->wbxmlDecoder)
        wbxmlDecoder_delete(&doc->wbxmlDecoder);
}

void doc_handleBySelf(NDocument* doc)
{
    doc->tokenizer->checkJs = 0;
}

NLoadCmd* loadCmd_create(void)
{
	NLoadCmd* c = (NLoadCmd*)NBK_malloc0(sizeof(NLoadCmd));
	smartptr_init(c, (SP_FREE_FUNC)loadCmd_delete);
	return c;
}

void loadCmd_delete(NLoadCmd** cmd)
{
	NLoadCmd* c = *cmd;

    if (c->url)
        NBK_free(c->url);
    if (c->files)
        NBK_free(c->files);
        
    NBK_free(c);
	*cmd = N_NULL;
}
