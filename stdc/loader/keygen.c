const int defaultKey[4] = { 0xde22c128, 0x92050104, 0x1fe951e2, 0xc0204b02 };

int getcrc(char* databuf, int datalen, unsigned int* crcbuf)
{
    unsigned int Crc = 0;
    unsigned int iCrcTable[256];
    int i, j;
    for (i = 0; i < 256; i++)
    {
        Crc = i;
        for (j = 0; j < 8; j++)
        {
            if (Crc & 1)
            {
                Crc = (Crc >> 1) ^ 0xEDB88320;
            }
            else
            {
                Crc >>= 1;
            }
        }
        iCrcTable[i] = Crc;
    }

    crcbuf[0] = 0xffffffff;
    crcbuf[1] = 0xffffffff;
    crcbuf[2] = 0xffffffff;
    crcbuf[3] = 0xffffffff;
    for (i = 0; i < datalen; i++)
    {
        unsigned int va1;
        unsigned char tmp;

        va1 = i % 4;
        tmp = databuf[i];
        crcbuf[va1] = (crcbuf[va1] >> 8) ^ iCrcTable[(crcbuf[va1] & 0xFF) ^ tmp];
    }

    crcbuf[0] = (crcbuf[0]) ^ 0xFFFFFFFF;
    crcbuf[1] = (crcbuf[1]) ^ 0xFFFFFFFF;
    crcbuf[2] = (crcbuf[2]) ^ 0xFFFFFFFF;
    crcbuf[3] = (crcbuf[3]) ^ 0xFFFFFFFF;
    return 0;
}
