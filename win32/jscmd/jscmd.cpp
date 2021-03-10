// jscmd JavaScript 解释器
// Copyright (C) 2012, Wu Yulun
//

#include <Windows.h>
#include <tchar.h>
#include <stdio.h>

#include "../../stdc/inc/config.h"
#include "../../stdc/tools/str.h"
#include "../../stdc/tools/hashMap.h"
#include "../../stdc/js/script.h"
#include "../../stdc/js/js_keywords.h"

#include "../../stdc/cpconv/conv.h"

extern void runtime_statistic(void);

void* g_dp = N_NULL;

void msg_handler(JMessage* msg)
{
	switch (msg->type) {
	case JSMSG_CODE:
		fprintf(stdout, "%3d: %s\n", msg->d.code.lineNo, msg->d.code.code);
		break;

    case JSMSG_TOKEN:
        {
            JToken* t = msg->d.token;
            char buf[256];
            nbk_strncpy(buf, t->text, t->textLen);
            fprintf(stdout, "[%d] Ln %3d col %3d \t%s\n", t->type, t->row, t->col, buf);
        }
        break;

    case JSMSG_SYNTAX_ERROR:
        fprintf(stdout, "Syntax error at line %d col %d\n", msg->d.synErr.lineNo, msg->d.synErr.col);
        break;
	}
}

void test(void)
{
	//void* v;
	//NHashMap* map = hashMap_create(10);
	//hashMap_put(map, "new", (void*)1);
	//hashMap_put(map, "free", (void*)2);
	//hashMap_put(map, "class", (void*)3);
	//hashMap_dump(map);
	//hashMap_put(map, "new", (void*)4);
	//hashMap_get(map, "new", &v);
	//printf("%d\n", (int)v);
	//hashMap_remove(map, "new");
	//hashMap_dump(map);
	//hashMap_delete(&map);
	//runtime_statistic();

	//char* han = "\xd6\xd0\xb9\xfahan\xba\xba\xd7\xd6";
	//wchr uni[8];
	//int n = conv_gbk_to_unicode(han, nbk_strlen(han), uni, 8);
}

int _tmain(int argc, _TCHAR* argv[])
{
	JScript* script;
	char* source;
	int length;

	if (argc < 2) {
		fprintf(stdout, "Usage: jscmd [some.js]\n");
		test();
	}
	else {
		_TCHAR* fname = argv[1];

		FILE* fd = _wfopen(fname, L"r");
		if (fd == 0) {
			fwprintf(stdout, L"open %s error!", fname);
			return -1;
		}

		fwprintf(stdout, L"interpreting %s...\n", fname);

		fseek(fd, 0, SEEK_END);
		length = ftell(fd);
		fseek(fd, 0, SEEK_SET);

		source = (char*)NBK_malloc(length + 1);
		length = fread(source, 1, length, fd);
		source[length] = 0;
		fclose(fd);

		// 执行
		script = script_create();
		script_setMsgListener(script, msg_handler);
		script_execute(script, source, length);
		script_delete(&script);
		// 完成

		NBK_free(source);

		runtime_statistic();
	}

	return 0;
}
