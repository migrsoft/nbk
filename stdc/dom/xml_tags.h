/* this file is generated automatically */

#ifndef __XML_TAGS_H__
#define __XML_TAGS_H__

#ifdef __cplusplus
extern "C" {
#endif

enum TAGS {
	TAGID_A = 1,
	TAGID_ANCHOR = 2,
	TAGID_ARTICLE = 3,
	TAGID_ASIDE = 4,
	TAGID_B = 5,
	TAGID_BASE = 6,
	TAGID_BIG = 7,
	TAGID_BODY = 8,
	TAGID_BR = 9,
	TAGID_BUTTON = 10,
	TAGID_CARD = 11,
	TAGID_DIV = 12,
	TAGID_FONT = 13,
	TAGID_FOOTER = 14,
	TAGID_FORM = 15,
	TAGID_FRAME = 16,
	TAGID_FRAMESET = 17,
	TAGID_GO = 18,
	TAGID_H1 = 19,
	TAGID_H2 = 20,
	TAGID_H3 = 21,
	TAGID_H4 = 22,
	TAGID_H5 = 23,
	TAGID_H6 = 24,
	TAGID_HEAD = 25,
	TAGID_HEADER = 26,
	TAGID_HR = 27,
	TAGID_HTML = 28,
	TAGID_IMG = 29,
	TAGID_INPUT = 30,
	TAGID_LABEL = 31,
	TAGID_LI = 32,
	TAGID_LINK = 33,
	TAGID_META = 34,
	TAGID_NAV = 35,
	TAGID_OBJECT = 36,
	TAGID_OL = 37,
	TAGID_ONEVENT = 38,
	TAGID_OPTION = 39,
	TAGID_P = 40,
	TAGID_PARAM = 41,
	TAGID_POSTFIELD = 42,
	TAGID_SCRIPT = 43,
	TAGID_SECTION = 44,
	TAGID_SELECT = 45,
	TAGID_SMALL = 46,
	TAGID_SPAN = 47,
	TAGID_STRONG = 48,
	TAGID_STYLE = 49,
	TAGID_TABLE = 50,
	TAGID_TC_ATTACHMENT = 51,
	TAGID_TD = 52,
	TAGID_TEXTAREA = 53,
	TAGID_TIMER = 54,
	TAGID_TITLE = 55,
	TAGID_TR = 56,
	TAGID_UL = 57,
	TAGID_WML = 58,
	TAGID_TEXT = 59,
	TAGID_LASTTAG = 59,
	TAGID_CLOSETAG = 32000
};

#define MAX_TAG_LEN	13

void xml_initTags(void);
void xml_delTags(void);
const char** xml_getTagNames(void);

#ifdef __cplusplus
}
#endif

#endif
