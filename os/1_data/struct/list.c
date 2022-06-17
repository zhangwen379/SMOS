#include "list.h"
#include "trace.h"
// 初始化双向链表
void list_init (list* plist)
{
	plist->head.prev =0;
	plist->head.next = &plist->tail;
	plist->tail.prev = &plist->head;
	plist->tail.next = 0;
	plist->head.belong=plist;
	plist->tail.belong=plist;
}
// 把链表变量插入在节点after之后
void list_insert_after(list_node* after, list_node* pnode)
{
	pnode->next = after->next;
	after->next->prev=pnode;
	after->next = pnode;
	pnode->prev = after;

	pnode->belong=after->belong;
}
// 把链表变量插入在节点before之前
void list_insert_before(list_node* before, list_node* pnode)
{
	before->prev->next = pnode;

	pnode->prev = before->prev;
	pnode->next = before;

	before->prev = pnode;
	pnode->belong=before->belong;
}
// 添加链表变量到链表队首
void list_push(list* plist, list_node* pnode)
{
	list_insert_after(&plist->head, pnode);
}

// 追加链表变量到链表队尾
void list_append(list* plist, list_node* pnode)
{
	list_insert_before(&plist->tail, pnode);
}

// 使链表变量脱离链表
void list_remove(list_node* pnode)
{

	pnode->prev->next = pnode->next;
	pnode->next->prev = pnode->prev;
	pnode->belong=0;
}

// 将链表第一个链表变量脱离链表
list_node* list_pop(list* plist)
{
	list_node* node = plist->head.next;
	list_remove(node);
	return node;
} 

// 判断链表是否为空
bool list_is_empty(list* plist)
{
	return plist->head.next == &plist->tail;
}

// 返回链表长度
int32_t list_len(list* plist)
{
	list_node* node = plist->head.next;
	int32_t length = 0;
	while (node != &plist->tail)
	{
		length++;
		node = node->next;
	}
	return length;
}

// 从链表中查找链表节点指针
bool list_node_belong(list* plist, list_node* pnode)
{
	return pnode->belong==plist;
}

list_node *list_node_get_by_index(list *plist, int32_t index)
{
	int32_t count=0;
	list_node* node = plist->head.next;
	while (node != &plist->tail) {
		if (index==count) {
			return node;
		}
		node = node->next;
		count++;
	}
	return 0;
}



list_node *list_node_get_by_id(list *plist, int32_t iID)
{
	list_node* node = plist->head.next;
	while (node != &plist->tail) {
		if (node->iID==iID) {
			return node;
		}
		node = node->next;
	}
	return 0;
}

int32_t list_node_get_max_id(list *plist)
{
	int32_t iID=0;
	list_node* node = plist->head.next;
	while (node != &plist->tail)
	{
		if (node->iID>iID)
		{
			iID=node->iID;
		}
		node = node->next;
	}
	return iID;
}
