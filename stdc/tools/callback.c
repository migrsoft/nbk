/*
 * nbk_callback.c
 *
 *  Created on: 2010-12-30
 *      Author: wuyulun
 */

#include "../inc/nbk_callback.h"

int nbk_cb_call(int id, NBK_Callback* cb, void* info)
{
    if (cb->func)
        return cb->func(id, cb->user, info);
    
    return 0;
}
