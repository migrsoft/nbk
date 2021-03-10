/*
 * netTypeDef.h
 *
 *  Created on: 2011-3-3
 *      Author: wuyulun
 */

#ifndef NETTYPEDEF_H_
#define NETTYPEDEF_H_

#include <cdblen.h>

typedef struct _TEApList
{
    TUint32 iId;
    TBuf<KCommsDbSvrMaxFieldLength> iName;
    TBuf<KCommsDbSvrMaxFieldLength> iApn;
    
} TEApList;

#endif /* NETTYPEDEF_H_ */
