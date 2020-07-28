//Implementation of K-tree
//ADT which accepts an arbitrary number of children per node
//Using resizing array provided by James Balajan
//By Finn Button

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "kTree.h"

#define DEFAULT_CAPACITY 10

struct baseNode {
    Item value;
    struct baseNode **children;
    int size;
    int capacity;
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

    new->capacity = DEFAULT_CAPACITY;
    new->size = 0;
    
    return new;
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
}

inline Node get_root_tree(Tree tree) {
    assert(tree != NULL);
    return tree->root;
}

inline Node* get_children_tree(Node node) {
    assert(node != NULL);
    return node->children;
}

inline int get_num_children_tree(Node node) {
    assert(node != NULL);
    return node->size;
}

inline Item get_node_value_tree(Node n) {
    assert(n != NULL);
    return n->value;
}
