/* this file is generated automatically */

#include "../inc/config.h"
#include "../tools/strList.h"
#include "xml_tags.h"

static NStrList* l_xmlTagNames = N_NULL;

void xml_initTags(void)
{
    const char* tags = \
		",a,anchor,article,aside,b,base,big,body,br,button" \
		",card,div,font,footer,form,frame,frameset,go,h1,h2" \
		",h3,h4,h5,h6,head,header,hr,html,img,input" \
		",label,li,link,meta,nav,object,ol,onevent,option,p" \
		",param,postfield,script,section,select,small,span,strong,style,table" \
		",tc_attachment,td,textarea,timer,title,tr,ul,wml" \
		",text";

    l_xmlTagNames = strList_create(tags);
}

void xml_delTags(void)
{
    strList_delete(&l_xmlTagNames);
}

const char** xml_getTagNames(void)
{
    return (const char**)l_xmlTagNames->lst;
}
