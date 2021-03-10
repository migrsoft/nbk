#ifndef __NBK_CRYPTO_H__
#define __NBK_GRYPTO_H__

#ifdef __cplusplus
extern "C" {
#endif

#define CRYPTO_LEVEL 128
#define CRYPTO_KEY_SIZE ((CRYPTO_LEVEL + 7) / 8)
#define CRYPTO_BLOCKSIZE_MAX (CRYPTO_KEY_SIZE + 4)

typedef struct _NCryptHdr {
    int type; //加密类型
    int len; //明文数据长度; 4字节
    int reserved_key[4]; //128bit key for en/decrypt
} NCryptHdr;
    
typedef struct _NBK_Crypto {
    NCryptHdr head;
    int     decryptedSize;
    char*   result;   //result for en/decrypt
    int     dataLen;    //dataLen of result
    int     isHead;
    //used en/decrypt inside
    char*   lastRemainData;   //not handle inbuf max is RSA_BLOCKSIZE_MAX
    int     remainDataLen;
    char*   buf;
} NBK_Crypto;

enum NECryptType {
    NECRYPT_IDEA = 1,
    NECRYPT_AES = 2
};

NBK_Crypto* crypto_create(void);
void crypto_delete(NBK_Crypto** ncrypto);

// @aInbuf plaintext data,
// @aInLen plaintext data length
// @return 0 if succeed,fail return fail code
int crypto_decode(NBK_Crypto* ncrypto, const char* inbuf, const int inLen);

// @aInbuf ciphertext data,
// @aInLen ciphertext data lenght
// @return 0 if succeed,fail return fail code
int crypto_encode(NBK_Crypto* ncrypto, const char* inbuf, const int inLen);

#ifdef __cplusplus
}
#endif

#endif
