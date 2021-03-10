#ifndef __RENDER_CUSTOM_PAINT_H__
#define __RENDER_CUSTOM_PAINT_H__

#include "nbk_gdi.h"

#ifdef __cplusplus
extern "C" {
#endif	

typedef enum _NECtrlState {
	NECS_NORMAL,
	NECS_HIGHLIGHT,
	NECS_INEDIT
} NECtrlState;

// -----------------------------------------------------------------------------
// 表单控件绘制接口
// -----------------------------------------------------------------------------

// 绘制单行输入框
nbool NBK_paintText(void* pfd, const NRect* rect, NECtrlState state, const NRect* clip);

// 绘制复选钮
nbool NBK_paintCheckbox(void* pfd, const NRect* rect, NECtrlState state, nbool checked, const NRect* clip);

// 绘制单选钮
nbool NBK_paintRadio(void* pfd, const NRect* rect, NECtrlState state, nbool checked, const NRect* clip);

// 绘制按钮
nbool NBK_paintButton(void* pfd, const NFontId fontId, const NRect* rect,
                     const wchr* text, int len, NECtrlState state, const NRect* clip);

// 绘制列表正常状态
nbool NBK_paintSelectNormal(void* pfd, const NFontId fontId, const NRect* rect,
                           const wchr* text, int len, NECtrlState state, const NRect* clip);
// 绘制列表展开状态
nbool NBK_paintSelectExpand(void* pfd, const wchr* items, int num, int* sel);

// 绘制多行输入框
nbool NBK_paintTextarea(void* pfd, const NRect* rect, NECtrlState state, const NRect* clip);

// 绘制文件选择控件
nbool NBK_paintBrowse(void* pfd, const NFontId fontId, const NRect* rect,
                     const wchr* text, int len, NECtrlState state, const NRect* clip);

// 折叠块背景绘制接口
nbool NBK_paintFoldBackground(void* pfd, const NRect* rect, NECtrlState state, const NRect* clip);

// -----------------------------------------------------------------------------
// 平台层编辑器功能实现接口
// 编辑状态时，会多次调用该接口
// -----------------------------------------------------------------------------

// 单行编辑
nbool NBK_editInput(void* pfd, NFontId fontId, NRect* rect, const wchr* text, int len,int maxlen,nbool password);
// 多行编辑
nbool NBK_editTextarea(void* pfd, NFontId fontId, NRect* rect, const wchr* text, int len,int maxlen);

// -----------------------------------------------------------------------------
// 模态对话框接口
// -----------------------------------------------------------------------------

// open a file select dialog by framework
// 选择本地文件
// oldFile: 上次选择的文件路径
// newFile: 本次选择的文件路径
nbool NBK_browseFile(void* pfd, const char* oldFile, char** newFile);

// 提示对话框（仅含确定按钮）
nbool NBK_dlgAlert(void* pfd, const wchr* text, int len);
// 验证对话框（含确定、取消按钮）
nbool NBK_dlgConfirm(void* pfd, const wchr* text, int len, int* ret);
// 输入对话框（含单行输入控件，确定、取消按钮）
nbool NBK_dlgPrompt(void* pfd, const wchr* text, int len, int* ret, char** input);

// -----------------------------------------------------------------------------
// 资源接口
// -----------------------------------------------------------------------------

#define RES_TSEL_DOWN   "res://tsel/down"
#define RES_TSEL_UP     "res://tsel/up"

// 绘制程序内置图片
void NBK_drawStockImage(void* pfd, const char* res, const NRect* rect, NECtrlState state, const NRect* clip);

#ifdef __cplusplus
}
#endif

#endif	// __RENDER_CUSTOM_PAINT_H__
