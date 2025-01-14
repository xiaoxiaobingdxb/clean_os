#if !defined(LIB_HLIST_H)
#define LIB_HLIST_H

#include "../types/basic.h"

#ifndef ARCH_HAS_PREFETCH
#define prefetch(x) __builtin_prefetch(x)
#endif

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifndef container_of
/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({			\
	const typeof(((type *)0)->member) * __mptr = (ptr);	\
	(type *)((char *)__mptr - offsetof(type, member)); })
#endif


typedef struct hlist_node {
    struct hlist_node *next, **pprev;
} hlist_node_t;
typedef struct hlist_head {
    hlist_node_t *first;
} hlist_head_t;

/*
 * Double linked lists with a single pointer list head.
 * Mostly useful for hash tables where the two pointer list head is
 * too wasteful.
 * You lose the ability to access the tail in O(1).
 */

#define HLIST_HEAD_INIT {.first = NULL}
#define HLIST_HEAD(name) struct hlist_head name = {.first = NULL}
#define INIT_HLIST_HEAD(ptr) ((ptr)->first = NULL)
static inline void INIT_HLIST_NODE(struct hlist_node *h) {
    h->next = (hlist_node_t *)NULL;
    h->pprev = (hlist_node_t **)NULL;
}

static inline int hlist_unhashed(const struct hlist_node *h) {
    return !h->pprev;
}

static inline int hlist_empty(const struct hlist_head *h) { return !h->first; }

static inline void __hlist_del(struct hlist_node *n) {
    struct hlist_node *next = n->next;
    struct hlist_node **pprev = n->pprev;
    *pprev = next;
    if (next)
        next->pprev = pprev;
}

static inline void hlist_del(struct hlist_node *n) {
    __hlist_del(n);
    n->next = (hlist_node_t *)NULL;
    n->pprev = (hlist_node_t **)NULL;
}

static inline void hlist_del_init(struct hlist_node *n) {
    if (!hlist_unhashed(n)) {
        __hlist_del(n);
        INIT_HLIST_NODE(n);
    }
}

static inline void hlist_add_head(struct hlist_node *n, struct hlist_head *h) {
    struct hlist_node *first = h->first;
    n->next = first;
    if (first)
        first->pprev = &n->next;
    h->first = n;
    n->pprev = &h->first;
}

/* next must be != NULL */
static inline void hlist_add_before(struct hlist_node *n,
                                    struct hlist_node *next) {
    n->pprev = next->pprev;
    n->next = next;
    next->pprev = &n->next;
    *(n->pprev) = n;
}

static inline void hlist_add_after(struct hlist_node *n,
                                   struct hlist_node *next) {
    next->next = n->next;
    n->next = next;
    next->pprev = &n->next;

    if (next->next)
        next->next->pprev = &next->next;
}

/* after that we'll appear to be on some hlist and hlist_del will work */
static inline void hlist_add_fake(struct hlist_node *n) { n->pprev = &n->next; }

/*
 * Move a list from one list head to another. Fixup the pprev
 * reference of the first entry if it exists.
 */
static inline void hlist_move_list(struct hlist_head *old,
                                   struct hlist_head *new_node) {
    new_node->first = old->first;
    if (new_node->first)
        new_node->first->pprev = &new_node->first;
    old->first = (hlist_node_t *)NULL;
}

#define hlist_entry(ptr, type, member) container_of(ptr, type, member)

#define hlist_for_each(pos, head)                      \
    for (pos = (head)->first; pos && ({                \
                                  prefetch(pos->next); \
                                  1;                   \
                              });                      \
         pos = pos->next)

#define hlist_for_each_safe(pos, n, head)        \
    for (pos = (head)->first; pos && ({          \
                                  n = pos->next; \
                                  1;             \
                              });                \
         pos = n)

/**
 * hlist_for_each_entry	- iterate over list of given type
 * @tpos:	the type * to use as a loop cursor.
 * @pos:	the &struct hlist_node to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the hlist_node within the struct.
 */
#define hlist_for_each_entry(tpos, pos, head, member)                          \
    for (pos = (head)->first; pos && ({                                        \
                                  prefetch(pos->next);                         \
                                  1;                                           \
                              }) &&                                            \
                              ({                                               \
                                  tpos =                                       \
                                      hlist_entry(pos, typeof(*tpos), member); \
                                  1;                                           \
                              });                                              \
         pos = pos->next)

/**
 * hlist_for_each_entry_continue - iterate over a hlist continuing after current
 * point
 * @tpos:	the type * to use as a loop cursor.
 * @pos:	the &struct hlist_node to use as a loop cursor.
 * @member:	the name of the hlist_node within the struct.
 */
#define hlist_for_each_entry_continue(tpos, pos, member)                     \
    for (pos = (pos)->next; pos && ({                                        \
                                prefetch(pos->next);                         \
                                1;                                           \
                            }) &&                                            \
                            ({                                               \
                                tpos =                                       \
                                    hlist_entry(pos, typeof(*tpos), member); \
                                1;                                           \
                            });                                              \
         pos = pos->next)

/**
 * hlist_for_each_entry_from - iterate over a hlist continuing from current
 * point
 * @tpos:	the type * to use as a loop cursor.
 * @pos:	the &struct hlist_node to use as a loop cursor.
 * @member:	the name of the hlist_node within the struct.
 */
#define hlist_for_each_entry_from(tpos, pos, member)           \
    for (; pos && ({                                           \
               prefetch(pos->next);                            \
               1;                                              \
           }) &&                                               \
           ({                                                  \
               tpos = hlist_entry(pos, typeof(*tpos), member); \
               1;                                              \
           });                                                 \
         pos = pos->next)

/**
 * hlist_for_each_entry_safe - iterate over list of given type safe against
 * removal of list entry
 * @tpos:	the type * to use as a loop cursor.
 * @pos:	the &struct hlist_node to use as a loop cursor.
 * @n:		another &struct hlist_node to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the hlist_node within the struct.
 */
#define hlist_for_each_entry_safe(tpos, pos, n, head, member)                  \
    for (pos = (head)->first; pos && ({                                        \
                                  n = pos->next;                               \
                                  1;                                           \
                              }) &&                                            \
                              ({                                               \
                                  tpos =                                       \
                                      hlist_entry(pos, typeof(*tpos), member); \
                                  1;                                           \
                              });                                              \
         pos = n)

#endif  // LIB_HLIST_H
