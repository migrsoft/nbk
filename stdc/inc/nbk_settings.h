/*
 * nbk_settings.h
 *
 *  Created on: 2011-5-2
 *      Author: migr
 */

#ifndef NBK_SETTINGS_H_
#define NBK_SETTINGS_H_

#include "config.h"
#include "nbk_gdi.h"

#ifdef __cplusplus
extern "C" {
#endif
    
enum NESupport {
    NESupport,
    NENotSupportByFlyflow,
    NENotSupportByNbk
};

typedef struct _NNightTheme {
    NColor  text;
    NColor  link;
    NColor  background;
} NNightTheme;

typedef struct _NSettings {
    
    int16   mainFontSize; // custom font size
    int8    fontLevel;
    
    nbool    allowImage : 1;
    nbool    recognisePhoneSite : 1;
    nbool    selfAdaptionLayout : 1;
    nbool    fixedScreen : 1;
    
    nbool    clickToLoadImage : 1;

    nid     mode; // FF-Full, FF-Narrow or UCK
    nid     initMode; // direct or TC

    char*   mainBodyStyle;
    
    NNightTheme*    night;
    
    // used internal
    uint8   support;
    uint8   via;
    
} NSettings;
    
#ifdef __cplusplus
}
#endif

#endif /* NBK_SETTINGS_H_ */
