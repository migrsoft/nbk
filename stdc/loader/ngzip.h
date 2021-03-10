#ifndef __NBK_GZIP_H__
#define __NBK_GZIP_H__

#include "../inc/config.h"
#include <zlib.h>

#define NGZ_HEADER	"\x1f\x8b\x8\x0\x0\x0\x0\x0\x0\x3"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NBK_gzip {

	z_stream   strm;

    uint8*  inBuf;
    uint8*  outBuf;

    uint8*  data;
    int     dataLen;
    int     dataSize;

    nbool    init :1;
    nbool    error :1;

    int8    skip;

} NBK_gzip, NGzip;

NBK_gzip* ngzip_create(void);
void ngzip_delete(NBK_gzip** ngz);

// 解压
void ngzip_addZippedData(NBK_gzip* ngz, uint8* data, int len);
void ngzip_closeUnzip(NBK_gzip* ngz);

// 压缩
void ngzip_addUnzippedData(NBK_gzip* ngz, uint8* data, int len);
void ngzip_closeZip(NBK_gzip* ngz);

// 获取结果数据
uint8* ngzip_getData(NBK_gzip* ngz, int* len);

#ifdef __cplusplus
}
#endif

#endif
