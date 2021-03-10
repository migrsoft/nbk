// create by wuyulun, 2012.1.25

#include "ngzip.h"

#define CHUNK	16384

static voidpf ngzip_zalloc OF((voidpf opaque, uInt items, uInt size))
{
    if (opaque) items += size - size; /* make compiler happy */
    return sizeof(uInt) > 2 ? (voidpf)NBK_malloc(items * size) :
                              (voidpf)NBK_malloc0(items * size);
}


static void ngzip_zfree  OF((voidpf opaque, voidpf address))
{
    NBK_free(address);
    if (opaque) return; /* make compiler happy */
}


NBK_gzip* ngzip_create(void)
{
	NBK_gzip* g = (NBK_gzip*)NBK_malloc0(sizeof(NBK_gzip));
	g->inBuf = (uint8*)NBK_malloc(CHUNK);
	g->outBuf = (uint8*)NBK_malloc(CHUNK);
	return g;
}

void ngzip_delete(NBK_gzip** ngz)
{
	NBK_gzip* g = *ngz;
	NBK_free(g->inBuf);
	NBK_free(g->outBuf);
	if (g->data)
		NBK_free(g->data);
	NBK_free(g);
	*ngz = N_NULL;
}

static void grow_data_buffer(NBK_gzip* ngz, int add)
{
	int grow = (add < CHUNK) ? CHUNK : CHUNK * 2;
	if (ngz->dataSize == 0) { // 初始分配
		ngz->dataSize = grow;
		ngz->data = (uint8*)NBK_malloc(grow);
	}
	else if (ngz->dataSize - ngz->dataLen < add) { // 扩大
		ngz->dataSize += grow;
		ngz->data = (uint8*)NBK_realloc(ngz->data, ngz->dataSize);
	}
}

// 增加压缩数据
void ngzip_addZippedData(NBK_gzip* ngz, uint8* data, int len)
{
	int err;
	int size, read, offs, have;

	if (ngz->error)
		return;

	if (!ngz->init) {
		ngz->init = N_TRUE;
		ngz->strm.zalloc = ngzip_zalloc;
		ngz->strm.zfree = ngzip_zfree;
		ngz->strm.opaque = Z_NULL;
		ngz->strm.avail_in = 0;
		ngz->strm.next_in = Z_NULL;
		err = inflateInit2(&ngz->strm, -MAX_WBITS);
		if (err != Z_OK) {
			ngz->error = N_TRUE;
			return;
		}
		ngz->skip = 10;
	}
	
	if (ngz->skip && len <= ngz->skip) { // 忽略 gzip 头信息
        ngz->skip -= len;
        return;
	}

	ngz->dataLen = 0;

	size = len - ngz->skip;
	offs = ngz->skip;
	ngz->skip = 0;
	while (size > 0) {
		read = (size > CHUNK) ? CHUNK : size;
		NBK_memcpy(ngz->inBuf, data + offs, read);
		ngz->strm.avail_in = read;
		ngz->strm.next_in = ngz->inBuf;
		size -= read;
		offs += read;

		do {
			ngz->strm.avail_out = CHUNK;
			ngz->strm.next_out = ngz->outBuf;
			err = inflate(&ngz->strm, Z_SYNC_FLUSH);
			have = CHUNK - ngz->strm.avail_out;
			if (have > 0) {
				grow_data_buffer(ngz, have);
				NBK_memcpy(ngz->data + ngz->dataLen, ngz->outBuf, have);
				ngz->dataLen += have;
			}
		} while (ngz->strm.avail_out == 0);
	}
}

void ngzip_closeUnzip(NBK_gzip* ngz)
{
	if (ngz->init && !ngz->error)
		inflateEnd(&ngz->strm);

	ngz->init = N_FALSE;
	ngz->error = N_FALSE;

	ngz->dataLen = 0;
}

void ngzip_addUnzippedData(NBK_gzip* ngz, uint8* data, int len)
{
	int err;
	int size, read, offs, have;

	if (ngz->error)
		return;

	if (!ngz->init) {
		ngz->init = N_TRUE;
		ngz->strm.zalloc = Z_NULL;
		ngz->strm.zfree = Z_NULL;
		ngz->strm.opaque = Z_NULL;
		ngz->strm.avail_in = 0;
		ngz->strm.next_in = Z_NULL;
		err = deflateInit2(&ngz->strm,
			Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
		if (err != Z_OK) {
			ngz->error = N_TRUE;
			return;
		}
	}

	ngz->dataLen = 0;

	size = len;
	offs = 0;
	while (size > 0) {
		read = (size > CHUNK) ? CHUNK : size;
		NBK_memcpy(ngz->inBuf, data + offs, read);
		ngz->strm.avail_in = read;
		ngz->strm.next_in = ngz->inBuf;
		size -= read;
		offs += read;

		do {
			ngz->strm.avail_out = CHUNK;
			ngz->strm.next_out = ngz->outBuf;
			err = deflate(&ngz->strm, Z_SYNC_FLUSH);
			have = CHUNK - ngz->strm.avail_out;
			if (have > 0) {
				grow_data_buffer(ngz, have);
				NBK_memcpy(ngz->data + ngz->dataLen, ngz->outBuf, have);
				ngz->dataLen += have;
			}
		} while (ngz->strm.avail_out == 0);
	}
}

void ngzip_closeZip(NBK_gzip* ngz)
{
	if (ngz->init && !ngz->error)
		deflateEnd(&ngz->strm);

	ngz->init = N_FALSE;
	ngz->error = N_FALSE;

	ngz->dataLen = 0;
}

uint8* ngzip_getData(NBK_gzip* ngz, int* len)
{
	if (ngz->error) {
		*len = 0;
		return N_NULL;
	}
	else {
		*len = ngz->dataLen;
		return ngz->data;
	}
}
