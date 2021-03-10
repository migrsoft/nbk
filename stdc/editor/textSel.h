// Create by wuyulun, 2012.3.31

#ifndef __TEXTSEL_H__
#define __TEXTSEL_H__

#include "../inc/config.h"
#include "../inc/nbk_gdi.h"
#include "../dom/event.h"
#include "../dom/view.h"
#include "../render/renderNode.h"
#include "../tools/dlist.h"

#define TEXT_SEL_AREA_NUM   3

#ifdef __cplusplus
extern "C" {
#endif

enum NETSDrag {
    NETS_DRAG_NONE,
    NETS_DRAG_HEAD,
    NETS_DRAG_TAIL,
    NETS_DRAG_VIEW
};

typedef struct _NTextSel {

    NView*  view; // not owned

    NDList* rlst; // 文本渲染结点表

    nbool    init;
    nbool    useKey; // 按键操作方式
    nbool    moveH; // 操作头标
    uint8   drag; // 拖动方式

    NRect   sa[TEXT_SEL_AREA_NUM];

    NDLNode*    h; // 选择的起始结点
    NDLNode*    t; // 选择的结束结点
    int         h_offs; // 选择的起始结点内部文本偏移
    int         t_offs; // 选择的结束结点内部文本偏移

    NPoint  lastPos; // 上次的笔触位置

    coord   dy; // 文本选择范围检测，Y轴调节量

    NFontId fid;
    NPoint  countPt; // 字数统计提示绘制点

} NTextSel;

NTextSel* textSel_begin(NView* view);
void textSel_end(NTextSel** sel);

// note: 触控方式与按键方式不能混用
void textSel_setStartPoint(NTextSel* sel, const NPoint* point); // 触控方式
void textSel_useKey(NTextSel* sel, const NPoint* point); // 按键方式

nbool textSel_processEvent(NTextSel* sel, NEvent* event);
void textSel_paint(NTextSel* sel, NRect* rect);

wchr* textSel_getText(NTextSel* sel, int* len);

#ifdef __cplusplus
}
#endif

#endif
