// create by wuyulun, 2012.2.9

#include "../css/css_val.h"
#include "layoutStat.h"
#include "renderNode.h"
#include "renderBlock.h"

#define MAX_AA_NUM	16

NLayoutStat* layoutStat_create(void)
{
	NLayoutStat* s = (NLayoutStat*)NBK_malloc0(sizeof(NLayoutStat));
	int i, j;

	s->tre = (NRenderNode**)NBK_malloc0(sizeof(NRenderNode*) * MAX_TREE_DEPTH);
	s->lay = (NRenderNode**)NBK_malloc0(sizeof(NRenderNode*) * MAX_TREE_DEPTH);
    s->tp = s->lp = -1;

	s->asp = -1;
    s->store = (NAvailArea*)NBK_malloc0(sizeof(NAvailArea) * MAX_AA_NUM * (MAX_TREE_DEPTH + 1));

	for (i=j=0; i < MAX_TREE_DEPTH; i++, j += MAX_AA_NUM)
        s->aas[i].aal = &s->store[j];

    s->faal = &s->store[j];

	return s;
}

void layoutStat_delete(NLayoutStat** stat)
{
	NLayoutStat* s = *stat;

	NBK_free(s->tre);
	NBK_free(s->lay);
	NBK_free(s->store);
	NBK_free(s);

	*stat = N_NULL;
}

// 进入新的布局层
void layoutStat_push(NLayoutStat* stat, NRenderNode* root)
{
	stat->asp++;
    stat->aas[stat->asp].root = root;
    stat->aas[stat->asp].of_h = 0;
}

// 回退上一布局层
void layoutStat_pop(NLayoutStat* stat)
{
	stat->asp--;
}

static void aas_clear_float(NLayoutStat* stat, int asp, uint8 clear)
{
    NAvailArea* caas = stat->aas[asp].aal; // 当前布局层
    int i;

    for (i=0; caas[i].use; i++) {
        if (   caas[i].allowFloat
            && caas[i].r.l > 0
            && clear != CSS_CLEAR_RIGHT )
            caas[i].allowFloat = N_FALSE;

        if (   caas[i].allowFloat
            && caas[i].r.r < stat->aas[asp].width
            && clear != CSS_CLEAR_LEFT )
            caas[i].allowFloat = N_FALSE;
    }

    if (clear == CSS_CLEAR_BOTH) {
        i--;
        if (caas[i].r.t < stat->aas[asp].of_h)
            caas[i].r.t = stat->aas[asp].of_h;
    }
}

// 当产生一个新布局区域后，初始化当前唯一区域尺寸
void layoutStat_init(NLayoutStat* stat, coord width)
{
    NAvailArea* caas;

    N_ASSERT(stat->asp >= 0);

    if (stat->asp == 0) {
        stat->faal[0].use = N_TRUE;
        stat->faal[0].multiline = N_TRUE;
        stat->faal[0].allowFloat = N_TRUE;
        rect_set(&stat->faal[0].r, 0, 0, width, N_MAX_COORD);
    }

    stat->aas[stat->asp].width = width;
    stat->aas[stat->asp].change = N_FALSE;

    caas = stat->aas[stat->asp].aal;
    NBK_memset(caas, 0, sizeof(NAvailArea) * MAX_AA_NUM);

    if (stat->asp > 1 && !stat->aas[stat->asp].root->flo) {
        // 处理其它布局区域的影响

        int i;
        int asp = stat->asp - 1;
        NAvailArea* lass = stat->aas[asp].aal;
        nbool found = N_FALSE;

        if (stat->aas[stat->asp].root->clr)
            aas_clear_float(stat, asp, stat->aas[stat->asp].root->clr);

        for (i=0; lass[i].use; i++) {
            if (    lass[i].multiline
                && (lass[i].r.l > 0 || lass[i].r.r < stat->aas[asp].width) )
            {
                found = N_TRUE;
                break;
            }
        }

        if (found) {
            int j;
            coord dy = lass[i].r.t;
            dy += renderNode_getTopMBP(stat->aas[stat->asp].root);
            for (j=0; lass[i].use; i++, j++) {
                caas[j] = lass[i];
                caas[j].r.t -= dy;
                caas[j].r.r = N_MIN(caas[j].r.r, width);
                if (caas[j].r.b != N_MAX_COORD)
                    caas[j].r.b -= dy;
            }
            return;
        }
    }

	caas[0].use = N_TRUE;
	caas[0].multiline = N_TRUE;
	caas[0].allowFloat = N_TRUE;
	caas[0].r.l = 0;
	caas[0].r.t = 0;
	caas[0].r.r = width;
	caas[0].r.b = N_MAX_COORD;
}

static NAvailArea* aas_get_aa_for_inline(NAvailArea* aa, int from, coord height)
{
    int i;
    for (i=from; aa[i].use; i++) {
        if (!aa[i].multiline)
            return &aa[i];
        if (rect_getHeight(&aa[i].r) >= height)
            return &aa[i];
        if (aa[i+1].r.l <= aa[i].r.l && aa[i+1].r.r >= aa[i].r.r)
            return &aa[i];
    }
    return N_NULL;
}

nbool layoutStat_isLastAA(NLayoutStat* stat, NAvailArea* aa)
{
    return (aa->r.b >= N_MAX_COORD) ? N_TRUE : N_FALSE;
}

// 从当前布局层内，选择一个布局区域
NAvailArea* layoutStat_getAA(NLayoutStat* stat, NRenderNode* rn, coord height)
{
    NAvailArea* caas = stat->aas[stat->asp].aal; // 当前布局层
    NAvailArea* aa = N_NULL;
    int i;

	if (rn->flo) {
        for (i=0; caas[i].use; i++) {
            if (caas[i].allowFloat) {
                aa = &caas[i]; // 返回第一个允许放置浮动块的排版区域
                break;
            }
        }
	}
	else if (renderNode_isBlockElement(rn))	{
        for (i=0; caas[i].use; i++) {
            if (caas[i].multiline) {
                aa = &caas[i];
                break;
            }
        }
	}
	else { // inline element
        aa = aas_get_aa_for_inline(caas, 0, height);
	}

    N_ASSERT(aa);
	return aa;
}

// 从当前布局层内，返回指定区域的下一个
NAvailArea* layoutStat_getNextAA(NLayoutStat* stat, NAvailArea* aa, nbool inl, coord height)
{
    NAvailArea* caas = stat->aas[stat->asp].aal;
    NAvailArea* _aa = N_NULL;
	int i;

	for (i=0; i < MAX_AA_NUM - 1; i++) {
		if (&caas[i] == aa) {
            if (inl) {
                _aa = aas_get_aa_for_inline(caas, i+1, height);
                N_ASSERT(_aa);
                return _aa;
            }
			break;
        }
	}
    
	_aa = &caas[i+1];
    N_ASSERT(_aa->use);
    return _aa;
}

static void aas_shift_down(NAvailArea* aa, int at)
{
    int i;
    for (i = MAX_AA_NUM - 1; i > at; i--) {
        aa[i] = aa[i-1];
    }
}

static void aas_set_no_float(NAvailArea* aa, int at)
{
    int i;
    for (i = at - 1; i >= 0; i--) {
        aa[i].allowFloat = N_FALSE;
    }
}

static void aas_normalize(NAvailArea* aa)
{
    int i, j;
    for (i=0; i < MAX_AA_NUM; i++) {
        if (!aa[i].use || rect_getWidth(&aa[i].r) <= 0 || rect_getHeight(&aa[i].r) <= 0) {
            for (j = i+1; j < MAX_AA_NUM; j++) {
                if (aa[j].use && rect_getWidth(&aa[j].r) > 0 && rect_getHeight(&aa[j].r) > 0)
                    break;
            }
            if (j == MAX_AA_NUM) {
                aa[i].use = N_FALSE;
            }
            else {
                aa[i] = aa[j];
                aa[j].use = N_FALSE;
            }
        }
    }

    i = MAX_AA_NUM - 1;
    if (aa[i].use && aa[i].r.b != N_MAX_COORD)
        N_KILL_ME();
}

// 将左浮元素移至当前行最左边
static void shift_floated_element_to_most_left(NRenderNode* rn)
{
	coord x = 0;
    coord shift = rect_getWidth(&rn->r);
    NRenderNode* r = renderNode_getPrevNode(rn);

    while (r) {
        if (rn->og_r.t != r->og_r.t)
            break;

        if (!r->flo) {
			x = r->r.l;
            r->r.l += shift;
            r->r.r += shift;
        }
        r = renderNode_getPrevNode(r);
    }

    rect_move(&rn->r, x, rn->r.t);
}

static void adjust_pos(NRenderNode* root, coord top, coord dy)
{
    NRenderNode* r = renderNode_getFirstChild(root);
    while (r && !r->needLayout) {
        if (r->r.t >= top) {
            r->r.t += dy;
            r->r.b += dy;
        }
        r = renderNode_getNextNode(r);
    }
}

static void aas_update(NLayoutStat* stat, NRenderNode* rn, NAvailArea* aa, nbool restore, coord of_h)
{
    int i, j;
    NAvailArea* caas;
    NRect ar;

    if (!rn->display) // 忽略不可见元素
        return;

    stat->aas[stat->asp].change = N_TRUE;

    if (stat->asp > 0 && of_h > 0) {
        stat->aas[stat->asp].of_h = N_MAX(stat->aas[stat->asp].of_h, rn->og_r.b + of_h);
    }

    caas = stat->aas[stat->asp].aal;
    for (i=0; i < MAX_AA_NUM; i++) {
        if (&caas[i] == aa)
            break;
        if (!rn->flo)
            caas[i].use = N_FALSE;
    }

    if (rn->r.b <= aa->r.b) {
        // 排版元素包含于区域内，将区域拆分

        if (rn->flo) {
            if (rn->flo == CSS_FLOAT_LEFT) {
                if (aa->multiline) {
                    ar = aa->r;
                    if (caas[i+1].use) {
                        ar.t = rn->r.b;
                        aa->r.l = rn->r.r;
                        aa->r.b = rn->r.b;
                        i++;
                    }
                    else { // init
                        ar.l = rn->r.r;
                        ar.b = rn->r.b;
                        aa->r.t = rn->r.b;
                    }
                    aas_shift_down(caas, i);
                    caas[i].use = N_TRUE;
                    caas[i].multiline = N_TRUE;
                    caas[i].allowFloat = N_TRUE;
                    caas[i].r = ar;
                }
                else {
                    aa->r.l = rn->r.r;
                    aa->allowFloat = N_TRUE;
                    if (!restore)
                        shift_floated_element_to_most_left(rn);
                }
            }
            else { // CSS_FLOAT_RIGHT
				if (aa->multiline) {
                    ar = aa->r;
                    if (caas[i+1].use) {
                        aa->r.r = rn->r.l;
                        aa->r.b = rn->r.b;
                        ar.t = rn->r.b;
                        i++;
                    }
                    else { // init
                        ar.r = rn->r.l;
                        ar.b = rn->r.b;
                        aa->r.t = rn->r.b;
                    }
                    aas_shift_down(caas, i);
                    caas[i].use = N_TRUE;
                    caas[i].multiline = N_TRUE;
                    caas[i].allowFloat = N_TRUE;
                    caas[i].r = ar;
				}
				else {
                    aa->r.r = rn->r.l;
                    aa->allowFloat = N_TRUE;
				}
            }

            aas_set_no_float(caas, i-1);
        }
        else {
            if (renderNode_isBlockElement(rn)) {
                caas[i].r.t = rn->r.b;
            }
            else if (caas[i+1].use) {
                if (aa->multiline) {
                    ar = aa->r;
                    ar.l = rn->r.r;
                    ar.b = rn->r.b;
                    aa->r.t = rn->r.b;
                    aas_shift_down(caas, i);
                    caas[i].use = N_TRUE;
                    caas[i].multiline = N_FALSE;
                    caas[i].allowFloat = N_TRUE;
                    caas[i].r = ar;
                }
                else {
                    aa->r.l = rn->r.r;
                    aa->use = N_TRUE;
                    aa->multiline = N_FALSE;
                    aa->allowFloat = N_TRUE;
                }
            }
            else {
                ar = aa->r;
                ar.l = rn->r.r;
                ar.b = rn->r.b;

                aa->r.t = rn->r.b;
                aa->use = N_TRUE;
                aa->multiline = N_TRUE;
                aa->allowFloat = N_TRUE;

                aas_shift_down(caas, i);
                caas[i].use = N_TRUE;
                caas[i].multiline = N_FALSE;
                caas[i].allowFloat = N_TRUE;
                caas[i].r = ar;
            }
        }
    }
    else {
        // 调整当前排版区域及下一区域

        if (rn->flo) {
			if (rn->flo == CSS_FLOAT_LEFT) {
                aa->r.l = rn->r.r;
                aa->r.b = rn->r.b;
                caas[i+1].r.t = rn->r.b;
				if (aa->multiline) {
					// do nothing
				}
				else {
					aa->r.l = rn->r.r;
					aa->r.b = rn->r.b;
					aa->allowFloat = N_TRUE;
                    if (!restore)
					    shift_floated_element_to_most_left(rn);
				}
			}
			else { // CSS_FLOAT_RIGHT
                aa->r.r = rn->r.l;
                aa->r.b = rn->r.b;
                for (j=i+1; caas[j].use; j++)
                    caas[j].r.t = rn->r.b;
			}

            aas_set_no_float(caas, i-1);
        }
        else {
            int top = caas[i+1].r.t;
            int dy = rn->r.b - top;

            aa->r.l = rn->r.r;
            aa->r.b = rn->r.b;
            aa->multiline = N_FALSE;
            aa->allowFloat = N_TRUE;

            for (j=i+1; caas[j].use; j++) {
                if (caas[j].r.t < rn->r.b) {
                    caas[j].r.t = rn->r.b;
                    caas[j].r.b += dy;
                }
                else
                    caas[j].r.t += dy;
                caas[j].multiline = N_TRUE;
                caas[j].allowFloat = N_TRUE;
            }

            if (!restore && stat->aas[stat->asp].root)
                adjust_pos(stat->aas[stat->asp].root, top, dy);
        }
    }

    aas_normalize(caas);
}

// 更新当前布局层
void layoutStat_updateAA(NLayoutStat* stat, NRenderNode* rn, NAvailArea* aa)
{
    coord of_h = 0;
    if (rn->type == RNT_BLOCK)
        of_h = ((NRenderBlock*)rn)->overflow_h;
    aas_update(stat, rn, aa, N_FALSE, of_h);
}

static void aas_update_float(NLayoutStat* stat, const NRect* r)
{
    NAvailArea* ga = stat->faal;
}

// 将子布局层应用至父层
void layoutStat_uniteAA(NLayoutStat* stat, NRenderNode* rn)
{
    int asp = stat->asp + 1;
    int i;
    NAvailArea* sa;
    NRect r;

    return;

    if (rn->type != RNT_BLOCK || !stat->aas[asp].change)
        return;

    //if (stat->aas[asp].change && rn->type == RNT_BLOCK) {
    //    coord dy = rn->og_r.t;
    //    NAvailArea* pa = stat->aas[stat->asp].aal;
    //    NAvailArea* sa = stat->aas[asp].aal;
    //    int i;
    //    for (i=0; sa[i].use; i++) {
    //        pa[i] = sa[i];
    //        pa[i].r.t += dy;
    //        if (pa[i].r.r == stat->aas[asp].width)
    //            pa[i].r.r = stat->aas[stat->asp].width;
    //        if (pa[i].r.b != N_MAX_COORD)
    //            pa[i].r.b += dy;
    //    }
    //}

    sa = stat->aas[asp].aal;
    for (i=0; sa[i].use && sa[i].r.b < N_MAX_COORD; i++) {
        if (sa[i].r.l > 0) {
            r.l = 0;
            r.t = sa[i].r.t;
            r.r = sa[i].r.l;
            r.b = sa[i].r.b;
        }
        rect_move(&r, r.l + rn->r.l, r.t + rn->r.t);
        aas_update_float(stat, &r);
    }
}

void layoutStat_return(NLayoutStat* stat, coord height)
{
    NAvailArea* caas = stat->aas[stat->asp].aal;
    if (caas[0].multiline)
        caas[0].r.t += height;
    else
        caas[0].use = N_FALSE;
    aas_normalize(caas);
}

void layoutStat_rebuildForInline(NLayoutStat* stat, NRenderNode* rn)
{
    NRenderNode r = *rn;
    coord h = rect_getHeight(&r.og_r);
    NAvailArea* aa;

    r.r = r.og_r;
    aa = layoutStat_getAA(stat, &r, h);
    while (!rect_isEquaTopLeft(&aa->r, &r.aa_r))
        aa = layoutStat_getNextAA(stat, aa, N_TRUE, h);
    aas_update(stat, &r, aa, N_TRUE, 0);
}

void layoutStat_rebuildForBlock(NLayoutStat* stat, NRenderNode* rn)
{
    NRenderNode r = *rn;
    NAvailArea* aa;
    coord of_h = 0;

    if (rn->type == RNT_BLOCK)
        of_h = ((NRenderBlock*)rn)->overflow_h;

    r.r = r.og_r;
    aa = layoutStat_getAA(stat, &r, 0);
    while (!rect_isEquaTopLeft(&aa->r, &r.aa_r))
        aa = layoutStat_getNextAA(stat, aa, N_FALSE, 0);
    aas_update(stat, &r, aa, N_TRUE, of_h);
}
