/*
 * nbk_callback.h
 *
 *  Created on: 2010-12-30
 *      Author: wuyulun
 */

#ifndef NBK_CALLBACK_H_
#define NBK_CALLBACK_H_

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * id:      the type of callback
 * user:    the data of caller
 * info:    the data send to caller
 */
typedef int (*NBK_CALLBACK)(int id, void* user, void* info);
    
typedef struct _NBK_Callback {
    
    NBK_CALLBACK    func;
    void*           user; // data need by caller
    
} NBK_Callback;

int nbk_cb_call(int id, NBK_Callback* cb, void* info);

#ifdef __cplusplus
}
#endif

#endif /* NBK_CALLBACK_H_ */
