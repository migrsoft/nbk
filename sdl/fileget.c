// create by wuyulun, 2012.1.18

#include "../stdc/dom/document.h"
#include "../stdc/loader/loader.h"
#include "fileget.h"
#include "resmgr.h"
#include "cache.h"
#include "ini.h"

#define BUFFER_SIZE	16384

enum NEWorkState {
	WORK_INIT,
	WORK_READ,
	WORK_ABORT
};

static void file_transfer(void* user)
{
	int dbg = ini_getInt(NEINI_DBG_FGET);
	NFileGet* fget = (NFileGet*)user;
	nbool stop = N_FALSE;
	int read;

	if (fget->state == WORK_INIT) {
		fget->state = WORK_READ;

		if (dbg) {
			fprintf(stderr, "\n---------- file request ----- (%d) ----->>>\n", fget->id);
			fprintf(stderr, "GET %s\n", fget->path);
		}

		if (fget->path)
			fget->fd = fopen(fget->path, "rb");

		if (fget->fd) {
			fseek(fget->fd, 0, SEEK_END);
			fget->length = ftell(fget->fd);
			fget->read = 0;
			if (dbg) {
				fprintf(stderr, "\n<<<---------- file response ----- (%d) -----\n", fget->id);
				fprintf(stderr, "REQ %s\n", fget->path);
				fprintf(stderr, "content-type [ %d ]\n", fget->mime);
				fprintf(stderr, "content-length [ %d ]\n", fget->length);
			}
			fget->handler(fget, NFILE_EVENT_GOT_RESPONSE, N_NULL, 0, fget->user);
		}
		else {
			stop = N_TRUE;
			fget->handler(fget, NFILE_EVENT_ERROR, N_NULL, 0, fget->user);
		}
	}
	else if (fget->state == WORK_READ) {
		fseek(fget->fd, fget->read, SEEK_SET);
		read = fread(fget->buf, 1, BUFFER_SIZE, fget->fd);
		fget->read += read;
		if (dbg)
			fprintf(stderr, "file (%d) -> %d", fget->id, read);
		fget->handler(fget, NFILE_EVENT_GOT_DATA, fget->buf, read, fget->user);

		if (fget->read >= fget->length) {
			fclose(fget->fd);
			fget->fd = N_NULL;
			stop = N_TRUE;
			if (dbg)
				fprintf(stderr, " OVER\n");
			fget->handler(fget, NFILE_EVENT_COMPLETE, N_NULL, 0, fget->user);
		}
		else {
			if (dbg)
				fprintf(stderr, "\n");
		}
	}

	if (stop) {
		fget->state = WORK_ABORT;
		tim_stop(fget->t);
	}
}

void fsc_transfer(void* user)
{
	int dbg = ini_getInt(NEINI_DBG_FGET);
	NFileGet* fget = (NFileGet*)user;
	nbool stop = N_FALSE;
	int read;
	NCacheMgr* cache = resMgr_getCacheMgr();

	if (fget->state == WORK_INIT) {
		fget->state = WORK_READ;

		if (dbg) {
			fprintf(stderr, "\n---------- fscache request ----- (%d) ----->>>\n", fget->id);
			fprintf(stderr, "GET %s\n", fget->path);
		}

		if (fget->fscId == -1) {
			stop = N_TRUE;
			fget->handler(fget, NFILE_EVENT_ERROR, N_NULL, 0, fget->user);
		}
		else {
			nid type;
			int dataLen, userLen;

			type = cacheMgr_loadDataInfo(cache, fget->fscId, &dataLen, &userLen);
			if (type == NECACHET_CSS)
				fget->mime = NEMIME_DOC_CSS;
			else if (type == NECACHET_CSS_GZ) {
				fget->mime = NEMIME_DOC_CSS;
				fget->gzip = N_TRUE;
			}
			else if (type == NECACHET_PNG)
				fget->mime = NEMIME_IMAGE_PNG;
			else if (type == NECACHET_JPG)
				fget->mime = NEMIME_IMAGE_JPEG;
			else if (type == NECACHET_GIF)
				fget->mime = NEMIME_IMAGE_GIF;

			fget->length = dataLen;
			fget->read = 0;
			if (dbg) {
				fprintf(stderr, "\n<<<---------- fscache response ----- (%d) -----\n", fget->id);
				fprintf(stderr, "REQ %s\n", fget->path);
				fprintf(stderr, "content-type [ %d ]\n", fget->mime);
				fprintf(stderr, "content-length [ %d ]\n", fget->length);
			}
			fget->handler(fget, NFILE_EVENT_GOT_RESPONSE, N_NULL, 0, fget->user);
		}
	}
	else if (fget->state == WORK_READ) {
		read = cacheMgr_loadData(cache, fget->fscId, fget->read, fget->buf, BUFFER_SIZE);
		fget->read += read;
		if (dbg)
			fprintf(stderr, "fscache (%d) -> %d", fget->id, read);
		fget->handler(fget, NFILE_EVENT_GOT_DATA, fget->buf, read, fget->user);

		if (fget->read >= fget->length) {
			stop = N_TRUE;
			if (dbg)
				fprintf(stderr, " OVER\n");
			fget->handler(fget, NFILE_EVENT_COMPLETE, N_NULL, 0, fget->user);
		}
		else {
			if (dbg)
				fprintf(stderr, "\n");
		}
	}

	if (stop) {
		fget->state = WORK_ABORT;
		tim_stop(fget->t);
	}
}

NFileGet* fileGet_create(int id)
{
	NFileGet* f = (NFileGet*)NBK_malloc0(sizeof(NFileGet));
	f->id = id;
	f->fscId = -1;
	return f;
}

void fileGet_delete(NFileGet** fget)
{
	NFileGet* f = *fget;
	if (f->t)
		tim_delete(&f->t);
	if (f->buf)
		NBK_free(f->buf);
	NBK_free(f);
	*fget = N_NULL;
}

void fileGet_setWorkMode(NFileGet* fget, nid mode)
{
	fget->mode = mode;
}

void fileGet_setEventHandler(NFileGet* fget, FILE_EVENT_HANDLER handler, void* user)
{
	fget->handler = handler;
	fget->user = user;
}

void fileGet_start(NFileGet* fget, char* path)
{
	TIMER_CALLBACK callback = file_transfer;
	if (fget->mode == NEFGET_FSCACHE)
		callback = fsc_transfer;

	if (fget->t == N_NULL)
		fget->t = tim_create(callback, fget);

	if (fget->buf == N_NULL)
		fget->buf = (char*)NBK_malloc(BUFFER_SIZE);

	fget->path = path;
	fget->state = WORK_INIT;
	tim_start(fget->t, 10, 10);
}

void fileGet_cancel(NFileGet* fget)
{
	fget->state = WORK_ABORT;
	if (fget->t)
		tim_stop(fget->t);
}
