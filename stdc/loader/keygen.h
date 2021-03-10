#ifndef KEY_GEN_H_
#define KEY_GEN_H_

#ifdef __cplusplus
extern "C" {
#endif
    
extern const int defaultKey[4];
int getcrc(char* databuf, int datalen, unsigned int* crcbuf);

#ifdef __cplusplus
}
#endif
#endif 
