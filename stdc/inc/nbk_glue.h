#ifndef __NBK_GLUE_H__
#define __NBK_GLUE_H__

#ifdef __WIN32__
	#include <io.h>
#endif

#ifdef __ANDROID__
	#include <android/log.h>
	#define __LINUX__
#endif

#ifdef __LINUX__
	#include <unistd.h>
	#include <sys/stat.h>
#endif

#ifdef __WIN32__
	#define NBK_ACCESS(path, mode) _access(path, mode)
	#define NBK_MKDIR(dir, mode)
#endif

#ifdef __LINUX__
	#define NBK_ACCESS(path, mode) access(path, mode)
	#define NBK_MKDIR(dir, mode) mkdir(dir, mode)
#endif

#ifdef __ANDROID__
	#define NBK_LOG(...) __android_log_print(ANDROID_LOG_INFO, "nbk", __VA_ARGS__)
#else
	#define NBK_LOG(...) printf(__VA_ARGS__)
#endif

#endif // __NBK_GLUE_H__
