/*
 * idea.h
 *
 *  Created on: 2011-5-5
 *      Author: mengkefeng
 */

#ifndef IDEA_H_
#define IDEA_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "crypto_def.h"
    
#ifdef SWITCH_RSA_IDEA

//typedef struct TIDEA_FileHeader {
//    int len;
//    char userkey[16];
//} TIDEA_FileHeader;

//IDEA加密,输入buff的结构示意如下，buff大小= 要加密的内容+27字节；
//struct TIdea_Buffer
//{
//    int len;   要加密的内容长度（32位整数，占用4字节）
//    char userkey[16];   16字节长度的密钥。
//    char filecontent[len]; 要加密的内容
//    char reservedspace[7]; 保留7字节空间用于块对齐;
//};
//void idea_encode(char* buffer);
//void idea_decode(char* buffer);

//考虑到旧版IDEA解密接口没有相应的验证机制，
//输入错误的数据可能导致内存越界访问而崩溃，
//所以这次在原来的基础上新增了一个idea_decodeV2接口，
//功能和idea_decode一样，新接口会在解密时验证数据长度，防止内存越界访问。

/*!
* IDEA解密接口v2（带数据长度验证）
* \param[in] buffer 要解密的数据,解密后的数据也写在这段内存上
* \param[in] bufferlen 要解密的数据（密文）"buffer"的长度，也就是   明文长度 + 27字节（起验证作用）
* \return 解密成功返回0，解密出错返回非零数值 
* \since 1.0.0.0
*/
//int idea_decodeV2(char* buffer,int buffer_len);

//===================================

//void idea_encode_header(char* hearder);
//void idea_decode_header(char* hearder);
//buffer容量（单位字节）必须是8的倍数，不足部分补0.key长16字节
void idea_encode_content(char* buffer, int len, char* userkey);
//buffer容量（单位字节）必然是8的倍数，key长16字节
void idea_decode_content(char* buffer, int len, char* userkey);
#endif //#ifdef SWITCH_RSA_IDEA

#ifdef __cplusplus
}
#endif
#endif /* IDEA_H_ */
