#include "list.h"

#include "../types/basic.h"

void init_list(list *l) {
    l->head = l->tail = NULL;
    l->size = 0;
}

void pushl(list *l, list_node *node) {
    node->pre = node->next = NULL;
    if (l->head == NULL) {
        l->head = l->tail = node;
    } else {
        node->next = l->head;
        l->head->pre = node;
        l->head = node;
    }
    l->size++;
}
list_node *popl(list *l) {
    if (l->head == NULL) {
        return NULL;
    }
    list_node *ret = l->head;
    if (l->head->next == NULL) {  // only one
        l->head = l->tail = NULL;
    } else {
        l->head = l->head->next;
        l->head->pre = NULL;
    }
    ret->pre = ret->next = NULL;
    l->size--;
    return ret;
}

void pushr(list *l, list_node *node) {
    node->pre = node->next = NULL;
    if (l->head == NULL) {
        l->head = l->tail = node;
    } else {
        node->pre = l->tail;
        l->tail->next = node;
        l->tail = node;
    }
    l->size++;
}

list_node *popr(list *l) {
    if (l->head == NULL) {
        return NULL;
    }
    list_node *ret = l->tail;
    if (l->tail->pre == NULL) {  // only one
        l->head = l->tail = NULL;
    } else {
        l->tail = l->tail->pre;
        l->tail->next = NULL;
    }
    ret->pre = ret->next = NULL;
    l->size--;
    return ret;
}

int add(list *l, int index, list_node *node) {}
list_node *remove(list *l, int index) {}

list_node *at(list *l, int index) {
    list_node *p = l->head;
    for (int i = 0; i <= index; i++) {
        if (p == NULL) {
            return NULL;
        }
        p = p->next;
    }
    return p;
}

bool have(list *l, list_node *node) {
    for (list_node *p = l->head; p != NULL; p = p->next) {
        if (p == node) {
            return true;
        }
    }
    return false;
}