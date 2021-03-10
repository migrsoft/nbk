/*
 * event.h
 *
 *  Created on: 2011-3-27
 *      Author: migr
 */

#ifndef EVENT_H_
#define EVENT_H_

#include "../inc/config.h"
#include "../inc/nbk_gdi.h"

#ifdef __cplusplus
extern "C" {
#endif

enum NEKeyType {
    NEKEY_NONE,
    NEKEY_UP,
    NEKEY_DOWN,
    NEKEY_LEFT,
    NEKEY_RIGHT,
    NEKEY_ENTER,
    NEKEY_BACKSPACE,
    NEKEY_CHAR
};

typedef struct _NKeyEvent {
    nid     key;
    wchr    chr;
} NKeyEvent;

enum NEPenType {
    NEPEN_NONE,
    NEPEN_MOVE,
    NEPEN_DOWN,
    NEPEN_UP
};

typedef struct _NPenEvent {
    nid     type;
    NPoint  pos;
} NPenEvent;

enum NEDomEventType {
    NEDOM_NONE,
    NEDOM_CLICK,
    NEDOM_MOUSE_OVER,
    NEDOM_MOUSE_MOVE,
    NEDOM_MOUSE_OUT,
    NEDOM_DIALOG,
    NEDOM_SELECT_CHANGE,
    NEDOM_LAST
};

typedef struct _NDomDlgData {
    int     type;
    int     choose;
    char*   input;
} NDomDlgData;

typedef struct _NDomEvent {
    nid     type;
    void*   node; // NNode
    
    union {
        NDomDlgData dlg;
    } d;
    
} NDomEvent;

enum NEEventType {
    NEEVENT_UNKNOWN,
    NEEVENT_KEY,
    NEEVENT_PEN,
    NEEVENT_DOM,
    NEEVENT_LAST
};

typedef struct _NEvent {
    
    nid     type;
    void*   page;

    union {
        NKeyEvent   keyEvent;
        NPenEvent   penEvent;
        NDomEvent   domEvent;
    } d;
    
} NEvent;

#ifdef __cplusplus
}
#endif

#endif /* EVENT_H_ */
