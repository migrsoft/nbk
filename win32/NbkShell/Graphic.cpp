#include "stdafx.h"
#include "Graphic.h"
#include "NbkCore.h"
#include "NbkShell.h"
#include "../../stdc/inc/nbk_gdi.h"
#include "../../stdc/inc/nbk_ctlPainter.h"

////////////////////////////////////////////////////////////////////////////////
//
// NBK 调用
//

void NBK_gdi_drawText(void* pfd, const wchr* text, int length, const NPoint* pos, int decor)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetGraphic()->DrawText(text, length, pos, decor);
}

void NBK_gdi_drawRect(void* pfd, const NRect* rect)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetGraphic()->DrawRect(rect);
}

void NBK_gdi_drawEllipse(void* pfd, const NRect* rect)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetGraphic()->DrawEllipse(rect);
}

void NBK_gdi_fillRect(void* pfd, const NRect* rect)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetGraphic()->FillRect(rect);
}

NFontId NBK_gdi_getFont(void* pfd, int16 pixel, nbool bold, nbool italic)
{
	NbkCore* core = (NbkCore*)pfd;
	return core->GetGraphic()->GetFont(pixel, bold, italic);
}

void NBK_gdi_useFont(void* pfd, NFontId id)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetGraphic()->UseFont(id);
}

void NBK_gdi_useFontNoZoom(void* pfd, NFontId id)
{
}

void NBK_gdi_releaseFont(void* pfd)
{
}

coord NBK_gdi_getFontHeight(void* pfd, NFontId id)
{
	NbkCore* core = (NbkCore*)pfd;
	return core->GetGraphic()->GetFontHeight(id);
}

coord NBK_gdi_getCharWidth(void* pfd, NFontId id, const wchr ch)
{
	NbkCore* core = (NbkCore*)pfd;
	return core->GetGraphic()->GetCharWidth(id, ch);
}

coord NBK_gdi_getTextWidth(void* pfd, NFontId id, const wchr* text, int length)
{
	NbkCore* core = (NbkCore*)pfd;
	return core->GetGraphic()->GetTextWidth(id, text, length);
}

void NBK_gdi_setPenColor(void* pfd, const NColor* color)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetGraphic()->SetPenColor(color);
}

void NBK_gdi_setBrushColor(void* pfd, const NColor* color)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetGraphic()->SetBrushColor(color);
}

void NBK_gdi_setClippingRect(void* pfd, const NRect* rect)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetGraphic()->SetClip(rect);
}

void NBK_gdi_cancelClippingRect(void* pfd)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetGraphic()->CancelClip();
}

// for editor, work in screen coordinate
coord NBK_gdi_getCharWidthByEditor(void* pfd, NFontId id, const wchr ch)
{
	return NBK_gdi_getCharWidth(pfd, id, ch);
}

coord NBK_gdi_getFontHeightByEditor(void* pfd, NFontId id)
{
	return NBK_gdi_getFontHeight(pfd, id);
}

coord NBK_gdi_getTextWidthByEditor(void* pfd, NFontId id, const wchr* text, int length)
{
	return NBK_gdi_getTextWidth(pfd, id, text, length);
}

void NBK_gdi_drawEditorCursor(void* pfd, NPoint* pt, coord xOffset, coord cursorHeight)
{
}

void NBK_gdi_drawEditorScrollbar(void* pfd, NPoint* pt, coord xOffset, coord yOffset, NSize* size)
{
}

void NBK_gdi_drawEditorText(void* pfd, const wchr* text, int length, const NPoint* pos, coord xOffset)
{
}


// -----------------------------------------------------------------------------
// 表单控件绘制接口
// -----------------------------------------------------------------------------

// 绘制单行输入框
nbool NBK_paintText(void* pfd, const NRect* rect, NECtrlState state, const NRect* clip)
{
	return N_FALSE;
}

// 绘制复选钮
nbool NBK_paintCheckbox(void* pfd, const NRect* rect, NECtrlState state, nbool checked, const NRect* clip)
{
	return N_FALSE;
}

// 绘制单选钮
nbool NBK_paintRadio(void* pfd, const NRect* rect, NECtrlState state, nbool checked, const NRect* clip)
{
	return N_FALSE;
}

// 绘制按钮
nbool NBK_paintButton(void* pfd, const NFontId fontId, const NRect* rect,
                     const wchr* text, int len, NECtrlState state, const NRect* clip)
{
	return N_FALSE;
}

// 绘制列表正常状态
nbool NBK_paintSelectNormal(void* pfd, const NFontId fontId, const NRect* rect,
                           const wchr* text, int len, NECtrlState state, const NRect* clip)
{
	return N_FALSE;
}

// 绘制列表展开状态
nbool NBK_paintSelectExpand(void* pfd, const wchr* items, int num, int* sel)
{
	return N_FALSE;
}

// 绘制多行输入框
nbool NBK_paintTextarea(void* pfd, const NRect* rect, NECtrlState state, const NRect* clip)
{
	return N_FALSE;
}

// 绘制文件选择控件
nbool NBK_paintBrowse(void* pfd, const NFontId fontId, const NRect* rect,
                     const wchr* text, int len, NECtrlState state, const NRect* clip)
{
	return N_FALSE;
}

// 折叠块背景绘制接口
nbool NBK_paintFoldBackground(void* pfd, const NRect* rect, NECtrlState state, const NRect* clip)
{
	return N_FALSE;
}

// -----------------------------------------------------------------------------
// 平台层编辑器功能实现接口
// 编辑状态时，会多次调用该接口
// -----------------------------------------------------------------------------

// 单行编辑
nbool NBK_editInput(void* pfd, NFontId fontId, NRect* rect, const wchr* text, int len,int maxlen,nbool password)
{
	return N_FALSE;
}

// 多行编辑
nbool NBK_editTextarea(void* pfd, NFontId fontId, NRect* rect, const wchr* text, int len,int maxlen)
{
	return N_FALSE;
}

// -----------------------------------------------------------------------------
// 模态对话框接口
// -----------------------------------------------------------------------------

// open a file select dialog by framework
// 选择本地文件
// oldFile: 上次选择的文件路径
// newFile: 本次选择的文件路径
nbool NBK_browseFile(void* pfd, const char* oldFile, char** newFile)
{
	return N_FALSE;
}

// 提示对话框（仅含确定按钮）
nbool NBK_dlgAlert(void* pfd, const wchr* text, int len)
{
	return N_FALSE;
}

// 验证对话框（含确定、取消按钮）
nbool NBK_dlgConfirm(void* pfd, const wchr* text, int len, int* ret)
{
	return N_FALSE;
}

// 输入对话框（含单行输入控件，确定、取消按钮）
nbool NBK_dlgPrompt(void* pfd, const wchr* text, int len, int* ret, char** input)
{
	return N_FALSE;
}

// -----------------------------------------------------------------------------
// 资源接口
// -----------------------------------------------------------------------------

// 绘制程序内置图片
void NBK_drawStockImage(void* pfd, const char* res, const NRect* rect, NECtrlState state, const NRect* clip)
{
}

////////////////////////////////////////////////////////////////////////////////
//
// FontW32 字体定义
//

FontW32::FontW32(Graphics* g, int pixel, nbool bold, nbool italic)
{
	mPixel = pixel;
	mBold = bold;
	mItalic = italic;

	FontFamily fontFamily(L"\u65b0\u5b8b\u4f53"); // 新宋体
	int style = FontStyleRegular;
	if (bold)
		style |= FontStyleBold;
	if (italic)
		style |= FontStyleItalic;
	mFont = new Font(&fontFamily, (REAL)pixel, style, UnitPixel);

	mAscent = mFont->GetSize() * fontFamily.GetCellAscent(style) / fontFamily.GetEmHeight(style);

	WCHAR hz[] = L"\u6c49";
	PointF origin(0.0f, 0.0f);
	RectF bound;
	g->MeasureDriverString((UINT16*)hz, 1, mFont, &origin, DriverStringOptionsCmapLookup, NULL, &bound);
	mHeight = (int)floor(bound.Height + 0.5f);
	mHzWidth = (int)floor(bound.Width + 0.5f);
}

FontW32::~FontW32()
{
	if (mFont)
		delete mFont;
}

bool FontW32::Equals(int pixel, nbool bold, nbool italic)
{
	return (mPixel == pixel && mBold == bold && mItalic == italic);
}

int FontW32::GetTextWidth(Graphics* g, const wchr* text, int length)
{
	if (length == 1 && ((text[0] >= 0x3400 && text[0] <= 0x9fff) || (text[0] >= 0xf900 && text[0] <= 0xfaff))) {
		return mHzWidth;
	}

	PointF origin(0.0f, 0.0f);
	RectF bound;
	g->MeasureDriverString(text, length, mFont, &origin,
		DriverStringOptionsCmapLookup | DriverStringOptionsRealizedAdvance, NULL, &bound);
	return (int)floor(bound.Width + 0.5f);
}

////////////////////////////////////////////////////////////////////////////////
//
// GraphicsW32 图形层
//

GraphicsW32::GraphicsW32()
{
	mShell = NULL;
	memset(mFonts, 0, sizeof(FontW32*) * MAX_FONTS);
	mCurFont = -1;

	color_set(&mPenColor, 0, 0, 0, 255);
	color_set(&mBrushColor, 255, 255, 255, 255);

	mPen = new Pen(Color(255, 0, 0, 0));
	mBrush = new SolidBrush(Color(255, 255, 255, 255));
	mTextBrush = new SolidBrush(Color(255, 0, 0, 0));
}

GraphicsW32::~GraphicsW32()
{
	delete mPen;
	delete mBrush;
	delete mTextBrush;

	Reset();
}

void GraphicsW32::Reset()
{
	for (int i=0; i < MAX_FONTS; i++) {
		if (mFonts[i]) {
			delete mFonts[i];
			mFonts[i] = NULL;
		}
	}
}

NFontId GraphicsW32::GetFont(int16 pixel, nbool bold, nbool italic)
{
	int i;
	for (i = 0; i < MAX_FONTS && mFonts[i]; i++) {
		FontW32* f = mFonts[i];
		if (f->Equals(pixel, bold, italic))
			return i;
	}
		
	if (i == MAX_FONTS)
		return -1;
	
	mFonts[i] = new FontW32(mShell->GetGdi(), pixel, bold, italic);
	if (mFonts[i]->GetFont() == NULL) {
		delete mFonts[i];
		mFonts[i] = NULL;
		return -1;
	}

	return (NFontId)i;
}

bool GraphicsW32::IsExist(int id)
{
	return (id >= 0 && id < MAX_FONTS && mFonts[id]);
}

coord GraphicsW32::GetFontHeight(NFontId id)
{
	int h = 0;

	if (IsExist(id))
		h = mFonts[id]->GetHeight();

	return (coord)h;
}

coord GraphicsW32::GetCharWidth(NFontId id, const wchr ch)
{
	int w = 0;

	if (IsExist(id))
		w = mFonts[id]->GetTextWidth(mShell->GetGdi(), &ch, 1);

	return (coord)w;
}

coord GraphicsW32::GetTextWidth(NFontId id, const wchr* text, int length)
{
	int w = 0;

	if (IsExist(id))
		w = mFonts[id]->GetTextWidth(mShell->GetGdi(), text, length);

	return (coord)w;
}

void GraphicsW32::UseFont(NFontId id)
{
	mCurFont = id;
}

void GraphicsW32::SetPenColor(const NColor* c)
{
	if (!color_equals(&mPenColor, c)) {
		mPenColor = *c;
		delete mPen;
		mPen = new Pen(Color(mPenColor.a, mPenColor.r, mPenColor.g, mPenColor.b));
		delete mTextBrush;
		mTextBrush = new SolidBrush(Color(mPenColor.a, mPenColor.r, mPenColor.g, mPenColor.b));
	}
}

void GraphicsW32::SetBrushColor(const NColor* c)
{
	if (!color_equals(&mBrushColor, c)) {
		mBrushColor = *c;
		delete mBrush;
		mBrush = new SolidBrush(Color(mBrushColor.a, mBrushColor.r, mBrushColor.g, mBrushColor.b));
	}
}

void GraphicsW32::SetClip(const NRect* c)
{
	Rect clip(c->l, c->t, rect_getWidth(c), rect_getHeight(c));
	Graphics* g = mShell->GetGdi();
	g->SetClip(clip);
}

void GraphicsW32::CancelClip()
{
	Graphics* g = mShell->GetGdi();
	g->ResetClip();
}

void GraphicsW32::DrawText(const wchr* text, int length, const NPoint* pos, int decor)
{
	if (IsExist(mCurFont)) {
		Graphics* g = mShell->GetGdi();
		FontW32* f = mFonts[mCurFont];
		PointF origin((REAL)pos->x, (REAL)pos->y + f->GetAscent());
		g->DrawDriverString(text, length, f->GetFont(), mTextBrush, &origin,
			DriverStringOptionsCmapLookup | DriverStringOptionsRealizedAdvance, NULL);
	}
}

void GraphicsW32::DrawRect(const NRect* r)
{
	Graphics* g = mShell->GetGdi();
	g->DrawRectangle(mPen, r->l, r->t, rect_getWidth(r) - 1, rect_getHeight(r) - 1);
}

void GraphicsW32::FillRect(const NRect* r)
{
	Graphics* g = mShell->GetGdi();
	g->FillRectangle(mBrush, r->l, r->t, rect_getWidth(r) - 1, rect_getHeight(r) - 1);
}

void GraphicsW32::DrawEllipse(const NRect* r)
{
	Graphics* g = mShell->GetGdi();
	g->DrawEllipse(mPen, r->l, r->t, rect_getWidth(r) - 1, rect_getHeight(r) - 1);
}

void GraphicsW32::DrawImage(Image* image, int x, int y, int w, int h)
{
	Graphics* g = mShell->GetGdi();
	g->DrawImage(image, x, y, w, h);
}
