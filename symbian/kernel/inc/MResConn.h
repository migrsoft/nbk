/*
 * MConnection.h
 *
 *  Created on: 2011-2-9
 *      Author: wuyulun
 */

#ifndef MRESCONN_H_
#define MRESCONN_H_

#include "../../../stdc/loader/loader.h"
#include <e32def.h>

class CResourceManager;

class MResConn {
    
    friend class CResourceManager;
    friend class CFfPkgDataSupplier;
    
public:
    typedef enum _TConnState {
        EInit,
        EWaiting,
        EActive,
        EFinished
    } EConnState;
    
    MResConn(NRequest* request)
        : iConnType(ENone), iConnState(EInit), iRequest(request) {}
    virtual ~MResConn() { smartptr_release(iRequest); }
    
    virtual void RC_Submit() = 0;
    virtual void RC_Cancel() = 0;
    
    TBool MainDoc() const { return (iRequest->type == NEREQT_MAINDOC); }
    int Via() const { return iRequest->via; }
    nid PageId() const { return iRequest->pageId; }
    
protected:
    
    typedef enum TConnType {
        ENone,
        EFileConn,
        EHttpConn
    } EConnType;
    
    EConnType       iConnType;
    EConnState      iConnState;
    NRequest*    iRequest;
};

#endif /* MRESCONN_H_ */
