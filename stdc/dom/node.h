#ifndef __NODE_H__
#define __NODE_H__

#include "../inc/config.h"
#include "../inc/nbk_public.h"
#include "../css/cssSelector.h"
#include "../tools/slist.h"
#include "attr.h"
#include "event.h"
#include "token.h"

#ifdef __cplusplus
extern "C" {
#endif
    
typedef struct _NNode {

    nid         id;
    int16       len; // for text
    
    union {
        wchr*   text;
    } d;
    
    NAttr*      atts;
    
    uint8       space : 1; // pure spaces
    
    void*       render; // type is RenderNode

    struct _NNode*   parent;
    struct _NNode*   child;
    struct _NNode*   prev; // previous sibling
    struct _NNode*   next; // next sibling
    
    void (*Click)(struct _NNode* node, void* doc);
    void (*OnEvent)(struct _NNode* node, NEvent* event);
    
    // test only
#if DOM_TEST_INFO
    int __no;
#endif
    
} NNode;

#ifdef NBK_MEM_TEST
int node_memUsed(const NNode* node, int* txtLen, int* styLen);
#endif

NNode* node_create(NToken* token, int no);
void node_delete(NNode** node);

void node_calcStyle(void* view, NNode* node, void* style);

NNode* node_getByTag(NNode* root, const nid tagId);
NSList* node_getListByTag(NNode* root, const nid* tags);
NNode* node_getParentById(NNode* node, nid id);

// Depth-first tree traverse
typedef int (*DOM_TRAVERSE_START_CB)(NNode* node, void* user, nbool* ignore);
typedef int (*DOM_TRAVERSE_CB)(NNode* node, void* user);
void node_traverse_depth(NNode* root, DOM_TRAVERSE_START_CB startCb, DOM_TRAVERSE_CB endCb, void* user);

nbool node_is_single(nid id);

void nodeA_click(NNode* node, void* doc);
void nodeInput_click(NNode* node, void* doc);
void nodeAnchor_click(NNode* node, void* doc);
void nodeSelect_click(NNode* node, void* doc);
void nodeTextarea_click(NNode* node, void* doc);
void nodeDiv_click(NNode* node, void* doc);
void nodeImg_click(NNode* node, void* doc);
void nodeAttachment_click(NNode* node, void* doc);

int node_getEditLength(NNode* node);
int node_getEditMaxLength(NNode* node);
void node_cancelEditing(NNode* node, void* page);

#ifdef __cplusplus
}
#endif

#endif
