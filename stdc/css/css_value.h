/* this file is generated automatically */

#ifndef __CSS_VALUE_H__
#define __CSS_VALUE_H__

#ifdef __cplusplus
extern "C" {
#endif

enum NCSSVALUE {
	CSSVAL_BLOCK = 1,
	CSSVAL_BOLD = 2,
	CSSVAL_BOTH = 3,
	CSSVAL_BOTTOM = 4,
	CSSVAL_CENTER = 5,
	CSSVAL_DASHED = 6,
	CSSVAL_DOTTED = 7,
	CSSVAL_FIXED = 8,
	CSSVAL_HIDDEN = 9,
	CSSVAL_INLINE = 10,
	CSSVAL_INLINE_BLOCK = 11,
	CSSVAL_LARGE = 12,
	CSSVAL_LEFT = 13,
	CSSVAL_MEDIUM = 14,
	CSSVAL_MIDDLE = 15,
	CSSVAL_NO_REPEAT = 16,
	CSSVAL_NONE = 17,
	CSSVAL_REPEAT_X = 18,
	CSSVAL_REPEAT_Y = 19,
	CSSVAL_RIGHT = 20,
	CSSVAL_SMALL = 21,
	CSSVAL_SOLID = 22,
	CSSVAL_TOP = 23,
	CSSVAL_UNDERLINE = 24,
	CSSVAL_LAST = 24
};

void css_initVals(void);
void css_delVals(void);
const char** css_getValueNames();

#ifdef __cplusplus
}
#endif

#endif
