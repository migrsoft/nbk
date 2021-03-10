# -*- coding: utf-8 -*-

import os
import string

s_comment = '/* this file is generated automatically */\n'
s_cpp_begin = '\n#ifdef __cplusplus\nextern "C" {\n#endif\n\n'
s_cpp_end = '\n#ifdef __cplusplus\n}\n#endif\n\n'

def gen_enum(inFile, idPre, lst=[]):
    if len(lst):
        lines = lst
    else:
        f = open(inFile, 'r')
        lines = f.readlines()
        f.close()

    enum = ''
    max_length = 0
    i = 1
    for l in lines:
        l = string.strip(l)
        if  len(l) == 0:
            continue

        max_length = max(max_length, len(l))
        l = string.upper(l)
        l = string.replace(l, '-', '_')
        if i > 1:
            enum += ',\n'
        s = '\t%s%s = %d' % (idPre, l, i)
        i += 1

        enum += s

    return (enum, i, max_length)


def gen_string(lines, reserved = True, numPreRow=9):

    num = numPreRow
    
    str = ''
    if reserved:
        i = 1
    else:
        i = 0
    j = 0
    for l in lines:
        if l[-1] == '\n':
            l = l[:-1]
        if len(l) == 0:
            continue

        if j == 0:
            str += '\t\t"'

        if i > 0:
            str += ','
        str += l
        
        if j == num:
            str += '" \\\n'

        i += 1
        if j == num:
            j = 0
        else:
            j += 1

    if j > 0 and j <= num:
        str += '" \\\n'

    return str


def gen_string_array(lines):
    s = '\t""'
    for l in lines:
        if l[-1] == '\n':
            l = l[:-1]
        if len(l) == 0:
            continue

        s += ',\n\t"' + l + '"'

    return s
        

# 创建关键字定义表
def create_keywords():
    os.chdir('c:/nbk/js')

    # 生成头文件
    doth = s_comment
    doth += '''
#ifndef __JS_KEYWORDS_H__
#define __JS_KEYWORDS_H__
'''
    doth += s_cpp_begin

    doth += 'enum JEKEYWORDS {\n'
    ret = gen_enum('js_keywords.in', 'JKWID_')
    doth += ret[0]
    doth += ',\n'
    doth += '\tJKWID_LAST = %d\n' % ret[1]
    doth += '};\n'

    doth += '\n#define MAX_KEYWORD_LEN\t%d\n' % ret[2]

    doth += '''
void js_keywordsInit(void);
void js_keywordsRelease(void);
const char** js_getKeywordNames(void);
nid js_getKeywordId(const char* word);
'''

    doth += s_cpp_end
    doth += '#endif\n'

    f = open('js_keywords.h', 'w')
    f.write(doth)
    f.close()

    print 'keyword total: %d\t max length %d' % (ret[1], ret[2])

    # 生成源文件
    dotc = s_comment

    dotc += '''
#include "../stdc/inc/config.h"
#include "../stdc/tools/hashMap.h"
#include "js_keywords.h"

static const char* l_names[] = {
'''

    f = open('js_keywords.in', 'r')
    lines = f.readlines()
    f.close()
    dotc += gen_string_array(lines)
    
    dotc += '''
};

static NHashMap* l_map = N_NULL;

void js_keywordsInit(void)
{
    if (l_map == N_NULL) {
        int i;
        l_map = hashMap_create(JKWID_LAST);
        for (i=1; i < JKWID_LAST; i++) {
            hashMap_put(l_map, l_names[i], (void*)i);
        }
    }
}

void js_keywordsRelease(void)
{
    if (l_map)
        hashMap_delete(&l_map);
}

const char** js_getKeywordNames(void)
{
    return (const char**)l_names;
}

nid js_getKeywordId(const char* word)
{
    void* v = 0;
    if (hashMap_get(l_map, word, &v))
        return (nid)v;
    else
        return N_INVALID_ID;
}
'''

    f = open('js_keywords.c', 'w')
    f.write(dotc)
    f.close()

