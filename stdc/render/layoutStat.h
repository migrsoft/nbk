#ifndef __LAYOUTSTAT_H__
#define __LAYOUTSTAT_H__

#include "../inc/config.h"
#include "../inc/nbk_limit.h"
#include "../inc/nbk_gdi.h"
#include "../css/cssSelector.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _NRenderNode;

typedef struct _NAvailArea {
	nbool	use;
	nbool	multiline;
	nbool	allowFloat;
	NRect	r;
} NAvailArea;

typedef struct _NAAHdr {
    struct _NRenderNode*	root;
    NAvailArea*     aal;
    coord           width;
    coord           of_h;
    nbool            change;
} NAAHdr;

typedef struct _NLayoutStat {

    NSheet*         sheet; // not onwed

    NAAHdr          aas[MAX_TREE_DEPTH]; // AA stack
    int16			asp; // pointer of AA stack
    NAvailArea*     faal; // AA list for floated
    NAvailArea*     store;
    
    struct _NRenderNode**   tre; // stack for build tree
    struct _NRenderNode**   lay; // stack for layout
    int16   tp; // tree pointer
    int16   lp; // layout pointer
    
} NLayoutStat;

NLayoutStat* layoutStat_create(void);
void layoutStat_delete(NLayoutStat** stat);

void layoutStat_push(NLayoutStat* stat, struct _NRenderNode* root);
void layoutStat_pop(NLayoutStat* stat);

void layoutStat_init(NLayoutStat* stat, coord width);

nbool layoutStat_isLastAA(NLayoutStat* stat, NAvailArea* aa);
NAvailArea* layoutStat_getAA(NLayoutStat* stat, struct _NRenderNode* rn, coord height);
NAvailArea* layoutStat_getNextAA(NLayoutStat* stat, NAvailArea* aa, nbool inl, coord height);
void layoutStat_updateAA(NLayoutStat* stat, struct _NRenderNode* rn, NAvailArea* aa);
void layoutStat_uniteAA(NLayoutStat* stat, struct _NRenderNode* rn);
void layoutStat_return(NLayoutStat* stat, coord height);

void layoutStat_rebuildForInline(NLayoutStat* stat, struct _NRenderNode* rn);
void layoutStat_rebuildForBlock(NLayoutStat* stat, struct _NRenderNode* rn);

#ifdef __cplusplus
}
#endif

#endif
