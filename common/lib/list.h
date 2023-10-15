#ifndef LIB_LIST_H
#define LIB_LIST_H
#include "../types/std.h"

#define tag_offset(stru, tag_name) &(((stru*)0)->tag_name)
#define tag2entry(stru, tag_name, tag) (stru*)((uint32_t)tag - (uint32_t)tag_offset(stru, tag_name))

typedef struct list_node_ {
    struct list_node_ *pre;
    struct list_node_ *next;
} list_node;
typedef struct {
    list_node *head;
    list_node *tail;
    int size;
} list;

void init_list(list *l);

void pushl(list *l, list_node *node);
list_node *popl(list *l);

void pushr(list *l, list_node *node);
list_node *popr(list *l);

int add(list *l, int index, list_node *node);
list_node *remove(list *l, int index);

list_node *at(list *l, int index);

bool have(list *l, list_node *node);

#endif