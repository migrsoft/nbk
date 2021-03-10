/* this file is generated automatically */

#ifndef __CHAR_REF_H__
#define __CHAR_REF_H__

#ifdef __cplusplus
extern "C" {
#endif

#define CHARREF_TOTAL	10

void xml_initEntityTable(void);
void xml_delEntityTable(void);
const char** xml_getCharrefNames(void);
const char** xml_getEntityNames(void);

#ifdef __cplusplus
}
#endif

#endif
