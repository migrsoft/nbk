#include "../dom/node.h"
#include "../dom/attr.h"
#include "../dom/xml_atts.h"
#include "../dom/document.h"
#include "../render/renderNode.h"
#include "../render/renderInput.h"
#include "../loader/url.h"
#include "../tools/str.h"
#include "formData.h"

typedef struct _NFormItem {
    char*   name;
    wchr*   value;
    int     valLen;
} NFormItem;

NFormData* formData_create(void)
{
    NFormData* d = (NFormData*)NBK_malloc0(sizeof(NFormData));
    return d;
}

void formData_delete(NFormData** data)
{
    NFormData* d = *data;
    formData_reset(d);
    NBK_free(d);
    *data = N_NULL;
}

void formData_reset(NFormData* data)
{
    if (data->lst) {
        NFormItem* i = (NFormItem*)sll_first(data->lst);
        while (i) {
            NBK_free(i->name);
            NBK_free(i->value);
            NBK_free(i);
            i = (NFormItem*)sll_next(data->lst);
        }
        sll_delete(&data->lst);
    }
}

static int find_input(NRenderNode* r, void* user, nbool* ignore)
{
    NSList* lst = (NSList*)user;

    if (!r->display) {
        *ignore = N_TRUE;
    }
    else if (  (r->type == RNT_INPUT
                && (   ((NRenderInput*)r)->type == NEINPUT_TEXT
                    || ((NRenderInput*)r)->type == NEINPUT_PASSWORD ))
             || r->type == RNT_TEXTAREA )
    {
        NNode* n = (NNode*)r->node;
        char* name = attr_getValueStr(n->atts, ATTID_NAME);
        if (name)
            sll_append(lst, r);
    }

    return 0;
}

// 查找所有可用输入控件
static NSList* find_all_input(NFormData* data, NPage* page)
{
    NSList* lst = sll_create();

    rtree_traverse_depth(page->view->root, find_input, N_NULL, lst);
    if (page->subPages) {
        NPage* p = (NPage*)sll_first(page->subPages);
        while (p) {
            if (p->attach)
                rtree_traverse_depth(p->view->root, find_input, N_NULL, lst);
            p = (NPage*)sll_next(page->subPages);
        }
    }
    return lst;
}

// 保存所有有效输入数据
void formData_save(NFormData* data, NPage* page)
{
    NSList* inputs;
    NRenderNode* r;
    NNode* n;
    char* name;
    wchr* value;
    int len;
    NFormItem* i;
    nbool saveLogin = N_TRUE;
    wchr* user = N_NULL;
    wchr* pw = N_NULL;

    formData_reset(data);
    data->lst = sll_create();
    
    inputs = find_all_input(data, page);

    r = (NRenderNode*)sll_first(inputs);
    while (r) {
        n = (NNode*)r->node;
        name = attr_getValueStr(n->atts, ATTID_NAME);
        value = renderNode_getEditText16(r, &len);
        
        i = N_NULL;
        if (len > 0) {
            i = (NFormItem*)NBK_malloc0(sizeof(NFormItem));
            i->name = (char*)NBK_malloc(nbk_strlen(name) + 1);
            i->value = (wchr*)NBK_malloc(sizeof(wchr) * (len + 1));
            i->valLen = len;
            nbk_strcpy(i->name, name);
            nbk_wcsncpy(i->value, value, len);
            sll_append(data->lst, i);
        }

        if (saveLogin && r->type == RNT_INPUT) { // 保存登录信息
            NRenderInput* ri = (NRenderInput*)r;
            if (ri->type == NEINPUT_TEXT)
                user = (i) ? i->value : N_NULL;
            if (ri->type == NEINPUT_PASSWORD) {
                pw = (i) ? i->value : N_NULL;
                if (user && pw) {
                    uint16 port;
                    char* domain = url_parseHostPort(doc_getUrl(page->doc), &port);
                    if (domain) {
                        //NBK_saveLoginData(page->platform, domain, user, pw);
                        NBK_free(domain);
                    }
                }
                saveLogin = N_FALSE;
            }
        }

        r = (NRenderNode*)sll_next(inputs);
    }

    sll_delete(&inputs);
}

void formData_restore(NFormData* data, NPage* page)
{
    NSList* inputs;
    NRenderNode* r;
    NNode* n;
    char* name;
    NFormItem* i;

    if (data->lst == N_NULL)
        return;

    inputs = find_all_input(data, page);

    r = (NRenderNode*)sll_first(inputs);
    while (r) {
        n = (NNode*)r->node;
        name = attr_getValueStr(n->atts, ATTID_NAME);

        i = (NFormItem*)sll_first(data->lst);
        while (i) {
            if (!nbk_strcmp(i->name, name)) {
                renderNode_setEditText16(r, i->value, i->valLen);
                break;
            }
            i = (NFormItem*)sll_next(data->lst);
        }

        r = (NRenderNode*)sll_next(inputs);
    }

    sll_delete(&inputs);

    formData_reset(data);
}

void formData_restoreLoginData(NPage* page)
{
    NSList* inputs = sll_create();
    NRenderNode* u = N_NULL;
    NRenderNode* r;

    rtree_traverse_depth(page->view->root, find_input, N_NULL, inputs);

    r = (NRenderNode*)sll_first(inputs);
    while (r) {
        if (r->type == RNT_INPUT) {
            NRenderInput* ri = (NRenderInput*)r;
            if (ri->type == NEINPUT_TEXT)
                u = r;
            if (ri->type == NEINPUT_PASSWORD) {
                char* domain;
                uint16 port;
                wchr* user = N_NULL;
                wchr* pw = N_NULL;
                int len;

                domain = url_parseHostPort(doc_getUrl(page->doc), &port);
                if (domain) {
                    //NBK_loadLoginData(page->platform, domain, &user, &pw);
                    NBK_free(domain);
                    if (u && user) {
                        len = nbk_wcslen(user);
                        renderNode_setEditText16(u, user, len);
                    }
                    if (pw) {
                        len = nbk_wcslen(pw);
                        renderNode_setEditText16(r, pw, len);
                    }
                }
                break;
            }
        }

        r = (NRenderNode*)sll_next(inputs);
    }

    sll_delete(&inputs);
}
