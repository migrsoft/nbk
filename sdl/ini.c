// create by wuyulun, 2012.1.30

#include "nbk_conf.h"
#include "../stdc/tools/str.h"
#include <stdio.h>
#include <stdlib.h>
#include "ini.h"

#ifdef PLATFORM_WIN32

	#include <direct.h>

	#define NBK_DATA_PATH	"nbkdata/"
	#define NBK_TTF_FONT	"\\fonts\\simsun.ttc"
	#define NBK_HOMEPAGE	"file://nbk_home.htm"
	#define	NBK_TESTPAGE	"file://nbk_test.htm"
	#define NBK_PAGE_ERROR	"file://nbk_error.htm"
	#define NBK_PAGE_404	"file://nbk_404.htm"

#endif // PLATFORM_WIN32

#ifdef PLATFORM_LINUX

	#include <unistd.h>

	#define NBK_DATA_PATH	"nbkdata/"
	#define NBK_TTF_FONT	"/usr/share/fonts/uming.ttc"
	#define NBK_HOMEPAGE	"file://nbk_home.htm"
	#define	NBK_TESTPAGE	"file://nbk_test.htm"
	#define NBK_PAGE_ERROR	"file://nbk_error.htm"
	#define NBK_PAGE_404	"file://nbk_404.htm"

#endif // PLATFORM_LINUX

#ifdef PLATFORM_WEBOS
	#define NBK_INI			"nbk.ini"
#else
	#define NBK_INI			"nbk.ini"
#endif // PLATFORM_WEBOS

#define NBK_WIDTH		"360"
#define NBK_HEIGHT		"640"
#define NBK_CBS			"http://cbs.baidu.com:80/"
#define NBK_MPIC		"http://cbs.baidu.com:80/?tc-res=6"
#define NBK_TF          "http://gate.baidu.com/tc?bd_page_type=1&src="

#define NBK_PLATFORM	"s3"
#define NBK_FROM		"1000a"

#define NBK_IMG_QUALITY "8"

static char* key_names[] = {
	"data-path",
	"ttf-font",
	"homepage",
	"testpage",
	"page-error",
	"page-404",

	"width",
	"height",
	"font-size",
	"image-off",
    "image-quality",
	"init-mode",
	"self-adapt",
    "block-collapse",

	"cbs",
	"mpic",
	"pic",
    "tf",

	"platform",
	"from",

	"debug-http",
	"debug-fget",

	"dump-doc",
    "dump-img",

	0
};

typedef struct _NIni {
	char*	conf;
	char*	kv[NEINI_LAST];
    char*   work_path;
	char*	data_path;
	char*	font_path;
} NIni;

static NIni* l_nbk_ini = N_NULL;

static int get_key_id(char* key)
{
	int i;
	for (i=0; key_names[i]; i++) {
		if (nbk_strcmp(key_names[i], key) == 0)
			return i;
	}
	return -1;
}

// 返回下一行起点
static char* read_line(char* txt)
{
    char* p = txt;
    while (*p) {
        if (*p == '\r' || *p == '\n') {
            *p++ = 0;
            if (*p == '\n')
                p++;
            return p;
        }
        p++;
    }
    return N_NULL;
}

// 解析配置文件
// key = value \r\n
static void parse_ini(NIni* ini, int size)
{
	char* t = ini->conf;
    char *p, *h;
	nbool findEqua;
	int id = -1;
    char* pair[2];
    int i;

    do {
        h = read_line(t);

        p = t;
        i = 0;
        pair[0] = pair[1] = N_NULL;
        findEqua = N_TRUE;

        while (*p) {
            if (*p == '#') {
                *p = 0;
                break;
            }
            else if (*p <= ' ') {
                *p = 0;
            }
            else if (findEqua && *p == '=') {
                findEqua = N_FALSE;
                *p = 0;
                i++;
            }
            else if (pair[i] == N_NULL) {
                pair[i] = p;
            }
            p++;
        }

        //if (pair[0]) {
        //    fprintf(stderr, "key\t%s\n", pair[0]);
        //    fprintf(stderr, "  >\t%s\n", pair[1]);
        //}

        if (pair[0] && pair[1]) {
            id = get_key_id(pair[0]);
            if (id != -1)
                ini->kv[id] = pair[1];
        }

        t = h;

    } while (h);
}

static void path_convert(char* dst, const char* path)
{
    int pos;

	if (*(path+1) == ':' || *path == '/' || *path == '\\') { // 为绝对路径，直接使用
		nbk_strcpy(dst, path);
	}
	else { // 为相对路径，与基址拼接
		pos = nbk_strlen(dst);
		dst[pos++] = '/';
		nbk_strcpy(dst + pos, path);
	}

    for (pos = 0; dst[pos]; pos++) {
        if (dst[pos] == '\\')
            dst[pos] = '/';
    }
}

// 配置初始化
void ini_init(void)
{
	NIni* ini;
	FILE* fd;

	if (l_nbk_ini)
		return;

	ini = (NIni*)NBK_malloc0(sizeof(NIni));
	l_nbk_ini = ini;

    ini->work_path = (char*)NBK_malloc(NBK_MAX_PATH);
	ini->data_path = (char*)NBK_malloc(NBK_MAX_PATH);
	ini->font_path = (char*)NBK_malloc(NBK_MAX_PATH);

    // 获取工作目录
#ifdef PLATFORM_WIN32
	_getcwd(ini->work_path, NBK_MAX_PATH);
#endif
#ifdef PLATFORM_LINUX
	getcwd(ini->work_path, NBK_MAX_PATH);
#endif

    nbk_strcpy(ini->data_path, ini->work_path);
	nbk_strcpy(ini->font_path, ini->work_path);

	// 默认值
	ini->kv[NEINI_DATA_PATH]    = NBK_DATA_PATH;
	//ini->kv[NEINI_TTF_FONT]     = NBK_TTF_FONT;

	ini->kv[NEINI_HOMEPAGE]     = NBK_HOMEPAGE;
	ini->kv[NEINI_TESTPAGE]     = NBK_TESTPAGE;
	ini->kv[NEINI_PAGE_ERROR]   = NBK_PAGE_ERROR;
	ini->kv[NEINI_PAGE_404]     = NBK_PAGE_404;

	ini->kv[NEINI_WIDTH]        = NBK_WIDTH;
	ini->kv[NEINI_HEIGHT]       = NBK_HEIGHT;

	ini->kv[NEINI_CBS]          = NBK_CBS;
	ini->kv[NEINI_MPIC]         = NBK_MPIC;
    ini->kv[NEINI_TF]           = NBK_TF;

	ini->kv[NEINI_PLATFORM]     = NBK_PLATFORM;
	ini->kv[NEINI_FROM]         = NBK_FROM;

    ini->kv[NEINI_IMAGE_QUALITY]    = NBK_IMG_QUALITY;

	// 读入ini
	fd = fopen(NBK_INI, "rb");
	if (fd) {
		int size;

		fseek(fd, 0, SEEK_END);
		size = ftell(fd);
		fseek(fd, 0, SEEK_SET);

		if (size) {
			ini->conf = (char*)NBK_malloc(size + 1);
			size = fread(ini->conf, 1, size, fd);
            ini->conf[size] = 0;
			parse_ini(ini, size);

            // 产生缓存绝对路径
			path_convert(ini->data_path, ini->kv[NEINI_DATA_PATH]);

            // 产生字体绝对路径
            if (ini->kv[NEINI_TTF_FONT]) { // 使用用户设置
			    path_convert(ini->font_path, ini->kv[NEINI_TTF_FONT]);
            }
            else { // 使用默认设置
#ifdef PLATFORM_WIN32
                char* windir = getenv("windir");
                sprintf(ini->font_path, "%s%s", windir, NBK_TTF_FONT);
#else
                path_convert(ini->font_path, NBK_TTF_FONT);
#endif
            }
		}
		fclose(fd);
	}
	else {
		fprintf(stderr, "NBK: nbk.ini not found!\n");
	}
}

void ini_end(void)
{
	if (l_nbk_ini) {
		NIni* ini = l_nbk_ini;
		if (ini->conf)
			NBK_free(ini->conf);
        NBK_free(ini->work_path);
		NBK_free(ini->data_path);
		NBK_free(ini->font_path);
		NBK_free(ini);
		l_nbk_ini = N_NULL;
	}
}

const char* ini_getString(int key)
{
	if (key == NEINI_DATA_PATH)
		return l_nbk_ini->data_path;
	else if (key == NEINI_TTF_FONT)
		return l_nbk_ini->font_path;
	else
		return l_nbk_ini->kv[key];
}

int ini_getInt(int key)
{
	NIni* ini = l_nbk_ini;

	if (ini->kv[key])
		return NBK_atoi(ini->kv[key]);
	else
		return 0;
}

const char* get_work_path(void)
{
    if (l_nbk_ini)
        return l_nbk_ini->work_path;
    else
        return N_NULL;
}
