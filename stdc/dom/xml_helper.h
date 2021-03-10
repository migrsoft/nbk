/*
 * xml_helper.h
 *
 *  Created on: 2010-12-26
 *      Author: wuyulun
 */

#ifndef XML_HELPER_H_
#define XML_HELPER_H_

#include "../inc/config.h"

#ifdef __cplusplus
extern "C" {
#endif

nid binary_search_id(const char** set, nid last, const char* name);
nid get_tag_id(const char* tag_name);
nid get_att_id(const char* att_name);

#ifdef __cplusplus
}
#endif

#endif /* XML_HELPER_H_ */
