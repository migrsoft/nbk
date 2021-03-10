#ifndef __LOGINDATA_H__
#define __LOGINDATA_H__

#include "../stdc/inc/config.h"
#include "../stdc/tools/slist.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NLoginData {
    NSList* lst;
} NLoginData;

NLoginData* loginData_create(void);
void loginData_delete(NLoginData** login);

void loginData_load(NLoginData* login);
void loginData_save(NLoginData* login);

void loginData_add(NLoginData* login, char* domain, wchr* user, wchr* pw);
void loginData_get(NLoginData* login, char* domain, wchr** user, wchr** pw);

#ifdef __cplusplus
}
#endif

#endif
