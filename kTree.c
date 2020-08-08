// Implementation of K-tree
// ADT which accepts an arbitrary number of children per node
// Using resizing array provided by James Balajan
// By Finn Button
// Multithreading by James

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <stdint.h>

#include "kTree.h"

#define DEFAULT_CAPACITY 10

struct baseNode {
    Item value;
    struct baseNode **children;
    struct baseNode *parent;
    uint32_t size;
    uint32_t capacity;

    pthread_rwlock_t read_write_lock_node;
};

struct kTree {
    struct baseNode *root;
};

typedef struct baseNode baseNode;
typedef struct kTree kTree;

kTree *create_new_tree() {
    kTree *new = malloc(sizeof(kTree));
    new->root = NULL;
    return new;
}

void free_tree_node(Node old) {
    assert(old != NULL);
    assert(old->children != NULL);
    assert(old->value.data != NULL);
    (*old->value.custom_free)(old->value.data);

    pthread_rwlock_destroy(&old->read_write_lock_node);

    free(old->children);
    free(old);
}

static void recursively_free_nodes(Node node) {
    if (node == NULL) {
        return;
    }

    for (int i = 0; i < node->size; ++i) {
        recursively_free_nodes(node->children[i]);
    }
    free_tree_node(node);
}

void free_tree(Tree old) {
    assert (old != NULL);

    recursively_free_nodes(old->root);
    free(old);
}

Node create_new_node_tree(Item value) {
    Node new = malloc(sizeof(struct baseNode));
    if (new == NULL) {
        fprintf(stderr, "Unable to allocate new tree node. Aborting...\n");
        exit(EXIT_FAILURE);
    }
    
    new->value = value;
    new->children = malloc(sizeof(Node) * DEFAULT_CAPACITY);
    if (new->children == NULL) {
        fprintf(stderr, "Unable to allocate new tree node children. Aborting...\n");
        exit(EXIT_FAILURE);
    }
    new->parent = NULL;

    new->capacity = DEFAULT_CAPACITY;
    new->size = 0;

    // Multithreading synchronization primitives
    pthread_rwlock_init(&new->read_write_lock_node, NULL);
    
    return new;
}

void set_node_value_tree(Node n, Item value) {
    n->value = value;
}

void set_root_tree(Tree t, Node n) {
    assert(t != NULL);

    if (t->root != NULL) {
        recursively_free_nodes(t->root);
    }

    t->root = n;
}

void add_new_child_tree(Node parent, Node child) {
    assert(parent != NULL);

    // Add child to array of parent's children
    if (parent->size >= parent->capacity) {
        parent->capacity *= 2;

        Node* realloced_children = (Node*) realloc(parent->children, sizeof(Node) * parent->capacity);
        if (realloced_children == NULL) {
            fprintf(stderr, "Unable to realloc children. Aborting...\n");
            exit(EXIT_FAILURE);
        }
        parent->children = realloced_children;
    }
    parent->children[parent->size++] = child;

    // Set child's parent pointer
    child->parent = parent;
}

inline Node get_root_tree(Tree tree) {
    assert(tree != NULL);
    return tree->root;
}

inline Node* get_children_tree(Node node) {
    assert(node != NULL);

    Node* children = node->children;

    return children;
}

inline uint32_t get_num_children_tree(Node node) {
    assert(node != NULL);

    int num_children = node->size;

    return num_children;
}

inline Node get_parent_tree(Node node) {
    assert(node != NULL);
    return node->parent;
}

inline Item get_node_value_tree(Node n) {
    assert(n != NULL);

    Item val = n->value;

    return val;
}

inline void write_lock_node_tree(Node node) {
    assert(node != NULL);
    pthread_rwlock_wrlock(&node->read_write_lock_node);
}

inline void read_lock_node_tree(Node node) {
    assert(node != NULL);
    pthread_rwlock_rdlock(&node->read_write_lock_node);
}

inline void unlock_node_tree(Node node) {
    assert(node != NULL);
    pthread_rwlock_unlock(&node->read_write_lock_node);
}
