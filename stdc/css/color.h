/* this file is generated automatically */

#ifndef __COLOR_H__
#define __COLOR_H__

#include "../inc/nbk_gdi.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const NColor colorAqua;
extern const NColor colorBlack;
extern const NColor colorBlue;
extern const NColor colorFuchsia;
extern const NColor colorGray;
extern const NColor colorGreen;
extern const NColor colorLightGray;
extern const NColor colorLightGreen;
extern const NColor colorLime;
extern const NColor colorMaroon;
extern const NColor colorNavy;
extern const NColor colorOlive;
extern const NColor colorPurple;
extern const NColor colorRed;
extern const NColor colorSilver;
extern const NColor colorTeal;
extern const NColor colorWhite;
extern const NColor colorYellow;

enum COLORS {
	CC_AQUA = 1,
	CC_BLACK = 2,
	CC_BLUE = 3,
	CC_FUCHSIA = 4,
	CC_GRAY = 5,
	CC_GREEN = 6,
	CC_LIGHTGRAY = 7,
	CC_LIGHTGREEN = 8,
	CC_LIME = 9,
	CC_MAROON = 10,
	CC_NAVY = 11,
	CC_OLIVE = 12,
	CC_PURPLE = 13,
	CC_RED = 14,
	CC_SILVER = 15,
	CC_TEAL = 16,
	CC_WHITE = 17,
	CC_YELLOW = 18,
	CC_LASTCOLOR = 18
};

#define MAX_COLOR_LEN	10

void css_initColor(void);
void css_delColor(void);
const char** css_getColorNames(void);
NColor* css_getColorValues(void);

#ifdef __cplusplus
}
#endif

#endif
