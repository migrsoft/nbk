/*
 * url.h
 *
 *  Created on: 2011-2-16
 *      Author: wuyulun
 */

#ifndef URL_H_
#define URL_H_

#include "../inc/config.h"

#ifdef __cplusplus
extern "C" {
#endif

//// 手机站uri判定
//void url_initPhoneSite(void);
//void url_delPhoneSite(void);
//nbool url_checkPhoneSite(const char* url);

//// 手机站白名单判定
//void url_parseWapWhiteList(uint8* data, int len);
//void url_freeWapWhiteList(void);
//nbool url_inWapWhiteList(const char* url);
//uint8* url_getWL(int* num);

void url_unescape(char* dst, const char* src, nbool forUrl);

// return: new url. caller is responsible for release.
char* url_parse(const char* base, const char* url);

// 获取主机名
char* url_parseHostPort(char* uri, uint16* port);

// 获取文件名
char* url_getFileName(char* uri);

char* url_hex_to_str(char* url, char* value);
char* url_str_to_hex(char* url, char* str);

// 是否包含查询信息
nbool url_hasQuery(const char* url);
    
#ifdef __cplusplus
}
#endif

#endif /* URL_H_ */
