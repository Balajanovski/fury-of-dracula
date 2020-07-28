//Implementation of K-tree
//ADT which accepts an arbitrary number of children per node
//Using resizing array provided by James Balajan
//By Finn Button

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#define DEFAULT_CAPACITY 10

struct kNode {
    int value;
    struct kNode *sibling;
    struct kNode *child;
    int size;
    int capacity;
    
};

struct kTree {
    struct kNode *root;
};

typedef struct kNode kNode;
typedef struct kTree kTree;

kNode *add_new_node(int);
kNode *add_new_sibling(kNode *, int);
kNode *add_child(kNode *, int);
void push_back_sibling_array(kNode *node, int value);
kTree *create_new_tree();
void freeTree (struct kTree *old);
void freeNode (struct kNode *old);



int main (void) {
    kTree *new = create_new_tree();
    new->root = add_new_node(5);
    add_child(new->root, 6);
    add_new_sibling(new->root->child, 7);
    add_new_sibling(new->root->child, 8);
    add_new_sibling(new->root->child, 9);
    add_new_sibling(new->root->child, 10);
    add_new_sibling(new->root->child, 11);
    add_new_sibling(new->root->child, 12);
    add_new_sibling(new->root->child, 13);
    add_new_sibling(new->root->child, 14);
    add_new_sibling(new->root->child, 15);
    add_new_sibling(new->root->child, 16);
    add_new_sibling(new->root->child, 17);
    add_new_sibling(new->root->child, 18);
    add_new_sibling(new->root->child, 19);
    add_new_sibling(new->root->child, 20);
    add_new_sibling(new->root->child, 21);
    add_new_sibling(new->root->child, 22);
    add_new_sibling(new->root->child, 23);
    add_new_sibling(new->root->child, 24);
    add_new_sibling(new->root->child, 25);
    
    printf("%d\n", new->root->child->value);
    printf("%d\n", new->root->value);
    printf("%d\n", new->root->child->sibling[13].value);
    
    freeTree(new);
    
    return 0;
}

kTree *create_new_tree() {
    kTree *new = malloc(sizeof(kTree));
    new->root = NULL;
    return new;

}

void freeNode (struct kNode *old) {
    assert(old != NULL);
    assert(old->sibling != NULL);
    free(old->sibling);
    free(old);
    return;
}

void freeTree (struct kTree *old) {
    assert (old != NULL);
    assert (old->root != NULL);
    freeNode(old->root);
    free(old);
    return;
}

kNode *add_new_node(int value) {
    kNode *new = malloc(sizeof(kNode));
    
    new->value = value;
    new->sibling = malloc(sizeof(kNode) * DEFAULT_CAPACITY);
    new->child = NULL;
    new->capacity = DEFAULT_CAPACITY;
    new->size = 0;
    
    return new;

}

kNode *add_new_sibling(kNode *k, int value) {
    assert (k != NULL);
    
    
    if (k->size >= k->capacity) {
        push_back_sibling_array(k, value);
        return &k->sibling[k->size];
    } else {
        k->sibling[k->size] = *add_new_node(value);
        k->size++;
        return &k->sibling[k->size-1];
    }
}

kNode *add_child(kNode *k, int value) {
    assert (k != NULL);
    
    if (k->child == NULL) {
        return (k->child = add_new_node(value));
    } else {
        return add_new_sibling(k->child, value);
    }
}

void push_back_sibling_array(kNode *node, int value) {
    if (node->size >= node->capacity) {
        node->capacity *= 2;
        node->sibling = realloc(node->sibling, sizeof(kNode) * node->capacity);
    }
    
    node->sibling[node->size++] = *add_new_node(value);
}

