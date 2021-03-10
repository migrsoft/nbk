#ifndef __NBK_INI_H__
#define __NBK_INI_H__

#include "../stdc/inc/config.h"

#define NBK_MAX_PATH	512

enum NEIniKey {
	NEINI_DATA_PATH,
	NEINI_TTF_FONT,
	NEINI_HOMEPAGE,
	NEINI_TESTPAGE,
	NEINI_PAGE_ERROR,
	NEINI_PAGE_404,

	NEINI_WIDTH,
	NEINI_HEIGHT,
	NEINI_FONT_SIZE,
	NEINI_IMAGE_OFF,
    NEINI_IMAGE_QUALITY,
	NEINI_INIT_MODE,
	NEINI_SELF_ADAPT,
    NEINI_BLOCK_COLLAPSE,

	NEINI_CBS,
	NEINI_MPIC,
	NEINI_PIC,
    NEINI_TF,

	NEINI_PLATFORM,
	NEINI_FROM,

	NEINI_DBG_HTTP,
	NEINI_DBG_FGET,

	NEINI_DUMP_DOC, // 保存解析的文档
    NEINI_DUMP_IMG, // 保存图片

	NEINI_LAST
};

#ifdef __cplusplus
extern "C" {
#endif

void ini_init(void);
void ini_end(void);

const char* ini_getString(int key);
int ini_getInt(int key);

const char* get_work_path(void);

#ifdef __cplusplus
}
#endif

#endif
