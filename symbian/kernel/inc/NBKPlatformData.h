/*
 * PlatformData.h
 *
 *  Created on: 2011-2-27
 *      Author: wuyulun
 */

#ifndef PLATFORMDATA_H_
#define PLATFORMDATA_H_

typedef struct _NBK_PlatformData {
    
    void*   nbk;
    void*   gdi;
    void*   imageManager;
    void*   fep;
    void*   probe;
    
    void*	ctlPainter;
    void*   dlgImpl;
    
} NBK_PlatformData;

#endif /* PLATFORMDATA_H_ */
