//interface for kTree ADT
//By Finn Button

#ifndef KTREE_H
#define KTREE_H

typedef struct Item {
    void* data;
    void (*custom_free)(void *);
} Item;

typedef struct baseNode *Node;
typedef struct kTree *Tree;

Tree create_new_tree(void);

void free_tree(Tree old);
void free_tree_node(Node old);

Node create_new_node_tree(Item value);
void set_node_value_tree(Node n, Item value);

Item get_node_value_tree(Node n);

void add_new_child_tree(Node parent, Node child);
void set_root_tree(Tree t, Node n);

Node get_root_tree(Tree tree);

Node* get_children_tree(Node node);
int get_num_children_tree(Node node);

Node get_parent_tree(Node node);

void write_lock_node_tree(Node node);
void read_lock_node_tree(Node node);
void unlock_node_tree(Node node);


#endif
