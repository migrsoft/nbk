/*
 ============================================================================
 Name		: NBrKernel.h
 Author	  : wuyulun
 Copyright   : 2010,2011 (C) MIC
 Description : NBrKernel.h - CNBrKernel class header
 ============================================================================
 */

#ifndef __NBRKERNEL_H__
#define __NBRKERNEL_H__

#include <e32base.h>
#include <e32std.h>
#include <es_sock.h>
#include <coemain.h>
#include <fbs.h>
#include <bitdev.h>
#include <bitstd.h>
#include <coedef.h>
#include <w32std.h>
#include <coecntrl.h>
#include "NBKPlatformData.h"

typedef struct _TNbkRect {
    TInt32   l;
    TInt32   t;
    TInt32   r;
    TInt32   b;
} TNbkRect;

class CNbkGdi;
class CProbe;
class CImageManager;
class CFEPHandler;
class CMonkeyTester;

const float KNbkDefaultZoomLevel = 1.0;
const float KNbkMaxZoomLevel = 5.0;

//////////////////////////////////////////////////
//
// 内核事件回调通知
//

class MNbrKernelInterface
{
public:
    
    // 请求重绘
    virtual void NbkRedraw() = 0;
    // 请求设置接入点
    virtual void NbkRequestAP() = 0;
    
    // 发出url请求，打开新文件
    virtual void OnNewDocument() = 0;
    // 打开历史页面
    virtual void OnHistoryDocument() = 0;
    // 通知当前处于请求等待状态
    virtual void OnWaiting() = 0;
    // 发出url请求，收到可用数据，该通知 仅发出一次
    virtual void OnNewDocumentBegin() = 0;
    // 接收到回应头信息
    virtual void OnGotResponse() = 0;
    // 接收到数据。全长未知时 aTotal = -1
    virtual void OnReceivedData(int aReceived, int aTotal) = 0;
    // 接收到图片
    virtual void OnReceivedImage(int aCurrent, int aTotal) = 0;
    // 视图更新，需要重绘
    virtual void OnViewUpdate(TBool aSizeChange = ETrue) = 0;
    // 页面解析完成（x, y, zoom 为上次访问该页面的最后阅读位置及缩放因子信息）
    virtual void OnLayoutFinish(int aX, int aY, float aZoom) = 0;
    
    // 通知附件下载
    virtual void OnDownload(const TDesC8& aUrl, const TDesC16& aFileName, const TDesC8& aCookie, TInt aSize) {}
    
    // 通知进入主体模式
    virtual void OnEnterMainBody() = 0;
    // 通知退出主体模式，需要重绘
    virtual void OnQuitMainBody(TBool aRefresh = ETrue) = 0;
    
    // 通知即将载入错误页
    virtual void OnLoadingErrorPage() {}
    
    // 文档重定位通知（文档坐标）
    virtual void OnRePosition(int aX, int aY, float aZoom) {}
    
    // 通知收到增量数据
    virtual void OnGotIncData() {}
    
    // 通知进行交互操作
    virtual void OnInterative() {}
    
    // 调用文件选择对话框选择文件
    virtual TBool OnSelectFile(TDesC& aOldFile, HBufC*& aSelectedFile) {}
    
    // 获取内核可显示区域（区域左上角为当前文档左上角位置，区域宽高为可显示大小（窗口大小））
    virtual void OnGetDocViewPort(TRect& aRect) = 0;
    
    // 获取内核显示区域在屏幕中的偏移量
    virtual void OnGetNBKOffset(TPoint& aPoint) = 0;   
    
    // 通知进入/退出编辑模式 @aEdit 是否编辑状态
    virtual void OnEditorStateChange(TBool& aEdit) = 0;
    
    // 接收到 Oauth2 授权回应
    virtual void OnOauth2Response(const TDesC8& aResponse) {}
    
    // 通知自由复制（按键操作），选择完成 
    virtual void OnTextselFinish() {}
};


//////////////////////////////////////////////////
//
// 内核控件绘制接口
//

class MNbkCtrlPaintInterface
{
public:
    enum TCtrlState {
        ENormal,
        EHighlight,
        EInEdit,
        EPressed,
    };
    
public:
    // 绘制单行输入框
    virtual TBool OnInputTextPaint(CFbsBitGc& aGc, const TRect& aRect, TCtrlState aState, const TRect& aClip) = 0;
    // 绘制复选钮
    virtual TBool OnCheckBoxPaint(CFbsBitGc& aGc, const TRect& aRect, TCtrlState aState, TBool aChecked, const TRect& aClip) = 0;
    // 绘制单选钮
    virtual TBool OnRadioPaint(CFbsBitGc& aGc, const TRect& aRect, TCtrlState aState, TBool aChecked, const TRect& aClip) = 0;
    // 绘制按钮
    virtual TBool OnButtonPaint(CFbsBitGc& aGc, const CFont* aFont, const TRect& aRect, const TDesC& aText, TCtrlState aState, const TRect& aClip) = 0;
    // 绘制列表（非展开状态）
    virtual TBool OnSelectNormalPaint(CFbsBitGc& aGc, const CFont* aFont, const TRect& aRect, const TDesC& aText, TCtrlState aState, const TRect& aClip) = 0;
    // 点击下拉列表框（展开状态），调用者返回选择的值，未选择时，aSel 为  -1
    virtual TBool OnSelectExpandPaint(const RArray<TPtrC>& aItems, TInt& aSel) = 0;
    // 绘制多行编辑框
    virtual TBool OnTextAreaPaint(CFbsBitGc& aGc, const TRect& aRect, TCtrlState aState, const TRect& aClip) = 0;
    // 绘制文件选择控件
    virtual TBool OnBrowsePaint(CFbsBitGc& aGc, const CFont* aFont, const TRect& aRect, const TDesC& aText, TCtrlState aState, const TRect& aClip) = 0;
    // 绘制折叠控制块背景
    virtual TBool OnFoldCtlPaint(CFbsBitGc& aGc, const TRect& aRect, TCtrlState aState, const TRect& aClip) = 0;
    
    // 绘制内部图片
    virtual void DrawStockImage(CFbsBitGc& aGc, const TDesC8& aResId, const TRect& aRect, TCtrlState aState, const TRect& aClip) {}
};


//////////////////////////////////////////////////
//
// 内核模态对话框实现接口
//

class MNbkDialogInterface
{
public:
    virtual void OnAlert(const TDesC& aMessage) = 0;
    virtual TBool OnConfirm(const TDesC& aMessage) = 0;
    virtual TBool OnPrompt(const TDesC& aMessage, HBufC*& aInputted) = 0;
};

//////////////////////////////////////////////////
//
// NBK 内核
//

class CNBrKernel : public CBase
{
    friend class CNbkGdi;
    friend class CMonkeyTester;

public:
    
    // 工作模式
    enum EWorkMode {
        EUnknown,
        EDirect,    // 直连
        EUck,       // TC转码
        EFffull,    // 云端全浏览
        EFfNarrow   // 云端简略浏览
    };

    // 文档类型
    enum EDocType {
        EDocUnknown,
        EDocMainBody,
        EDocWml,
        EDocXhtml,
        EDocHtml,
        EDocDxxml,
        EDocFull,
        EDocFullXhtml,
        EDocNarrow
    };

    // 字号级别
    enum EFontLevel {
        EFontSmall,
        EFontMiddle,
        EFontBig
    };
    
    // 结点类型
    enum ERenderKind {
        ERenderKindNone,
        ERenderKindText,
        ERenderKindHyperlink,
        ERenderKindImage,
        ERenderKindImageHyperlink,
        ERenderKindEditor,
        ERenderKindControl
    };

#ifdef __NBK_SELF_TESTING__
    
private:
    CMonkeyTester* iMonkeyTester;
    
    void ResetMonkey(void);

public:
    typedef enum _MonkeyType {
        eRandomEvent,
        eHitPageLinks
    } eMonkeyType;

    // 开始 Momkey test
    // aUrl: 指定开始测试页面地址
    // aRandom: 固定模式(页面内的每个连接载入一次至全部完成)或者随机模式(随机发送事件到指定次数完成)
    // aEventInterval: 随机模式下事件的间隔
    // aEventCnt: 随机模式下事件次数
    // momkey test 完成后发送通知
    IMPORT_C void StartMonkey(const TDesC8& aUrl, EWorkMode aMode, eMonkeyType aType,
        TInt aEventInterval, TInt aEventCnt, TUid aUid);
    
    //停止当前 Monkey test
    IMPORT_C void StopMonkey();
    
#endif
    
public:
    IMPORT_C static CNBrKernel* NewL(MNbrKernelInterface* interface);
    IMPORT_C static CNBrKernel* NewLC(MNbrKernelInterface* interface);
    IMPORT_C ~CNBrKernel();

    // 设置控件绘制接口
    IMPORT_C void SetControlPainter(MNbkCtrlPaintInterface* aPainter);
    
    // 设置模态对话框实现接口
    IMPORT_C void SetDialogInterface(MNbkDialogInterface* aInterface);
     
    //////////////////////////////////////////////////
    //
    // fep 实现
    //

    // 初始化 FEP 功能
    IMPORT_C void FepInit(CCoeControl* aObjectProvider);
    // 获取当前输入能力
    IMPORT_C TCoeInputCapabilities InputCapabilities() const;
    // 设置 FEP矩形大小偏移量。该偏移量为框架绘制皮肤效果用
    IMPORT_C void SetFepShrinkSize(TSize& aSize) { iFepShrinkSize = aSize; }
    
    IMPORT_C void PasteText(const TDesC& aText);

public:
    // 版本信息
    IMPORT_C TVersion Version() const;
    
    // 设置是否由内核处理所有操作
    IMPORT_C void SetAutoMode(TBool aAuto = ETrue) { iAuto = aAuto; }
    
    // 设置内核工作模式
    IMPORT_C void SetWorkMode(EWorkMode aMode);
    // 获取内核工作模式
    IMPORT_C EWorkMode GetWorkMode() const;
    // 设置中转浏览工作模式
    IMPORT_C void SetWorkModeForTC(EWorkMode aMode);
    
    // 获取操作模式
    // ETrue: 4向移动模庿  EFalse: 上下移动模式
    IMPORT_C TBool Is4Ways() const;
    
    // 设置缓图位图
    IMPORT_C void SetScreen(CFbsBitmap* aScreen, TBool aFixedScreen = EFalse);
    // 获取页面宽度
    IMPORT_C TInt GetWidth() const;
    // 获取页面高度
    IMPORT_C TInt GetHeight() const;
    
    // 获取绘制水平偏移
    IMPORT_C TInt GetXOffset() const { return iViewXOffset; }
    
    // 设置项
    // 开关图片显示
    IMPORT_C void SetAllowImage(TBool aEnable);
    // 获取当前图片是否显示
    IMPORT_C TBool AllowImage() const;

    // 开关自适应布局
    IMPORT_C void SetSelfAdaptionLayout(TBool aEnable);

    // 设置字体大小，像素单位
    IMPORT_C void SetFontSize(TInt aPixel, TInt aLevel);
    // 获取字体大小，返回为0时为默认值
    IMPORT_C TInt GetFontSize() const;
    // 设置字体抗锯齿
    IMPORT_C void SetFontAntiAliasing(TBool aAnti);
    // 字体是否抗锯齿
    IMPORT_C TBool IsFontAntiAliasing(void);
    
    //////////////////////////////////////////////////
    //
    // 页面基本操作
    //

    // 载入页面数据
    IMPORT_C void LoadData(const TDesC8& aUrl, const HBufC8* aData);
    // 载入 URL
    IMPORT_C void LoadUrl(const TDesC8& aUrl);
    // 停止下载
    IMPORT_C void Stop();
    
    // 暂停页面活动项
    IMPORT_C void Pause();
    // 恢复页面活动项
    IMPORT_C void Resume();
    
    // 刷新页面
    IMPORT_C void Refresh();
    
    // 设置绘制参数
    // aMaxTime: 最大绘制时间（单位1/1000秒），设为 -1 代表不限制
    // aDrawPic: 是否绘制前景图片
    // aDrawBgimg: 是否绘制背景图片
    IMPORT_C void SetDrawParams(int aMaxTime, TBool aDrawPic, TBool aDrawBgimg);
    // 绘制指定区域（文档坐标）
    IMPORT_C void DrawPage(const TRect& aRect);
    // 绘制局部区域（文档坐标） aOffset（屏幕坐标）
    IMPORT_C void DrawPage(const TRect& aRect, const TPoint& aOffset);

    // Fep / 控件按键处理
    // 返回 EKeyWasNotConsumed 需继续处理
    IMPORT_C TKeyResponse HandleKeyByControlL(const TKeyEvent &aKeyEvent, TEventCode aType);
    
    // 按键入口
    IMPORT_C TKeyResponse OfferKeyEventL(const TKeyEvent &aKeyEvent, TEventCode aType);
    
    IMPORT_C TBool HandlePointerByControlL(const TPointerEvent& aPointerEvent);
    // 点击事件接口
    IMPORT_C void HandlePointerEventL(const TPointerEvent& aPointerEvent);

    // 设置接入点
    IMPORT_C TInt SetIap(TUint32 aIapId, TBool aIsWap);
    
    // 设置外部 Connection
    IMPORT_C void SetAccessPoint(
        RConnection& aConnect, RSocketServ& aSocket,
        TUint32 aAccessPoint, const TDesC& aAcessName, const TDesC& aApnName,TBool aReady = ETrue);

    //////////////////////////////////////////////////
    //
    // 页面信息接口
    //

    // 获取文档 URL
    IMPORT_C const TPtrC8 GetUrl() const;
    
    // 获取文档标题
    IMPORT_C const TPtrC GetTitle() const;

    // 获取文档类型
    IMPORT_C const EDocType GetType() const;

    //////////////////////////////////////////////////
    //
    // 焦点功能接口
    //
    
    // 获取指定区域内的焦点，无焦点时，返回 NULL
    IMPORT_C void* GetFocus(const TRect& aRect);
    
    // 获取上次设置的焦点
    IMPORT_C void* GetFocus();
    
    // 获取指定坐标下的焦点，无焦点时，返回 NULL
    // aPos 为文档坐标
    IMPORT_C void* GetFocusByPos(const TPoint& aPos);
    
    // 获取焦点URL，调用者负责释放内存。无URL时，返回NULL
    IMPORT_C HBufC8* GetFocusUrl(void* focus);
    
    // 获取以指定焦点为基准的下一个焦点
    IMPORT_C void* GetNextFocus(void* focus);
    
    // 获取以指定焦点为基准的上一个焦点
    IMPORT_C void* GetPrevFocus(void* focus);
    
    // 获取指定焦点的区域信息
    IMPORT_C void GetFocusRect(void* focus, TRect& aRect);
    
    // 设置当前焦点
    IMPORT_C void SetFocus(void* focus);
    
    //设置焦点结点状态
    IMPORT_C void SetCtrlNodeState(void* aFocusNode,MNbkCtrlPaintInterface::TCtrlState aState);

    // 激活焦点
    IMPORT_C void ClickFocus(void* focus);
    
    // 获取指定坐标下渲染结点的类型
    IMPORT_C ERenderKind GetRenderKindByPos(const TPoint& aPos);
    
    // 获取指定坐标下图片的原始URL
    IMPORT_C TPtrC8 GetImageSrcByPos(const TPoint& aPos);
    
    //////////////////////////////////////////////////
    //
    // 页面缩放功能接口
    //
    
    // 以指定固子缩放页面 。例  1.0 为 100%， 0.25 为 25%
    IMPORT_C void ZoomPage(float aZoomFactor);
    // 获取当前缩放因子
    IMPORT_C float GetCurrentZoomFactor(void);
    // 设置当前缩放因子
    IMPORT_C void SetCurrentZoomFactor(float aZoom);
    // 获取最小缩放因子。在接收到OnViewUpdate()回调后即可获取
    IMPORT_C float GetMinZoomFactor(void);

    //////////////////////////////////////////////////
    //
    // 历史功能接口
    //
    
    IMPORT_C TBool Back();
    IMPORT_C TBool Forward();
    
    // 获取历史列表信息
    // aCurrent:    当前页面在历史列表中的位置
    // aAll:        历史列表中的所有项目数
    // 注：   0 <= aCurrent <= aAll
    // 值为 0 时，表示当前为空页面
    IMPORT_C void GetHistoryRange(TInt& aCurrent, TInt& aAll);
    
    // 显示指定页， aCurrent <= aWhere <= aAll
    IMPORT_C TBool GotoHistoryPage(TInt aWhere);

    // 清除cookie
    IMPORT_C void ClearCookies();
    
    //////////////////////////////////////////////////
    //
    // 其它功能接口
    //

    // 排版调用不产生重绘通知 ，由调用者确定是否重绘
    // 调用页面排版
    IMPORT_C void Layout(TBool bForce = EFalse);
    
    // 设置夜间模式的颜色
    IMPORT_C void SetNightTheme(TRgb aText, TRgb aLink, TRgb aBg);
    // 使用夜间模式
    IMPORT_C void UseNightTheme(TBool aUse = ETrue);
    
    // 开始自由复制
    IMPORT_C void CopyModeBegin();
    // 结束自由复制
    IMPORT_C void CopyModeEnd();
    // 设置选择起点位置（触屏操作）
    IMPORT_C void SetCopyStartPoint(const TPoint& aPoint);
    // 设置选择起点位置（按键操作）
    IMPORT_C void SelectTextByKey(const TPoint& aPoint);
    // 获取复制的文本
    IMPORT_C HBufC* GetSelectedText();
    
public: // 仅供调试
    CProbe* iProbe;

    IMPORT_C void Dump(char* string, int length);
    
public:
    static int OnNbkEvent(int id, void* user, void* info);
    void HandleNbkEvent();
    void RequestAP();
    void Download(const TDesC8& aUrl, const TDesC16& aFileName, const TDesC8& aCookie, TInt aSize);
    TBool SelectFile(TDesC& aOldFile, HBufC*& aSelectedFile);

public:
    
    void GetViewPort(void* aRect);
    void GetNBKOffset(void* aPoint);
    void* GetPage(void);
    void OnEditStateChange(TBool aEdit);
    MNbrKernelInterface* GetShellInterface() { return iShellInterface; }
    
    CFbsBitGc& GetScreenGc() { return *iScreenGc; }
    TSize GetFepShrinSize() const { return iFepShrinkSize; }
    
    TBool IsNightTheme(void);
    void* GetNightTheme(void) { return iNightTheme; }
    
    TDisplayMode GetDisplayMode();
    
    float GetCurZoom(); // 获取实际值
    void  SetCurZoom(float aZoom);

    MNbkCtrlPaintInterface::TCtrlState CurNodeState(void) { return iNodeState; }
    
    void DenyEvent(TBool aDeny) { iDenyEvent = aDeny; }
    
private:
    CNBrKernel(MNbrKernelInterface* interface);
    void ConstructL();
    
private:
    TBool iAuto; // 是否由内部处理所有常用操作
    TBool i4Ways; // 四向操作模式
    
    CFbsBitmap* iScreen;
    CFbsBitmapDevice* iScreenDev; // owned
    CFbsBitGc* iScreenGc; // owned
    
    int iViewXOffset; // 云简排，当页面宽度小于屏幕时，居中显示时，x偏移量
    TPoint iDrawOffset; //局部刷新偏移量
    MNbrKernelInterface* iShellInterface;
    MNbkDialogInterface* iDialogInterface;
    
    void* iHistory;
    NBK_PlatformData iPlatform;
    CImageManager* iImageManager;
    CNbkGdi* iGdi;
    
    TNbkRect iViewRect; // 当前文档的观察区域，文档坐标
    TPoint iLastPos; // 最近一次文档观察区域的左上角坐标，用于恢复位置
    TPoint iLastPosBak; // iLastPos 暂存
    
    // 光标位图
    TPoint iArrowPos;
    
    CFbsBitmap* iBmpArrow;
    CFbsBitmap* iBmpArrowMask;
    
    // FEP 实现
    CCoeControl* iObjectProvider;
    CFEPHandler* iFepHandler;
    TBool iEditMode;
    TSize iFepShrinkSize;
    
    void UpdateViewRect();
    void AdjustViewRect(TNbkRect* rect, TBool aDirDown = ETrue);
    void ChangeZoomLevel(float aFactor);
    void CalcMinZoomFactor();
    
    TKeyResponse HandleKey(const TKeyEvent &aKeyEvent, TEventCode aType);
    TKeyResponse HandleKey4Ways(const TKeyEvent &aKeyEvent, TEventCode aType);
    
    void PageUp();
    void PageDown();
    void FocusNext();
    void FocusPrev();
    void FocusClick();
    void MoveLeft();
    void MoveRight();
    void MoveUp();
    void MoveDown();
    void CheckFocus();
    
    void MoveScreenLeft(float ratio);
    void MoveScreenRight(float ratio);
    void MoveScreenUp(float ratio);
    void MoveScreenDown(float ratio);
    
    void DragByPen(TPoint aMove);
    
    void RePos(int aX, int aY);
    
    void Clear();
    void SetPageWidth();
    
    void DrawScreen();
    void UpdateScreen();
    void RefreshScreen();
    void DrawArrow();
    
private:
    
    typedef struct _TNbkEvent {
        
        int eventId;
        
        int imgCur;
        int imgTotal;
        int datReceived;
        int datTotal;
        
        int x;
        int y;
        
        float   zoom;
        
        int     pageId;
        void*   func;
        void*   user;
        
    } TNbkEvent;

    int iEventId;

    // 循环事件队列
    TNbkEvent*  iEvtQueue;
    int         iEvtAdd;
    int         iEvtUsed;
    
    int         iCounter;
    CPeriodic*  iTimer;
    TInt8       iInfoFont;
    TBuf<32>    iMessage;
    
    RPointerArray<HBufC> m_shortcutTextLst;

    static TInt OnTimer(TAny* ptr);
    void DrawCounter();
    void DrawText(const TDesC& aText);
    
    // 缩放因子值区间:[页面宽度适屏,1.0]
    float   ifCurZoomFactor;
    float   ifMinZoomfactor;
    
    float   iSavedCurZoom;
    float   iSavedMinZoom;
    
    TPoint  iLastPenPos;
    void*   iPenFocus;
    
    MNbkCtrlPaintInterface::TCtrlState iNodeState;
    
    TBool iDenyEvent; // 禁止事件通知接口，用于避免不正确 的模态对话框实现带来的无效事件
    void* iPageSettings;
    void* iNightTheme;
    
    // 双击检测
    TInt    iDbcInterval; // 通过时间差检测双击
    
    TBool   iTouchScreen; // 是否触屏
    TBool   iUseMinZoom; // 是否适屏
    TBool   iEditPaused;
    
    // 复制功能
    void*   iTextSel;
};

#endif  // __NBRKERNEL_H__
