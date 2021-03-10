/* this file is generated automatically */

#include "../inc/config.h"
#include "../tools/strList.h"
#include "xml_atts.h"

static NStrList* l_xmlAttNames = N_NULL;

void xml_initAtts(void)
{
    const char* atts = \
		",action,align,alt,bgcolor,border,cellpadding,cellspacing,charset,checked,class" \
		",color,cols,content,disabled,enctype,height,href,http-equiv,id,maxlength" \
		",method,multiple,name,ne_display,onenterbackward,onenterforward,onpick,ontimer,readonly,rel" \
		",rows,selected,selected_p,size,src,style,tc_cuteoffsetx,tc_cuteoriginw,tc_filename,tc_ia" \
		",tc_isads,tc_maintopic,tc_name,tc_nidx,tc_onfocus,tc_originrect,tc_pclip,tc_rate,tc_rect,tc_segment" \
		",tc_stretch,title,type,url,value,width" \
		;

    l_xmlAttNames = strList_create(atts);
}

void xml_delAtts(void)
{
    strList_delete(&l_xmlAttNames);
}

const char** xml_getAttNames(void)
{
    return (const char**)l_xmlAttNames->lst;
}
