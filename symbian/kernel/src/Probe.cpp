/*
 ============================================================================
 Name		: Probe.cpp
 Author	  : wuyulun
 Version	 : 1.0
 Copyright   : 2010,2011 (C) Baidu.MIC
 Description : CProbe implementation
 ============================================================================
 */

#include "../../../stdc/inc/config.h"
#include "../../../stdc/inc/nbk_cbDef.h"
#include "../../../stdc/tools/str.h"
#include "../../../stdc/tools/unicode.h"
#include "Probe.h"
#include <coemain.h>
#include <f32file.h>
#include <stdio.h>
#include <bautils.h>

#define FLUSH_IMMEDIATELY   0

#define PROBE_BUF_SIZE  4096

_LIT(KDbgFile, "c:\\data\\nbk_dump.txt");

static char* hint_invalid_pointer = "*** NULL ***";

CProbe::CProbe()
{
    iBuffered = ETrue;
    iBufPos = 0;
    iBuffer = (char*)NBK_malloc(PROBE_BUF_SIZE);
    iOutputTab = ETrue;
}

CProbe::~CProbe()
{
    Flush();
    NBK_free(iBuffer);
}

void CProbe::OutputUint(unsigned int i)
{
    NBK_DbgInfo dinfo;
    dinfo.t = NBKDBG_UINT;
    dinfo.d.ui = i;
    Output(&dinfo);
}

void CProbe::OutputInt(int i)
{
    NBK_DbgInfo dinfo;
    dinfo.t = NBKDBG_INT;
    dinfo.d.si = i;
    Output(&dinfo);
}

void CProbe::OutputWchr(unsigned short* wc, int length)
{
    NBK_DbgInfo dinfo;
    dinfo.t = NBKDBG_WCHR;
    dinfo.d.wp = wc;
    dinfo.len = length;
    Output(&dinfo);
}

void CProbe::OutputChar(char* mb, int length)
{
    NBK_DbgInfo dinfo;
    dinfo.t = NBKDBG_CHAR;
    dinfo.d.cp = mb;
    dinfo.len = length;
    Output(&dinfo);
}

void CProbe::OutputTRect(TRect aRect)
{
    _LIT8(KFmt, "(%d,%d,%d,%d)");
    TBuf8<32> txt;
    txt.Format(KFmt, aRect.iTl.iX, aRect.iTl.iY, aRect.iBr.iX, aRect.iBr.iY);

    NBK_DbgInfo dinfo;
    dinfo.t = NBKDBG_CHAR;
    dinfo.d.cp = (char*)txt.Ptr();
    dinfo.len = txt.Length();
    Output(&dinfo);
}

void CProbe::OutputTPoint(TPoint aPoint)
{
    _LIT8(KFmt, "(%d,%d)");
    TBuf8<32> txt;
    txt.Format(KFmt, aPoint.iX, aPoint.iY);

    NBK_DbgInfo dinfo;
    dinfo.t = NBKDBG_CHAR;
    dinfo.d.cp = (char*)txt.Ptr();
    dinfo.len = txt.Length();
    Output(&dinfo);
}

void CProbe::OutputReturn()
{
    NBK_DbgInfo dinfo;
    dinfo.t = NBKDBG_RETURN;
    Output(&dinfo);
}

void CProbe::Output(const TDesC& aText)
{
    OutputWchr((TUint16*)aText.Ptr(), aText.Length());
}

void CProbe::OutputTime()
{
    TTime time;
    time.HomeTime();

    TBuf<64> dateString;
    _LIT(KDateString,"%D%M%Y%/0%1%/1%2%/2%3%/3 %-B%:0%J%:1%T%:2%S%.%*C4%:3%+B");
    time.FormatL(dateString, KDateString);
    
    Output(dateString);
}

void CProbe::Output(void* dbgInfo)
{
    NBK_DbgInfo* dinfo = (NBK_DbgInfo*)dbgInfo;
    int space = 0;
    char* u8 = NULL;
    
    if ((dinfo->t == NBKDBG_WCHR && !dinfo->d.wp) ||
        (dinfo->t == NBKDBG_CHAR && !dinfo->d.cp)) {
        dinfo->t = NBKDBG_CHAR;
        dinfo->d.cp = hint_invalid_pointer;
        dinfo->len = -1;
    }
    
    switch (dinfo->t) {
    case NBKDBG_INT:
    case NBKDBG_UINT:
        space = 10;
        break;
        
    case NBKDBG_WCHR:
    {
        space = (dinfo->len == -1) ? nbk_wcslen(dinfo->d.wp) : dinfo->len;
        u8 = uni_utf16_to_utf8_str(dinfo->d.wp, space, NULL);
        space = nbk_strlen(u8);
    }
        break;
        
    case NBKDBG_CHAR:
        space = (dinfo->len == -1) ? nbk_strlen(dinfo->d.cp) : dinfo->len;
        break;
    }
    space += 1;
    
    if (iBufPos + space > PROBE_BUF_SIZE)
        Flush();
    
    switch (dinfo->t) {
    case NBKDBG_INT:
    {
        TPtr8 num((TUint8*)iBuffer + iBufPos, 0, space);
        if (iOutputTab) {
            _LIT8(KFormat, "%d\t");
            num.Format(KFormat, dinfo->d.si);
        }
        else {
            _LIT8(KFormat, "%d");
            num.Format(KFormat, dinfo->d.si);
        }
        iBufPos += num.Length();
        break;
    }
        
    case NBKDBG_UINT:
    {
        TPtr8 num((TUint8*)iBuffer + iBufPos, 0, space);
        if (iOutputTab) {
            _LIT8(KFormat, "%u\t");
            num.Format(KFormat, dinfo->d.ui);
        }
        else {
            _LIT8(KFormat, "%u");
            num.Format(KFormat, dinfo->d.ui);
        }
        iBufPos += num.Length();
        break;
    }
        
    case NBKDBG_WCHR:
    {
        TBool addSuffix = EFalse;
        if (space > PROBE_BUF_SIZE) {
            space = PROBE_BUF_SIZE - 3;
            addSuffix = ETrue;
        }
        Mem::Copy(iBuffer + iBufPos, u8, space - 1);
        NBK_free(u8);
        iBufPos += space - 1;
        if (addSuffix) {
            Mem::Copy(iBuffer + iBufPos, "...", 3);
            iBufPos += 3;
        }
        if (iOutputTab)
            iBuffer[iBufPos++] = '\t';
        break;
    }
        
    case NBKDBG_CHAR:
    {
        TBool addSuffix = EFalse;
        if (space > PROBE_BUF_SIZE) {
            space = PROBE_BUF_SIZE - 3;
            addSuffix = ETrue;
        }
        Mem::Copy(iBuffer + iBufPos, dinfo->d.cp, space - 1);
        iBufPos += space - 1;
        if (addSuffix) {
            Mem::Copy(iBuffer + iBufPos, "...", 3);
            iBufPos += 3;
        }
        if (iOutputTab)
            iBuffer[iBufPos++] = '\t';
        break;
    }
        
    case NBKDBG_RETURN:
        iBuffer[iBufPos++] = '\n';
        break;
        
    case NBKDBG_TIME:
        OutputTime();
        break;
        
    case NBKDBG_FLUSH:
        Flush();
        break;
        
    case NBKDBG_TAB:
        iOutputTab = (TBool)dinfo->d.on;
        break;
    }
    
#if FLUSH_IMMEDIATELY
    Flush();
#endif
}

void CProbe::Flush()
{
    RFs& rfs = CCoeEnv::Static()->FsSession();
    RFile file;
    
    if (iBufPos == 0)
        return;
    
    if (file.Create(rfs, KDbgFile, EFileWrite) == KErrNone)
        file.Close();
    
    if (file.Open(rfs, KDbgFile, EFileWrite) == KErrNone) {
        TPtrC8 dataP((TUint8*)iBuffer, iBufPos);
        TInt pos = 0;
        file.Seek(ESeekEnd, pos);
        file.Write(dataP);
        file.Flush();
        file.Close();
    }
    
    iBufPos = 0;
}

void CProbe::FileInit(const TDesC& aFileName, TBool aCreate)
{
    RFs& rfs = CCoeEnv::Static()->FsSession();
    RFile file;
    TInt err;
    
    if (BaflUtils::FileExists(rfs, aFileName))
        BaflUtils::DeleteFile(rfs, aFileName);
    else if (!aCreate)
        return;
    
    if (aCreate) {
        rfs.MkDirAll(aFileName);
    }
    
    err = file.Create(rfs, aFileName, EFileWrite);
    if (err != KErrNone)
        return;
    
    file.Close();
}

void CProbe::FileWrite(const TDesC& aFileName, const TDesC8& aData)
{
    RFs& rfs = CCoeEnv::Static()->FsSession();
    RFile file;
    TInt err;
    TInt pos = 0;
    
    err = file.Open(rfs, aFileName, EFileWrite);
    if (err != KErrNone)
        return;
    
    file.Seek(ESeekEnd, pos);
    file.Write(aData);
    file.Flush();
    file.Close();
}
