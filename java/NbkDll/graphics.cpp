#include "graphics.h"
#include "NbkDll.h"
#include "com_migrsoft_nbk_Graphic.h"
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
	core->GetGraphic()->DrawOval(rect);
}

void NBK_gdi_fillRect(void* pfd, const NRect* rect)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetGraphic()->ClearRect(rect);
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
	core->GetGraphic()->SetColor(color);
}

void NBK_gdi_setBrushColor(void* pfd, const NColor* color)
{
	NbkCore* core = (NbkCore*)pfd;
	core->GetGraphic()->SetBgColor(color);
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
// Java 调用
//

JNIEXPORT void JNICALL Java_com_migrsoft_nbk_Graphic_register
  (JNIEnv * env, jobject obj)
{
	NbkCore* core = NbkCore::GetInstance();
	core->GetGraphic()->RegisterJavaMethods(env, obj);
}

////////////////////////////////////////////////////////////////////////////////
//
// Graphic 接口类
//

Graphic::Graphic()
{
	OBJ_graphic = NULL;
}

Graphic::~Graphic()
{
	if (OBJ_graphic != NULL) {
		JNI_env->DeleteGlobalRef(OBJ_graphic);
		OBJ_graphic = NULL;
	}
}

void Graphic::RegisterJavaMethods(JNIEnv* env, jobject obj)
{
	JNI_env = env;
	OBJ_graphic = env->NewGlobalRef(obj);

	jclass clazz = env->GetObjectClass(obj);

	MID_reset = env->GetMethodID(clazz, "reset", "()V");

	MID_getFont = env->GetMethodID(clazz, "getFont", "(IZZ)I");
	MID_getFontHeight = env->GetMethodID(clazz, "getFontHeight", "(I)I");
	MID_getTextWidth = env->GetMethodID(clazz, "getTextWidth", "(ILjava/lang/String;)I");
	MID_useFont = env->GetMethodID(clazz, "useFont", "(I)V");

	MID_setColor = env->GetMethodID(clazz, "setColor", "(IIII)V");
	MID_setBgColor = env->GetMethodID(clazz, "setBgColor", "(IIII)V");

	MID_setClip = env->GetMethodID(clazz, "setClip", "(IIII)V");
	MID_cancelClip = env->GetMethodID(clazz, "cancelClip", "()V");

	MID_drawText = env->GetMethodID(clazz, "drawText", "(Ljava/lang/String;III)V");
	MID_drawRect = env->GetMethodID(clazz, "drawRect", "(IIII)V");
	MID_fillRect = env->GetMethodID(clazz, "fillRect", "(IIII)V");
	MID_drawOval = env->GetMethodID(clazz, "drawOval", "(IIII)V");
}

void Graphic::Reset()
{
	JNI_env->CallVoidMethod(OBJ_graphic, MID_reset);
}

NFontId Graphic::GetFont(int16 pixel, nbool bold, nbool italic)
{
	int fid = JNI_env->CallIntMethod(OBJ_graphic, MID_getFont, (int)pixel, (jboolean)bold, (jboolean)italic);
	return (NFontId)fid;
}

coord Graphic::GetFontHeight(NFontId id)
{
	int h = JNI_env->CallIntMethod(OBJ_graphic, MID_getFontHeight, (int)id);
	return h;
}

coord Graphic::GetCharWidth(NFontId id, const wchr ch)
{
	jstring str = JNI_env->NewString(&ch, 1);
	int w = JNI_env->CallIntMethod(OBJ_graphic, MID_getTextWidth, (int)id, str);
	JNI_env->DeleteLocalRef(str);
	return w;
}

coord Graphic::GetTextWidth(NFontId id, const wchr* text, int length)
{
	jstring str = JNI_env->NewString(text, length);
	int w = JNI_env->CallIntMethod(OBJ_graphic, MID_getTextWidth, (int)id, str);
	JNI_env->DeleteLocalRef(str);
	return w;
}

void Graphic::UseFont(NFontId id)
{
	JNI_env->CallVoidMethod(OBJ_graphic, MID_useFont, (int)id);
}

void Graphic::SetColor(const NColor* c)
{
	mColor = *c;
	JNI_env->CallVoidMethod(OBJ_graphic, MID_setColor, (int)c->r, (int)c->g, (int)c->b, (int)c->a);
}

void Graphic::SetBgColor(const NColor* c)
{
	mBgColor = *c;
	JNI_env->CallVoidMethod(OBJ_graphic, MID_setBgColor, (int)c->r, (int)c->g, (int)c->b, (int)c->a);
}

void Graphic::SetClip(const NRect* c)
{
	JNI_env->CallVoidMethod(OBJ_graphic, MID_setClip, (int)c->l, (int)c->t, (int)rect_getWidth(c), (int)rect_getHeight(c));
}

void Graphic::CancelClip()
{
	JNI_env->CallVoidMethod(OBJ_graphic, MID_cancelClip);
}

void Graphic::DrawText(const wchr* text, int length, const NPoint* pos, int decor)
{
	jstring str = JNI_env->NewString(text, length);
	JNI_env->CallVoidMethod(OBJ_graphic, MID_drawText, str, (int)pos->x, (int)pos->y, decor);
	JNI_env->DeleteLocalRef(str);
}

void Graphic::DrawRect(const NRect* rect)
{
	JNI_env->CallVoidMethod(OBJ_graphic, MID_drawRect, (int)rect->l, (int)rect->t, (int)rect_getWidth(rect), (int)rect_getHeight(rect));
}

void Graphic::ClearRect(const NRect* rect)
{
	NColor old = mColor;
	SetColor(&mBgColor);
	JNI_env->CallVoidMethod(OBJ_graphic, MID_fillRect, (int)rect->l, (int)rect->t, (int)rect_getWidth(rect), (int)rect_getHeight(rect));
	SetColor(&old);
}

void Graphic::DrawOval(const NRect* r)
{
	JNI_env->CallVoidMethod(OBJ_graphic, MID_drawOval, (int)r->l, (int)r->t, (int)rect_getWidth(r), (int)rect_getHeight(r));
}
