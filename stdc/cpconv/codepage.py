# -*- coding: utf-8 -*-

import os
import sys
import string


def gbk_conv():

    area = range(0x81, 0xff)
    code = range(0x40, 0xff)

    out = open('uf_gbk.c', 'w')

    # 生成块代码
    for a in area:

        print 'generating %x...' % a
        
        src = '\nstatic uint16 area_%x[] = {\n' % a
        
        col = 0
        row = '\t'
        for c in code:
            
            if c == 0x7f:
                un_i = 0
            else:
                mb = chr(a) + chr(c)
                un = unicode(mb, 'gbk', 'ignore')
                if len(un) == 0:
                    un_i = 0
                else:
                    un_i = ord(un)

            col += 1
            if col == 9:
                src += row + '\n'
                row = '\t'
                col = 1

            un_h = '0x%x' % un_i
            row += un_h + ', '

        src += row + '0x0\n};\n'
##        print src
##        break
        out.write(src)


    # 生成区索引
    src = '\n'
    src += 'static uint16* index[] = {\n'
    for a in area:
        row = '\tarea_%x,\n' % a
        src += row

    src += '\tN_NULL\n};\n'
    out.write(src)

    out.close()
    
            
if __name__ == "__main__":
    os.chdir('c:/')
    print 'Current folder: ' + os.getcwd()
