/*
 * upCmd.h
 *
 *  Created on: 2011-3-16
 *      Author: wuyulun
 */

#ifndef UPCMD_H_
#define UPCMD_H_

#include "../inc/config.h"
#include "../tools/slist.h"

#ifdef __cplusplus
extern "C" {
#endif
    
enum NEFFSetting {
    
    NEFFS_COMPRESS = 0x1,
    NEFFS_STREAM = 0x2,
    NEFFS_ALLOW_IMAGE = 0x4,
    NEFFS_INCREASE_MODE = 0x8,
    NEFFS_FF_NARROW = 0x10,
    NEFFS_FF_XHTML = 0x20,
    NEFFS_COLLAPSE = 0x40,
    NEFFS_FIXED_SCREEN = 0x80
};

enum NEFontLevel {
    NEFNTLV_SMALL = 1,
    NEFNTLV_MIDDLE,
    NEFNTLV_BIG
};

enum NEFFIaEvent {

    NEFIEVT_NONE = 0,
    NEFIEVT_CLICK =  0x1,
    NEFIEVT_SUBMIT =  0x2,
    NEFIEVT_CHANGE = 0x4,
    NEFIEVT_MOUSEOVER = 0x8,
    NEFIEVT_MOUSEOUT = 0x10,
    NEFIEVT_MOUSEMOVE = 0x20,
    NEFIEVT_MOUSEDOWN = 0x40,
    NEFIEVT_MOUSEUP = 0x80,
    NEFIEVT_FOCUS = 0x100,
    NEFIEVT_BLUR = 0x200
};

typedef struct _NUpCmd {
    
    uint8*  buf;
    int     cur;
    int     total;
    
    void*   node; // which active this event
    
} NUpCmd;

typedef struct _NUpCmdSet {
    nbool    image : 1;
    nbool    incMode : 1;
    nbool    collapse : 1;
    nbool    fixedScreen : 1;
    nid     via;
} NUpCmdSet;

NUpCmd* upcmd_create(NUpCmdSet* settings);
void upcmd_delete(NUpCmd** cmd);

void upcmd_url(NUpCmd* cmd, const char* url, const char* base);

void upcmd_fontLevel(NUpCmd* cmd, uint32 level);

void upcmd_iaSetting(NUpCmd* cmd, uint32 settings);
void upcmd_iaBasic(NUpCmd* cmd, const char* url, int32 nidx, uint32 type);
void upcmd_iaFormData(NUpCmd* cmd, NSList* lst);
void upcmd_iaSession(NUpCmd* cmd, uint8* data, int32 len);
void upcmd_iaFrameBegin(NUpCmd* cmd, char* name);
void upcmd_iaFrameEnd(NUpCmd* cmd);
void upcmd_iaParams(NUpCmd* cmd, NSList* lst);

void upcmd_multiPics(NUpCmd* cmd, NSList* lst);

void upcmd_formSubmit(NUpCmd* cmd, char* base, char* action, char* charset, char* method, char* enctype);
void upcmd_formFields(NUpCmd* cmd, NSList* lst);
void upcmd_formTextarea(NUpCmd* cmd, char* name, char* text, int len);

void upcmd_userInfo(NUpCmd* cmd);

nbool upcmd_isIa(NUpCmd* cmd);

// for platform use

// *data is free by caller
int upcmd_fileUploadField(char* name, char* path, uint8** data);
// *data is free by caller
int upcmd_fileUploadData(int filesize, uint8** data);

// *data is free by caller
int upcmd_iaFileUpload(int nidx, char* path, int filesize, uint8** data);

void upcmd_iaDialog(NUpCmd* cmd, int type, int choose, char* input);


/**************************************************
 *
 * http form submission
 * 
 */

// return free by caller
char* multipart_contentType(void);
int multipart_field(const char* name, const char* value, char* body);
int multipart_file(const char* name, const char* value, char* body);
int multipart_end(char* body);

// -----------------------------------------------------------------------------
// 平台接口
// -----------------------------------------------------------------------------

//char* NBK_helper_getUserInfo(void);

#ifdef __cplusplus
}
#endif

#endif /* UPCMD_H_ */
