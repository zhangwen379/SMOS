#ifndef __SMOS_LIST_H
#define __SMOS_LIST_H

#include "stdint.h"
#define ioffset(struct_type,struct_member_name) (int32_t)(&((struct_type*)0)->struct_member_name)
#define elem2entry(struct_type, struct_member_name, elem_ptr) \
    (struct_type*)((int)elem_ptr - ioffset(struct_type, struct_member_name))

/**********   定义链表结点成员结构   ************/
struct _tag_list;
typedef  struct _taglist_node
{
	struct _taglist_node* prev; // 前躯结点
	struct _taglist_node* next; // 后继结点
    struct _tag_list* belong;
    int32_t iID;
}list_node;

/* 链表结构,用来实现队列 */
typedef  struct _tag_list{
	list_node head;
	list_node tail;
}list;


void list_init (list*plist);
void list_insert_after(list_node* after, list_node* pnode);
void list_insert_before(list_node* before, list_node* pnode);
void list_push(list* plist, list_node* pnode);
void list_append(list* plist, list_node* pnode);
void list_remove(list_node* pnode);
list_node* list_pop(list* plist);
bool list_is_empty(list* plist);
int32_t list_len(list* plist);

bool list_node_belong(list* plist, list_node* pnode);
list_node* list_node_get_by_index(list* plist, int32_t index);
list_node* list_node_get_by_id(list* plist,int32_t iID);
int32_t list_node_get_max_id(list* plist);

#endif  //__SMOS_LIST_H
