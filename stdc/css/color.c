/* this file is generated automatically */

#include "../inc/config.h"
#include "../tools/strList.h"
#include "color.h"

const NColor colorAqua = {0, 0xff, 0xff, 255};
const NColor colorBlack = {0, 0, 0, 255};
const NColor colorBlue = {0, 0, 255, 255};
const NColor colorFuchsia = {0xff, 0, 0xff, 255};
const NColor colorGray = {0x80, 0x80, 0x80, 255};
const NColor colorGreen = {0, 0x80, 0, 255};
const NColor colorLightGray = {211, 211, 211, 255};
const NColor colorLightGreen = {144, 238, 144, 255};
const NColor colorLime = {0, 0xff, 0, 255};
const NColor colorMaroon = {0x80, 0, 0, 255};
const NColor colorNavy = {0, 0, 0x80, 255};
const NColor colorOlive = {0x80, 0x80, 0, 255};
const NColor colorPurple = {0x80, 0, 0x80, 255};
const NColor colorRed = {255, 0, 0, 255};
const NColor colorSilver = {0xc0, 0xc0, 0xc0, 255};
const NColor colorTeal = {0, 0x80, 0x80, 255};
const NColor colorWhite = {255, 255, 255, 255};
const NColor colorYellow = {0xff, 0xff, 0, 255};

static NStrList* l_colorNames = N_NULL;

static NColor l_colorValues[] = {
    {0, 0, 0, 0},
	{0, 0xff, 0xff, 255},
	{0, 0, 0, 255},
	{0, 0, 255, 255},
	{0xff, 0, 0xff, 255},
	{0x80, 0x80, 0x80, 255},
	{0, 0x80, 0, 255},
	{211, 211, 211, 255},
	{144, 238, 144, 255},
	{0, 0xff, 0, 255},
	{0x80, 0, 0, 255},
	{0, 0, 0x80, 255},
	{0x80, 0x80, 0, 255},
	{0x80, 0, 0x80, 255},
	{255, 0, 0, 255},
	{0xc0, 0xc0, 0xc0, 255},
	{0, 0x80, 0x80, 255},
	{255, 255, 255, 255},
	{0xff, 0xff, 0, 255}
};

void css_initColor(void)
{
    const char* colors = \
		",aqua,black,blue,fuchsia,gray,green" \
		",lightgray,lightgreen,lime,maroon,navy,olive" \
		",purple,red,silver,teal,white,yellow" \
     ;

    l_colorNames = strList_create(colors);
}
void css_delColor(void)
{
    strList_delete(&l_colorNames);
}

const char** css_getColorNames(void)
{
    return (const char**)l_colorNames->lst;
}

NColor* css_getColorValues(void)
{
    return l_colorValues;
}
