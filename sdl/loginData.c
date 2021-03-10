#include "../stdc/editor/formData.h"
#include "loginData.h"
#include <stdio.h>

typedef struct _NLoginDataItem {
    char*   domain;
    wchr*   user;
    wchr*   pw;
} NLoginDataItem;

typedef struct _NFileItemHdr {
    int domainLen;
    int userLen;
    int pwLen;
} NFileItemHdr;

void NBK_saveLoginData(void* pfd, char* domain, wchr* user, wchr* pw)
{
    printf("save password for domain: %s\n", domain);
}

void NBK_loadLoginData(void* pfd, char* domain, wchr** user, wchr** pw)
{
}

NLoginData* loginData_create(void)
{
    return (NLoginData*)NBK_malloc0(sizeof(NLoginData));
}

void loginData_delete(NLoginData** login)
{
    NLoginData* p = *login;
    NBK_free(p);
    *login = N_NULL;
}

void loginData_load(NLoginData* login)
{
    login->lst = sll_create();
}

void loginData_save(NLoginData* login)
{
}

void loginData_add(NLoginData* login, char* domain, wchr* user, wchr* pw)
{
}

void loginData_get(NLoginData* login, char* domain, wchr** user, wchr** pw)
{
}
