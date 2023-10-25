#include "list.h"

#include "../types/basic.h"

void init_list(list *l) {
    l->head.pre = NULL;
    l->head.next = &l->tail;
    l->tail.pre = &l->head;
    l->tail.next = NULL;
    l->size = 0;
}

void pushl(list *l, list_node *node) {
    node->pre = &l->head;
    node->next = l->head.next;
    l->head.next->pre = node;
    l->head.next = node;
    l->size++;
}

list_node *popl(list *l) {
    if (l->size <= 0) {
        return NULL;
    }
    list_node *node = l->head.next;
    l->head.next = node->next;
    node->next->pre = &l->head;
    node->pre = NULL;
    node->next = NULL;
    l->size--;
    return node;
}

void pushr(list *l, list_node *node) {
    node->pre = l->tail.pre;
    node->next = &l->tail;
    l->tail.pre->next = node;
    l->tail.pre = node;
    l->size++;
}

list_node *popr(list *l) {
    if (l->size <= 0) {
        return NULL;
    }
    list_node *node = l->tail.pre;
    l->tail.pre = node->pre;
    node->pre->next = &l->tail;
    node->pre = NULL;
    node->next = NULL;
    l->size--;
    return node;
}

int add(list *l, int index, list_node *node) {
    if (index <= 0) {
        pushl(l, node);
        return 0;
    }
    if (index >= l->size) {
        pushr(l, node);
        return l->size - 1;
    }
    list_node *p = l->head.next;
    for (int i = 0; i < index; i++) {
        p = p->next;
    }
    node->next = p;
    node->pre = p->pre;
    p->pre->next = node;
    p->pre = node;

    l->size++;
    return index;
}
list_node *remove(list *l, int index) {
    list_node *p = at(l, index);
    if (p == NULL) {
        return NULL;
    }
    p->pre->next = p->next;
    p->next->pre = p->pre;
    p->pre = NULL;
    p->next = NULL;
    l->size--;
    return p;
}

list_node *at(list *l, int index) {
    if (index < 0 || index >= l->size) {
        return NULL;
    }
    list_node *p = l->head.next;
    for (int i = 0; i < index; i++) {
        p = p->next;
    }
    return p;
}

bool have(list *l, list_node *node) {
    for (list_node *p = l->head.next; p != &l->tail && p != NULL;
         p = p->next) {
        if (p == node) {
            return true;
        }
    }
    return false;
}