/*
 * nbk_str.cpp
 *
 *  Created on: 2011-1-2
 *      Author: wuyulun
 */

#include "../../../stdc/inc/config.h"
#include "../../../stdc/tools/str.h"
#include <stdio.h>
#include <stdlib.h>
#include <e32std.h>
#include <string.h>
#include <stdarg.h>
#include "NbkGdi.h"
#include "ResourceManager.h"

int NBK_atoi(const char* s)
{
    return atoi(s);
}

float NBK_atof(const char* s)
{
    return atof(s);
}

void NBK_md5(const char* s, int len, uint8* md5)
{
    TPtrC8 str((uint8*)s, len);
    CResourceManager::GetMd5(str, md5);
}
