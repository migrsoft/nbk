/*
 * tree.h
 *
 *  Created on: 2011-9-5
 *      Author: wuyulun
 */

#ifndef TREE_H_
#define TREE_H_

#include "../inc/config.h"

#ifdef __cplusplus
extern "C" {
#endif
    
typedef struct _NTreeNode {
    
    nid     node_type;
    uint16  data;
    
    struct _NTreeNode*  parent;
    struct _NTreeNode*  child;
    struct _NTreeNode*  prev;
    struct _NTreeNode*  next;
    
} NTreeNode;
    
#ifdef __cplusplus
}
#endif

#endif /* TREE_H_ */
