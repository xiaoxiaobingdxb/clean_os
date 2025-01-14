#include "../common/lib/hlist.h"
#include "../common/lib/hash.h"
#include "stdio.h"

typedef struct {
    hlist_node_t node;
    int32_t data;
} hdata_t;
int main() {
    #define bits 10
    #define max_addr  1 << bits
    hlist_head_t table[max_addr];
    for (int i = 0; i < sizeof(table)/sizeof(hlist_head_t); i++) {
        INIT_HLIST_HEAD(&table[i]);
    }
    hdata_t data = {.data = 3};
    INIT_HLIST_NODE(&data.node);
    uint32_t key = data.data % max_addr;
    key = hash_long(data.data, bits);
    printf("key=%d\n", key);
    hlist_add_head(&data.node, &table[key]);

    if (hlist_empty(&table[key])) {
        printf("data not exist\n");
        return -1;
    } 
    hdata_t *hnode = NULL;
    hlist_node_t *hlist = NULL, *n = NULL;
    hlist_for_each_entry(hnode, hlist, &table[key], node) {
        if (hnode->data == data.data) {
            printf("find_data:%d\n", hnode->data);
        }
    }
    hlist_for_each_entry_safe(hnode, hlist, n, &table[key], node) {
        hlist_del(&hnode->node);
        hnode = NULL;
    }
    return 0;
}