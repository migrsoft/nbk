#include "ImageMgr.h"
#include "NbkDll.h"
#include "com_migrsoft_nbk_ImageMgr.h"

////////////////////////////////////////////////////////////////////////////////
//
// NBK 图像资源请求
//

void NBK_imageCreate(void* pfd, NImagePlayer* player, NImage* image)
{
	NbkCore::GetInstance()->GetImageMgr()->Create(image, player);
}

void NBK_imageDelete(void* pfd, NImagePlayer* player, NImage* image)
{
	NbkCore::GetInstance()->GetImageMgr()->Delete(image);
}

void NBK_imageDecode(void* pfd, NImagePlayer* player, NImage* image)
{
	NbkCore::GetInstance()->GetImageMgr()->Decode(image);
}

void NBK_imageDraw(void* pfd, NImagePlayer* player, NImage* image, const NRect* dest)
{
	NbkCore::GetInstance()->GetImageMgr()->Draw(image, dest);
}

void NBK_imageDrawBg(void* pfd, NImagePlayer* player, NImage* image, const NRect* dest, int rx, int ry)
{
	NbkCore::GetInstance()->GetImageMgr()->DrawBg(image, dest, rx, ry);
}

void NBK_onImageData(void* pfd, NImage* image, void* data, int length)
{
	NbkCore::GetInstance()->GetImageMgr()->OnData(image, data, length);
}

void NBK_onImageDataEnd(void* pfd, NImage* image, nbool succeed)
{
	NbkCore::GetInstance()->GetImageMgr()->OnDataEnd(image, succeed);
}

void NBK_imageStopAll(void* pfd)
{
	NbkCore::GetInstance()->GetImageMgr()->StopAll();
}

nbool NBK_isImageCached(void* pfd, const char* url, nbool history)
{
	return N_FALSE;
}

////////////////////////////////////////////////////////////////////////////////
//
// Java 调用
//

JNIEXPORT void JNICALL Java_com_migrsoft_nbk_ImageMgr_register
  (JNIEnv * env, jobject obj)
{
	NbkCore::GetInstance()->GetImageMgr()->RegisterMethods(env, obj);
}

JNIEXPORT void JNICALL Java_com_migrsoft_nbk_ImageMgr_nbkImageDecodeOver
  (JNIEnv *, jobject, jint id, jboolean ok, jint w, jint h)
{
	NbkCore::GetInstance()->GetImageMgr()->OnDecodeOver(id, ok, w, h);
}

////////////////////////////////////////////////////////////////////////////////
//
// ImageMgr 接口类
//

ImageMgr::ImageMgr()
{
	OBJ_ImageMgr = NULL;

	NBK_memset(&mImages, 0, sizeof(NImage*) * MAX_IMAGES);
}

ImageMgr::~ImageMgr()
{
	if (OBJ_ImageMgr) {
		JNI_env->DeleteGlobalRef(OBJ_ImageMgr);
		OBJ_ImageMgr = NULL;
	}
}

void ImageMgr::RegisterMethods(JNIEnv* env, jobject obj)
{
	JNI_env = env;
	OBJ_ImageMgr = env->NewGlobalRef(obj);

	jclass clazz = env->GetObjectClass(obj);
	MID_createImage = env->GetMethodID(clazz, "createImage", "(I)V");
	MID_deleteImage = env->GetMethodID(clazz, "deleteImage", "(I)V");
	MID_onData = env->GetMethodID(clazz, "onImageData", "(I[B)V");
	MID_onDataEnd = env->GetMethodID(clazz, "onImageEnd", "(IZ)V");
	MID_decodeImage = env->GetMethodID(clazz, "decodeImage", "(I)V");
	MID_drawImage = env->GetMethodID(clazz, "drawImage", "(IIIII)V");
	MID_drawBgImage = env->GetMethodID(clazz, "drawBgImage", "(IIIIIII)V");
}

void ImageMgr::Create(NImage* image, NImagePlayer* player)
{
	mPlayer = player;
	if (IsAvailable(image->id)) {
		mImages[image->id] = image;
		JNI_env->CallVoidMethod(OBJ_ImageMgr, MID_createImage, (int)image->id);
	}
}

void ImageMgr::Delete(NImage* image)
{
	if (IsExist(image->id)) {
		mImages[image->id] = N_NULL;
		JNI_env->CallVoidMethod(OBJ_ImageMgr, MID_deleteImage, (int)image->id);
	}
}

void ImageMgr::Decode(NImage* image)
{
	if (IsExist(image->id)) {
		JNI_env->CallVoidMethod(OBJ_ImageMgr, MID_decodeImage, (int)image->id);
	}
}

void ImageMgr::OnData(NImage* image, void* data, int length)
{
	if (IsExist(image->id)) {
		jbyteArray b = JNI_env->NewByteArray(length);
		JNI_env->SetByteArrayRegion(b, 0, length, (jbyte*)data);
		JNI_env->CallVoidMethod(OBJ_ImageMgr, MID_onData, (int)image->id, b);
		JNI_env->DeleteLocalRef(b);
	}
}

void ImageMgr::OnDataEnd(NImage* image, nbool ok)
{
	if (IsExist(image->id)) {
		JNI_env->CallVoidMethod(OBJ_ImageMgr, MID_onDataEnd, (int)image->id, (jboolean)ok);
	}
}

void ImageMgr::OnDecodeOver(int id, jboolean ok, int w, int h)
{
	if (IsExist(id)) {
		if (ok) {
			mImages[id]->size.w = w;
			mImages[id]->size.h = h;
		}
		imagePlayer_onDecodeOver(mPlayer, (int16)id, (nbool)ok);
	}
}

void ImageMgr::Draw(NImage* image, const NRect* r)
{
	if (IsExist(image->id)) {
		JNI_env->CallVoidMethod(OBJ_ImageMgr, MID_drawImage, (int)image->id, r->l, r->t, rect_getWidth(r), rect_getHeight(r));
	}
}

void ImageMgr::DrawBg(NImage* image, const NRect* r, int rx, int ry)
{
	if (IsExist(image->id)) {
		JNI_env->CallVoidMethod(OBJ_ImageMgr, MID_drawBgImage, (int)image->id, r->l, r->t, rect_getWidth(r), rect_getHeight(r), rx, ry);
	}
}

bool ImageMgr::IsAvailable(int id)
{
	return (id >= 0 && id < MAX_IMAGES);
}

bool ImageMgr::IsExist(int id)
{
	return (IsAvailable(id) && mImages[id]);
}

void ImageMgr::StopAll()
{
	NBK_memset(&mImages, 0, sizeof(NImage*) * MAX_IMAGES);
}
