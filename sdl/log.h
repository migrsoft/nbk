#ifndef __NBK_LOG_H__
#define __NBK_LOG_H__

#define NBK_LOG(fmt, args, ...) fprintf(stderr, fmt, args)
#define NET_LOG(fmt, ...) fprintf(stderr, fmt, ...)
#define RES_LOG(fmt, ...) fprintf(stderr, fmt, ...)

#endif
