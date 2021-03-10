#ifndef __SMARTPTR_H__
#define __SMARTPTR_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*SP_FREE_FUNC)(void** p);

typedef struct _NSmartPtr {

	int				ref; // 引用计数
	SP_FREE_FUNC	free; // 删除函数

} NSmartPtr;

// 智能指针初始化，置引用计数为0，设定删除函数
void smartptr_init(void* ptr, SP_FREE_FUNC func);
// 增加引用计数
void smartptr_get(void* ptr);
// 减少引用计数，当计数为0时，调用删除函数清理指针内存块
void smartptr_release(void* ptr);

#ifdef __cplusplus
}
#endif

#endif //__SMARTPTR_H__
