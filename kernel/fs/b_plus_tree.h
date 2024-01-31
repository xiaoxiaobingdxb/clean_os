#if !defined(B_PLUS_TREE)
#define B_PLUS_TREE

struct _tree_node_t;
typedef struct _tree_node_t {
    void *value;
    int children_size;
    struct _tree_node_t *children;
} tree_node_t;




#endif // B_PLUS_TREE
