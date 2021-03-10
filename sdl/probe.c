// create by wuyulun, 2011.12.27

#include "../stdc/inc/nbk_cbDef.h"
#include "../stdc/tools/str.h"
#include "../stdc/tools/unicode.h"
#include <stdio.h>
#include <string.h>
#include "probe.h"
#include "resmgr.h"
#include "runtime.h"
#include "ini.h"

#define FLUSH_IMMEDIATELY   0

#define PROBE_BUF_SIZE  4096

#define PROBE_DUMP_PATH	"nbk_dump.txt"
#define PROBE_NULL_PTR	"(null)"

NBK_probe* probe_create(void)
{
	NBK_probe* p = (NBK_probe*)NBK_malloc0(sizeof(NBK_probe));
	p->buffer = (char*)NBK_malloc(PROBE_BUF_SIZE);
	p->buffered = N_TRUE;
	p->outputTab = N_TRUE;
	return p;
}

void probe_delete(NBK_probe** probe)
{
	NBK_probe* p = *probe;
	probe_flush(p);
	NBK_free(p->buffer);
	NBK_free(p);
	*probe = N_NULL;
}

void probe_outputUint(NBK_probe* probe, unsigned int i)
{
    NBK_DbgInfo dinfo;
    dinfo.t = NBKDBG_UINT;
    dinfo.d.ui = i;
    probe_output(probe, &dinfo);
}

void probe_outputInt(NBK_probe* probe, int i)
{
    NBK_DbgInfo dinfo;
    dinfo.t = NBKDBG_INT;
    dinfo.d.si = i;
    probe_output(probe, &dinfo);
}

void probe_outputWchr(NBK_probe* probe, unsigned short* wc, int len)
{
    NBK_DbgInfo dinfo;
    dinfo.t = NBKDBG_WCHR;
    dinfo.d.wp = wc;
    dinfo.len = len;
    probe_output(probe, &dinfo);
}

void probe_outputChar(NBK_probe* probe, char* mb, int len)
{
    NBK_DbgInfo dinfo;
    dinfo.t = NBKDBG_CHAR;
    dinfo.d.cp = mb;
    dinfo.len = len;
    probe_output(probe, &dinfo);
}

void probe_outputReturn(NBK_probe* probe)
{
    NBK_DbgInfo dinfo;
    dinfo.t = NBKDBG_RETURN;
    probe_output(probe, &dinfo);
}

void probe_outputTime(NBK_probe* probe)
{
}

void probe_output(NBK_probe* probe, void* dbgInfo)
{
    NBK_DbgInfo* dinfo = (NBK_DbgInfo*)dbgInfo;
    int space = 0;
    char* u8 = NULL;
	char number[16];
    
    if (   (dinfo->t == NBKDBG_WCHR && !dinfo->d.wp)
		|| (dinfo->t == NBKDBG_CHAR && !dinfo->d.cp) ) {
        dinfo->t = NBKDBG_CHAR;
        dinfo->d.cp = PROBE_NULL_PTR;
        dinfo->len = -1;
    }

    switch (dinfo->t) {
    case NBKDBG_INT:
    case NBKDBG_UINT:
        space = 10;
        break;
        
    case NBKDBG_WCHR:
        space = (dinfo->len == -1) ? nbk_wcslen(dinfo->d.wp) : dinfo->len;
        u8 = uni_utf16_to_utf8_str(dinfo->d.wp, space, NULL);
        space = nbk_strlen(u8);
        break;
        
    case NBKDBG_CHAR:
        space = (dinfo->len == -1) ? nbk_strlen(dinfo->d.cp) : dinfo->len;
        break;
    }
    space += 1;
    
    if (probe->bufPos + space > PROBE_BUF_SIZE)
        probe_flush(probe);
    
    switch (dinfo->t) {
    case NBKDBG_INT:
    {
        if (probe->outputTab) {
			sprintf(number, "%d\t", dinfo->d.si);
        }
        else {
            sprintf(number, "%d", dinfo->d.si);
        }
		nbk_strcpy(probe->buffer + probe->bufPos, number);
        probe->bufPos += nbk_strlen(number);
        break;
    }
        
    case NBKDBG_UINT:
    {
        if (probe->outputTab) {
            sprintf(number, "%u\t", dinfo->d.ui);
        }
        else {
            sprintf(number, "%u", dinfo->d.ui);
        }
		nbk_strcpy(probe->buffer + probe->bufPos, number);
        probe->bufPos += nbk_strlen(number);
        break;
    }
    
    case NBKDBG_WCHR:
    {
        nbool addSuffix = N_FALSE;
        if (space > PROBE_BUF_SIZE) {
            space = PROBE_BUF_SIZE - 3;
            addSuffix = N_TRUE;
        }
        NBK_memcpy(probe->buffer + probe->bufPos, u8, space - 1);
        NBK_free(u8);
        probe->bufPos += space - 1;
        if (addSuffix) {
            NBK_memcpy(probe->buffer + probe->bufPos, "...", 3);
            probe->bufPos += 3;
        }
        if (probe->outputTab)
            probe->buffer[probe->bufPos++] = '\t';
        break;
    }
    
    case NBKDBG_CHAR:
    {
        nbool addSuffix = N_FALSE;
        if (space > PROBE_BUF_SIZE) {
            space = PROBE_BUF_SIZE - 3;
            addSuffix = N_TRUE;
        }
        NBK_memcpy(probe->buffer + probe->bufPos, dinfo->d.cp, space - 1);
        probe->bufPos += space - 1;
        if (addSuffix) {
            NBK_memcpy(probe->buffer + probe->bufPos, "...", 3);
            probe->bufPos += 3;
        }
        if (probe->outputTab)
            probe->buffer[probe->bufPos++] = '\t';
        break;
    }
    
    case NBKDBG_RETURN:
        probe->buffer[probe->bufPos++] = '\n';
        break;
        
    case NBKDBG_TIME:
        probe_outputTime(probe);
        break;
        
    case NBKDBG_FLUSH:
        probe_flush(probe);
        break;
        
    case NBKDBG_TAB:
        probe->outputTab = (nbool)dinfo->d.on;
        break;
    }
    
#if FLUSH_IMMEDIATELY
    probe_flush(probe);
#endif
}

void probe_flush(NBK_probe* probe)
{
	FILE* fd;
	char path[MAX_CACHE_PATH_LEN];

	if (probe->bufPos == 0)
		return;

	sprintf(path, "%s%s", ini_getString(NEINI_DATA_PATH), PROBE_DUMP_PATH);
	fd = fopen(path, "a");
	if (fd) {
		fwrite(probe->buffer, 1, probe->bufPos, fd);
		fclose(fd);
	}

	probe->bufPos = 0;
}

void dump_file_init(const char* fname, nbool exist)
{
	FILE* fd;

	if (exist && !nbk_pathExist(fname))
		return;

	fd = fopen(fname, "wb");
	if (fd)
		fclose(fd);
}

void dump_file_data(const char* fname, void* data, int len)
{
	FILE* fd;

	if (!nbk_pathExist(fname))
		return;

	fd = fopen(fname, "ab");
	if (fd) {
		fwrite(data, 1, len, fd);
		fclose(fd);
	}
}
