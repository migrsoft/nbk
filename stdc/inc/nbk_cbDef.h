/*
 * nbk_cbDef.h
 *
 *  Created on: 2011-2-10
 *      Author: wuyulun
 */

#ifndef __NBK_CALLBACKDEF_H__
#define __NBK_CALLBACKDEF_H__

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif
    
enum NBKEventType {

    NBK_EVENT_NONE = 0,
    NBK_EVENT_NEW_DOC,                      // 准备加载新文档
    NBK_EVENT_HISTORY_DOC,                  // 准备加载历史页
    NBK_EVENT_DO_IA,                        // 产生交互请求
    NBK_EVENT_WAITING,                      // 发送POST后，等待回应
    NBK_EVENT_GOT_RESPONSE,                 // 接收到回应
    NBK_EVENT_NEW_DOC_BEGIN,                // 首次接收到文档数据
    NBK_EVENT_GOT_DATA,                     // 接收文档数据
    NBK_EVENT_UPDATE,                       // 文档视图更新，文档尺寸改变
    NBK_EVENT_UPDATE_PIC,                   // 文档视图更新，文档尺寸无改变
    NBK_EVENT_LAYOUT_FINISH,                // 文档排版完成
    NBK_EVENT_DOWNLOAD_IMAGE,               // 下载图片进度
    NBK_EVENT_PAINT_CTRL,                   // 通知绘制控件
    NBK_EVENT_ENTER_MAINBODY,               // 进入主体模式
    NBK_EVENT_QUIT_MAINBODY,                // 退出主体模式
    NBK_EVENT_QUIT_MAINBODY_AFTER_CLICK,    // 在主体模式中，产生操作后，自动退出主体模式
    NBK_EVENT_LOADING_ERROR_PAGE,           // 加载内置错误页
    NBK_EVENT_REPOSITION,                   // 文档视图重定位
    NBK_EVENT_GOT_INC,                      // 接收到增量数据
    NBK_EVENT_DOWNLOAD_FILE,                // 检测到附件下载
    NBK_EVENT_TEXTSEL_FINISH,               // 自由复制，尾标选择完成
    NBK_EVENT_DEBUG1,                       // 调试输出
    
    NBK_EVENT_LAST
};

extern char* NBKEventDesc[];

typedef struct _NBK_Event_NewDoc {
    char* url;
} NBK_Event_NewDoc;

typedef struct _NBK_Event_GotResponse {
    int total;
    int received;
} NBK_Event_GotResponse;

typedef struct _NBK_Event_GotImage {
    int total;
    int curr;
    int receivedSize;
    int totalSize;
} NBK_Event_GotImage;

typedef struct _NBK_Event_RePosition {
	int		x;
	int		y;
	float	zoom;
} NBK_Event_RePosition;

typedef struct _NBK_Event_DownloadFile {
    int     size;
    char*   url;
    char*   fileName;
    char*   cookie;
} NBK_Event_DownloadFile;

typedef enum _NBKDbgType {
    
    NBKDBG_NONE,
    NBKDBG_UINT,
    NBKDBG_INT,
    NBKDBG_WCHR,
    NBKDBG_CHAR,
    NBKDBG_RETURN,
    NBKDBG_TIME,
    NBKDBG_FLUSH,
    NBKDBG_TAB
    
} NBKDbgType;

typedef struct _NBK_DbgInfo {
    
    NBKDbgType  t;
    
    union {
        int32   si;
        uint32  ui;
        wchr*   wp;
        char*   cp;
        nbool    on;
    } d;
    int         len; // length of string
    
} NBK_DbgInfo;

#ifdef __cplusplus
}
#endif

#endif /* __NBK_CALLBACKDEF_H__ */
