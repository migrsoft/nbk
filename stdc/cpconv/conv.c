#include "conv.h"
#include "uf_gbk.h"

int conv_gbk_to_unicode(char* mbs, int mbsLen, wchr* wcs, int wcsLen)
{
	return unicode_str_from_gbk(mbs, mbsLen, wcs, wcsLen);
}
