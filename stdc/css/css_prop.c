/* this file is generated automatically */

#include "../inc/config.h"
#include "../tools/strList.h"
#include "css_prop.h"

static NStrList* l_cssPropNames = N_NULL;

void css_initProp(void)
{
    const char* props = \
		",background,background-attachment,background-color,background-image,background-position" \
		",background-repeat,border,border-bottom,border-color,border-left" \
		",border-right,border-style,border-top,clear,color" \
		",display,float,font-size,font-weight,height" \
		",line-height,margin,margin-bottom,margin-left,margin-right" \
		",margin-top,max-width,opacity,overflow,padding" \
		",padding-bottom,padding-left,padding-right,padding-top,text-align" \
		",text-decoration,text-indent,vertical-align,width,z-index" \
		;

    l_cssPropNames = strList_create(props);
}

void css_delProp(void)
{
    strList_delete(&l_cssPropNames);
}

const char** css_getPropNames(void)
{
    return (const char**)l_cssPropNames->lst;
}
