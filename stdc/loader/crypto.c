#include "../inc/config.h"
//#include "rsa.h"
#include "idea.h"
#include "crypto.h"
#include "keygen.h"

#define CRYPTO_BUFFER_SIZE 32768
#define REMAIN_BLOCKSIZE_MAX   32

NBK_Crypto* crypto_create(void)
{
    NBK_Crypto* crypto = (NBK_Crypto*) NBK_malloc0(sizeof(NBK_Crypto));
    crypto->buf = (char*)NBK_malloc0(CRYPTO_BUFFER_SIZE);
    crypto->lastRemainData = (char*)NBK_malloc0(REMAIN_BLOCKSIZE_MAX);
    return crypto;
}

void crypto_delete(NBK_Crypto** ncrypto)
{
    NBK_Crypto* tmp = *ncrypto;
    if (tmp->buf)
        NBK_free(tmp->buf);
    if (tmp->lastRemainData)
        NBK_free(tmp->lastRemainData);
    NBK_free(tmp);
    *ncrypto = 0;
}

//  decrypt
int crypto_decode(NBK_Crypto* ncrypto, const char* inBuf, const int inLen)
{
    int err = 0;
    int len = inLen;
    int remainLen, newlen;
    int headLen = sizeof(NCryptHdr);
    
    if (inLen < headLen) {
        NBK_memcpy(ncrypto->lastRemainData + ncrypto->remainDataLen, (void*)inBuf, inLen);
        err = 1;
        return err;
    }

    if (ncrypto->remainDataLen) {
        if (inLen + ncrypto->remainDataLen > CRYPTO_BUFFER_SIZE) {
            NBK_free(ncrypto->buf);
            ncrypto->buf = (char*) NBK_malloc0(inLen + ncrypto->remainDataLen);
        }

        NBK_memcpy(ncrypto->buf, ncrypto->lastRemainData, ncrypto->remainDataLen);
        NBK_memcpy(ncrypto->buf + ncrypto->remainDataLen, (void*) inBuf, inLen);
        len = inLen + ncrypto->remainDataLen;
        ncrypto->remainDataLen = 0;
    }
    else {
        if (inLen > CRYPTO_BUFFER_SIZE) {
            NBK_free(ncrypto->buf);
            ncrypto->buf = (char*) NBK_malloc0(inLen);
        }
        NBK_memcpy(ncrypto->buf, (void*) inBuf, inLen);
    }

    remainLen = len % 8;
    newlen = len - remainLen;
    
    if (remainLen) {
        NBK_memcpy(ncrypto->lastRemainData + ncrypto->remainDataLen, ncrypto->buf + newlen, remainLen);
        ncrypto->remainDataLen += remainLen;
    }

    if (!ncrypto->isHead) {
        NCryptHdr* header;

        ncrypto->isHead = 1;
        idea_decode_content(ncrypto->buf, headLen, (char*) defaultKey);

        header = (NCryptHdr*) (ncrypto->buf);
        NBK_memcpy(ncrypto->head.reserved_key, &(header->reserved_key), CRYPTO_KEY_SIZE);
        ncrypto->head.len = header->len;
        ncrypto->head.type = header->type;

        if (ncrypto->decryptedSize + len >= ncrypto->head.len) {
            newlen = len / 8 * 8;
        }
        
        if (newlen - headLen) {
            if (NECRYPT_IDEA == ncrypto->head.type) {
                ncrypto->decryptedSize += newlen - headLen;                    
                idea_decode_content(ncrypto->buf + headLen, newlen - headLen, (char*)header->reserved_key);
                ncrypto->result = ncrypto->buf + headLen;
                if (ncrypto->decryptedSize <= ncrypto->head.len)
                    ncrypto->dataLen = newlen - headLen;
                else {
                    ncrypto->dataLen = newlen - headLen - (ncrypto->decryptedSize - ncrypto->head.len);
                    ncrypto->decryptedSize = ncrypto->head.len;
                }
            }
        }
    }
    else {
        if (NECRYPT_IDEA == ncrypto->head.type) {
            idea_decode_content(ncrypto->buf, newlen, (char*)&ncrypto->head.reserved_key);
            ncrypto->result = ncrypto->buf;
            if (ncrypto->decryptedSize <= ncrypto->head.len)
                ncrypto->dataLen = newlen;
            else {
                ncrypto->dataLen = newlen - (ncrypto->decryptedSize - ncrypto->head.len);
                ncrypto->decryptedSize = ncrypto->head.len;
            }
        }
    }

    return err;
    
//    int err = 0;
//    int inLen = inLen;
//    int remainLen, newlen;
//
//    if (inLen < RSA_BLOCKSIZE_MAX) {
//        NBK_memcpy(ncrypto->lastRemainData + ncrypto->remainDataLen, (void*) inBuf, inLen);
//        err = 1;
//        return err;
//    }
//
//    if (ncrypto->remainDataLen) {
//        if (inLen + ncrypto->remainDataLen > CRYPTO_BUFFER_SIZE) {
//            NBK_free(ncrypto->buf);
//            ncrypto->buf = (char*)NBK_malloc0(inLen + ncrypto->remainDataLen);
//        }
//
//        NBK_memcpy(ncrypto->buf, ncrypto->lastRemainData, ncrypto->remainDataLen);
//        NBK_memcpy(ncrypto->buf + ncrypto->remainDataLen, (void*) inBuf, inLen);
//        inLen = inLen + ncrypto->remainDataLen;
//        ncrypto->remainDataLen = 0;
//    }
//    else {
//        if (inLen > CRYPTO_BUFFER_SIZE) {
//            NBK_free(ncrypto->buf);
//            ncrypto->buf = (char*)NBK_malloc0(inLen);
//        }
//        NBK_memcpy(ncrypto->buf, (void*) inBuf, inLen);
//    }
//    
//    remainLen = inLen % 8;
//    newlen = inLen - remainLen;
//    if (remainLen) {
//        NBK_memcpy(ncrypto->lastRemainData + ncrypto->remainDataLen, ncrypto->buf + newlen,remainLen);
//        ncrypto->remainDataLen += remainLen;
//    }
//    
//    if (!ncrypto->isHead) {
//        TRSA_FileHeader* header;
//        ncrypto->isHead = TRUE;
//        err = rsa_decode(ncrypto->buf, newlen, rsa_privatekey);
//        if (!err) {
//            header = (TRSA_FileHeader*) (ncrypto->buf);
//            ncrypto->dataLen = header->reserved_for_RSA[2];
//            NBK_memcpy(ncrypto->head.reserved_key, &(header->reserved_for_RSA[3]), 16);
//            ncrypto->result = ncrypto->buf + sizeof(TRSA_FileHeader);
//        }
//    }
//    else {
//        
//        idea_decode_content(ncrypto->buf, newlen, (char*) (&(ncrypto->head.reserved_key)));
//        ncrypto->result = ncrypto->buf;
//    }
//    
//    return err;
}

// encrypt
int crypto_encode(NBK_Crypto* ncrypto, const char* inBuf, const int inLen)
{
    int idea_len = ((inLen + 7) / 8) * 8;
    int buf_len = sizeof(NCryptHdr) + idea_len;
    int res = 0;
    NCryptHdr* head;

    if (buf_len > CRYPTO_BUFFER_SIZE) {
        NBK_free(ncrypto->buf);
        ncrypto->buf = (char*) NBK_malloc0(buf_len);
    }

    head = (NCryptHdr*) ncrypto->buf;
    
    //data for encrypt
    NBK_memcpy(ncrypto->buf + sizeof(NCryptHdr), (void*)inBuf, inLen);
    getcrc(ncrypto->buf + sizeof(NCryptHdr), inLen, (unsigned int*)head->reserved_key);
    idea_encode_content(ncrypto->buf + sizeof(NCryptHdr), idea_len, (char*)head->reserved_key);

    ncrypto->dataLen = buf_len;

    //head for encrypt
    head->len = inLen;
    head->type = NECRYPT_IDEA;
    idea_encode_content(ncrypto->buf, sizeof(NCryptHdr), (char*)defaultKey);
    ncrypto->result = ncrypto->buf;
    return 0;
  
//    int buf_len = rsa_get_buffer_need(inLen);
//    int res = 0;
//    TRSA_FileHeader* head;
//    if (buf_len > CRYPTO_BUFFER_SIZE) {
//        NBK_free(ncrypto->buf);
//        ncrypto->buf = (char*)NBK_malloc0(buf_len);
//    }
//    
//    //head for encrypt
//    head = (TRSA_FileHeader*) ncrypto->buf;
//    head->len = inLen;
//    //NBK_memcpy(&(head->reserved_for_RSA[3]), ncrypto->reserved_key, 16);
//    //data for encrypt
//    NBK_memcpy(ncrypto->buf + sizeof(TRSA_FileHeader), (void*) inBuf, inLen);
//    ncrypto->dataLen = buf_len;
//    res = rsa_encode_with_default_key(ncrypto->buf);
//    if (!res)
//        ncrypto->result = ncrypto->buf;
}
