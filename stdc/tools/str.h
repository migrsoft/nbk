/*
 * str.h
 *
 *  Created on: 2010-12-26
 *      Author: wuyulun
 */

#ifndef STR_H_
#define STR_H_

#include "../inc/config.h"

#ifdef __cplusplus
extern "C" {
#endif

// for ascii
int nbk_strlen(const char* s);
int nbk_strcmp(const char* s1, const char* s2);
int nbk_strncmp(const char* s1, const char* s2, int len);
int nbk_strncmp_nocase(const char* s1, const char* s2, int len);
char* nbk_strcpy(char* dst, const char* src);
char* nbk_strncpy(char* dst, const char* src, int len);
int nbk_strfind(const char* str, const char* sub);
int nbk_strfind_nocase(const char* str, const char* sub);
int nbk_strnfind_nocase(const char* str, const char* sub, int len);
int nbk_strchr(const char* str, char ch);

int		str_lastIndexOf(const char* str, const char* search);
void	str_toLower(char* str, int len);
char*	str_clone(const char* str);
char*	str_md5(const char* str);

int NBK_atoi(const char* s);
uint32 nbk_htoi(const char* s);
float NBK_atof(const char* s);
double NBK_atol(const char* s);

// for unicode
int nbk_wcslen(const wchr* s);
int8 nbk_wcscmp(const wchr* s1, const wchr* s2);
wchr* nbk_wcscpy(wchr* dst, const wchr* src);
wchr* nbk_wcsncpy(wchr* dst, const wchr* src, int len);
int nbk_wcsfind(const wchr* str, const wchr* sub);

// helper
uint8* str_skip_invisible_char(const uint8* s, const uint8* e, nbool* end);
void nbk_unescape(char* dst, char* src);
    
#ifdef __cplusplus
}
#endif

#endif /* STR_H_ */
