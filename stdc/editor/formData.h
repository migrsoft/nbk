// create by wuyulun, 2012.4.3

#ifndef __FORMDATA_H__
#define __FORMDATA_H__

#include "../inc/config.h"
#include "../dom/page.h"
#include "../dom/view.h"
#include "../tools/slist.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NFormData {
    NSList* lst;
} NFormData;

NFormData* formData_create(void);
void formData_delete(NFormData** data);
void formData_reset(NFormData* data);

void formData_save(NFormData* data, NPage* page);
void formData_restore(NFormData* data, NPage* page);
void formData_restoreLoginData(NPage* page);

// 保存/读取用户登录数据（用户名、口令）
//void NBK_saveLoginData(void* pfd, char* domain, wchr* user, wchr* pw);
//void NBK_loadLoginData(void* pfd, char* domain, wchr** user, wchr** pw);

#ifdef __cplusplus
}
#endif

#endif
