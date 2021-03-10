//
// 输出功能
//

#ifndef __NBK_PROBE_H__
#define __NBK_PROBE_H__

#include "../inc/config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _NBK_probe {

	nbool	buffered;
	nbool	outputTab;
	char*	buffer;
	int		bufPos;

	char*	path;

} NBK_probe;

NBK_probe* probe_create(void);
void probe_delete(NBK_probe** probe);

void probe_setPath(NBK_probe* probe, const char* path);

void probe_outputUint(NBK_probe* probe, unsigned int i);
void probe_outputInt(NBK_probe* probe, int i);
void probe_outputWchr(NBK_probe* probe, unsigned short* wc, int len);
void probe_outputChar(NBK_probe* probe, char* mb, int len);
void probe_outputReturn(NBK_probe* probe);
void probe_outputTime(NBK_probe* probe);
void probe_output(NBK_probe* probe, void* dbgInfo);
void probe_flush(NBK_probe* probe);

void dump_file_init(const char* fname, nbool exist);
void dump_file_data(const char* fname, void* data, int len);

#ifdef __cplusplus
}
#endif

#endif
