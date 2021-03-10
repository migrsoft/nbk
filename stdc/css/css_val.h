/*
 * css_val.h
 *
 *  Created on: 2011-1-6
 *      Author: wuyulun
 */

#ifndef CSS_VAL_H_
#define CSS_VAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_FONT_SIZE   16  // default 1em
#define MAX_FONT_SIZE       48
#define DEFAULT_FONT_ADD    2

#define DEFAULT_LINE_SPACE  4

#define UNKNOWN_IMAGE_WIDTH     20
#define UNKNOWN_IMAGE_HEIGHT    20


enum NECssFloat {
    CSS_FLOAT_NONE,
    CSS_FLOAT_LEFT,
    CSS_FLOAT_RIGHT,
    CSS_FLOAT_UNSET
};

enum NECssDisplay {
    CSS_DISPLAY_NONE,
    CSS_DISPLAY_BLOCK,
    CSS_DISPLAY_INLINE_BLOCK,
	CSS_DISPLAY_INLINE,
    CSS_DISPLAY_UNSET
};

enum NECssFloatClear {
    CSS_CLAER_NONE,
    CSS_CLEAR_LEFT,
    CSS_CLEAR_RIGHT,
    CSS_CLEAR_BOTH
};

enum NECssAlign {
    CSS_ALIGN_LEFT,
    CSS_ALIGN_CENTER,
    CSS_ALIGN_RIGHT,
    CSS_ALIGN_JUSTIFY
};

enum NECssVertAlign {
    CSS_VALIGN_BOTTOM,
    CSS_VALIGN_MIDDLE,
    CSS_VALIGN_TOP
};

enum NECssRepeat {
    CSS_REPEAT,
    CSS_REPEAT_NO,
    CSS_REPEAT_X,
    CSS_REPEAT_Y
};

enum NECssPosType {
    CSS_POS_PIXEL,
    CSS_POS_PERCENT,
    CSS_POS_NAME
};

enum NECssFontStyle {
    CSS_FONT_NORMAL,
    CSS_FONT_BOLD
};

enum NECssOverflow {
    CSS_OVERFLOW_VISIBLE,
    CSS_OVERFLOW_HIDDEN,
    CSS_OVERFLOW_SCROLL,
    CSS_OVERFLOW_AUTO,
    CSS_OVERFLOW_INHERIT
};

enum NECssTextDecor {
    CSS_TXTDECOR_NONE,
    CSS_TXTDECOR_UNDERLINE,
    CSS_TXTDECOR_OVERLINE,
    CSS_TXTDECOR_LINETHROUGH,
    CSS_TXTDECOR_BLINK
};

#ifdef __cplusplus
}
#endif

#endif /* CSS_VAL_H_ */
