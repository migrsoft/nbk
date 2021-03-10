/*
 * idea.c
 *
 *  Created on: 2011-5-5
 *      Author: mengkefeng
 */

#include "idea.h"
#ifdef SWITCH_RSA_IDEA
#define IDEAKEYSIZE 16 
#define IDEABLOCKSIZE 8 
#define word16 unsigned short int
#define word32 unsigned int 
#define ROUNDS    8 
#define KEYLEN    (6*ROUNDS+4) 
#define low16(x) ((x) & 0xffff) 
typedef unsigned short int uint16;
typedef word16 IDEAkey[KEYLEN];

#if 0
BIAPI   void idea_encode(char* buffer)
{
    TIDEA_FileHeader* fileheader = (TIDEA_FileHeader*) buffer;
    int i;
    char temp_key[16];
    for (i = 0; i < 16; i++)
    {
        temp_key[i] = (i * 7 - (i % 3) * i);
    }
    //加密正文;
    idea_encode_content(buffer + 20, fileheader->len, fileheader->userkey);
    //混淆文件头信息
    idea_encode_header(buffer);
    //加密文件头信息
    idea_encode_content(buffer, 24, temp_key);
}

void idea_decode(char* buffer)
{
    TIDEA_FileHeader* fileheader = (TIDEA_FileHeader*) buffer;
    int i;
    char temp_key[16];
    for (i = 0; i < 16; i++)
    {
        temp_key[i] = (i * 7 - (i % 3) * i);
    }
    //解密文件头信息
    idea_decode_content(buffer, 24, temp_key);
    //恢复文件头信息
    idea_decode_header(buffer);
    //解密正文；
    idea_decode_content(buffer + 20, fileheader->len, fileheader->userkey);
}

BIAPI   int idea_decodeV2(char* buffer, int bufferlen)
{
    TIDEA_FileHeader* fileheader = (TIDEA_FileHeader*) buffer;
    int i;
    char temp_key[16];
    for (i = 0; i < 16; i++)
    {
        temp_key[i] = (i * 7 - (i % 3) * i);
    }
    if (bufferlen < 28)
        return 1; //长度太小！
    //解密文件头信息
    idea_decode_content(buffer, 24, temp_key);
    //恢复文件头信息
    idea_decode_header(buffer);
    if (bufferlen == (fileheader->len + 27))
    {
        //解密正文；
        idea_decode_content(buffer + 20, fileheader->len, fileheader->userkey);
        return 0; //成功解密；
    }
    return 1;//长度不对！
}
#endif

void en_key_idea(word16 userkey[8], IDEAkey Z);
void de_key_idea(IDEAkey Z, IDEAkey DK);
void cipher_idea(word16 in[4], word16 out[4], IDEAkey Z);
uint16 inv(uint16 x);
uint16 mul(uint16 a, uint16 b);

#if 0
void idea_encode_header(char* hearder)
{
    char temp_buff[20];
    int i, j;

    for (i = 0; i < 20; i++)
    {
        unsigned char tmp = (unsigned char) (hearder[i]);
        tmp = 255 - tmp;
        hearder[i] = (char) tmp;
    }

    for (i = 0; i < 4; i++)
    {
        temp_buff[i * 5] = hearder[i];
        for (j = 0; j < 4; j++)
        {
            temp_buff[i * 5 + j + 1] = hearder[(i + 1) * 4 + j];
        }
    }

    for (i = 0; i < 10; i++)
    {
        hearder[i] = temp_buff[2 * i];
        hearder[i + 10] = temp_buff[2 * i + 1];
    }
}

void idea_decode_header(char* hearder)
{
    char temp_buff[20];
    int i, j;

    for (i = 0; i < 20; i++)
    {
        unsigned char tmp = (unsigned char) (hearder[i]);
        tmp = 255 - tmp;
        hearder[i] = (char) tmp;
    }

    for (i = 0; i < 10; i++)
    {
        temp_buff[2 * i] = hearder[i];
        temp_buff[2 * i + 1] = hearder[i + 10];
    }

    for (i = 0; i < 4; i++)
    {
        hearder[i] = temp_buff[i * 5];
        for (j = 0; j < 4; j++)
        {
            hearder[(i + 1) * 4 + j] = temp_buff[i * 5 + j + 1];
        }
    }

}
#endif

void idea_encode_content(char* buffer, int len, char* userkey) //buffer容量（单位字节）必须是8的倍数，不足部分补0. key长16字节
{
    word16* input_pos;
    word16 output[4];
    IDEAkey Z;
    int i, j;
    int count;

    word16* key = (word16*) userkey;
    en_key_idea(key, Z);

    count = (len / 8) + ((len & 7) ? 1 : 0);

    for (i = 0; i < count; i++)
    {
        input_pos = (word16*) (buffer + i * 8);

        cipher_idea(input_pos, output, Z);

        for (j = 0; j < 4; j++)
        {
            input_pos[j] = output[j];
        }
    }
}

void idea_decode_content(char* buffer, int len, char* userkey) //buffer容量（单位字节）必然是8的倍数，key长16字节
{
    word16* input;
    word16 output[4];
    int i, j;
    IDEAkey Z, DK;
    int count;
    word16 *key;

    key = (word16*) userkey;

    count = (len / 8) + ((len & 7) ? 1 : 0);

    en_key_idea(key, Z);
    de_key_idea(Z, DK);

    for (i = 0; i < count; i++)
    {
        input = (word16*) (buffer + i * 8);
        cipher_idea(input, output, DK);
        for (j = 0; j < 4; j++)
        {
            input[j] = output[j];
        }
    }

}

uint16 inv(uint16 x)
{
    uint16 t0, t1;
    uint16 q, y;
    if (x <= 1)
        return x;
    t1 = (uint16) (0x10001l / x);
    y = (uint16) (0x10001l % x);
    if (y == 1)
        return low16(1-t1);
    t0 = 1;
    do
    {
        q = x / y;
        x = x % y;
        t0 += q * t1;
        if (x == 1)
            return t0;
        q = y / x;
        y = y % x;
        t1 += q * t0;
    }
    while (y != 1);
    return low16(1-t1);
}

void en_key_idea(word16 *userkey, word16 *Z)
{
    int i, j;
    /* shifts */
    for (j = 0; j < 8; j++)
        Z[j] = *userkey++;
    for (i = 0; j < KEYLEN; j++)
    {
        i++;
        Z[i + 7] = ((Z[i & 7] << 9) | (Z[i + 1 & 7] >> 7));
        Z += i & 8;
        i &= 7;
    }
}

void de_key_idea(IDEAkey Z, IDEAkey DK)
{
    int j;
    uint16 t1, t2, t3;
    IDEAkey T;
    word16 *p = T + KEYLEN;
    t1 = inv(*Z++);
    t2 = -*Z++;
    t3 = -*Z++;
    *--p = inv(*Z++);
    *--p = t3;
    *--p = t2;
    *--p = t1;
    for (j = 1; j < ROUNDS; j++)
    {
        t1 = *Z++;
        *--p = *Z++;
        *--p = t1;
        t1 = inv(*Z++);
        t2 = -*Z++;
        t3 = -*Z++;
        *--p = inv(*Z++);
        *--p = t2;
        *--p = t3;
        *--p = t1;
    }
    t1 = *Z++;
    *--p = *Z++;
    *--p = t1;
    t1 = inv(*Z++);
    t2 = -*Z++;
    t3 = -*Z++;
    *--p = inv(*Z++);
    *--p = t3;
    *--p = t2;
    *--p = t1;
    /*copy and destroy temp copy*/
    for (j = 0, p = T; j < KEYLEN; j++)
    {
        *DK++ = *p;
        *p++ = 0;
    }
}

uint16 mul(uint16 a, uint16 b)
{
    word32 p;

    if (a)
    {
        if (b)
        {
            p = (word32) a * b;
            b = (uint16) (low16(p));
            a = (uint16) (p >> 16);
            return b - a + (b < a);
        }
        else
        {
            return 1 - a;
        }
    }
    else
        return 1 - b;
}

#define MUL(x,y) (x=mul(low16(x),y)) 

#define CONST 

void cipher_idea(word16 in[4], word16 out[4], register CONST IDEAkey Z)
{
    register uint16 x1, x2, x3, x4, t1, t2;
    int r = ROUNDS;
    x1 = *in++;
    x2 = *in++;
    x3 = *in++;
    x4 = *in;
    do
    {
        MUL(x1,*Z++);
        x2 += *Z++;
        x3 += *Z++;
        MUL(x4,*Z++);
        t2 = x1 ^ x3;
        MUL(t2,*Z++);
        t1 = t2 + (x2 ^ x4);
        MUL(t1,*Z++);
        t2 = t1 + t2;
        x1 ^= t1;
        x4 ^= t2;
        t2 ^= x2;
        x2 = x3 ^ t1;
        x3 = t2;
    }
    while (--r);
    MUL(x1,*Z++);
    *out++ = x1;
    *out++ = (x3 + *Z++);
    *out++ = (x2 + *Z++);
    MUL(x4,*Z);
    *out = x4;
}

#endif //#ifdef SWITCH_RSA_IDEA
