//interface for kTree ADT
//By Finn Button

#ifndef KTREE_H
#define KTREE_H

// DEFINE NODE CONTENTS BEFORE IMPORTING TO SET WHAT YOU WANT TO STORE IN THE TREE
#ifndef KTREE_NODE_CONTENTS
#define KTREE_NODE_CONTENTS int
#endif

typedef struct kNode *Node;
typedef struct kTree *Tree;

Tree create_new_tree(void);

void free_tree(Tree old);
void free_tree_node(Node old);

Node create_new_node_tree(KTREE_NODE_CONTENTS value);

KTREE_NODE_CONTENTS get_node_value_tree(Node n);

void add_new_child_tree(Node parent, Node child);
void set_root_tree(Tree t, Node n);

Node get_root_tree(Tree tree);

Node* get_children_tree(Node node);
int get_num_children_tree(Node node);


#endif
