#include "memMgr.h"
#include "memAlloc.h"
#include "../render/renderInc.h"

typedef struct _NMemMgr {

	int		renderSize;

	NMemAlloc*	nodeAlloc;
	NMemAlloc*	renderAlloc;

} NMemMgr;

static NMemMgr* l_mem_mgr = N_NULL;

static int render_max_size(void)
{
	int s = sizeof(NRenderNode);

	s = N_MAX(s, sizeof(NRenderBlock));
	s = N_MAX(s, sizeof(NRenderInline));
	s = N_MAX(s, sizeof(NRenderInlineBlock));

	s = N_MAX(s, sizeof(NRenderText));
	s = N_MAX(s, sizeof(NRenderTextPiece));
	s = N_MAX(s, sizeof(NRenderImage));
	s = N_MAX(s, sizeof(NRenderBr));
	s = N_MAX(s, sizeof(NRenderHr));

	s = N_MAX(s, sizeof(NRenderA));
	s = N_MAX(s, sizeof(NRenderObject));
	s = N_MAX(s, sizeof(NRenderBlank));

	s = N_MAX(s, sizeof(NRenderTable));
	s = N_MAX(s, sizeof(NRenderTr));
	s = N_MAX(s, sizeof(NRenderTd));

	s = N_MAX(s, sizeof(NRenderInput));
	s = N_MAX(s, sizeof(NRenderTextarea));
	s = N_MAX(s, sizeof(NRenderSelect));

	return s;
}

void memMgr_init(void)
{
	if (l_mem_mgr == N_NULL) {
		NMemMgr* mgr = (NMemMgr*)NBK_malloc0(sizeof(NMemMgr));
		mgr->nodeAlloc = memAlloc_create(sizeof(NNode), 512);
		mgr->renderSize = render_max_size();
		mgr->renderAlloc = memAlloc_create(mgr->renderSize, 512);
		l_mem_mgr = mgr;
	}
}

void memMgr_quit(void)
{
	if (l_mem_mgr) {
		NMemMgr* mgr = l_mem_mgr;
		l_mem_mgr = N_NULL;

		memAlloc_delete(&mgr->nodeAlloc);
		memAlloc_delete(&mgr->renderAlloc);
		NBK_free(mgr);
	}
}

void* node_alloc(void)
{
	return memAlloc_alloc(l_mem_mgr->nodeAlloc, sizeof(NNode));
}

void node_free(void* ptr)
{
	memAlloc_free(l_mem_mgr->nodeAlloc, ptr);
}

void* render_alloc(void)
{
	return memAlloc_alloc(l_mem_mgr->renderAlloc, l_mem_mgr->renderSize);
}

void render_free(void* ptr)
{
	memAlloc_free(l_mem_mgr->renderAlloc, ptr);
}
