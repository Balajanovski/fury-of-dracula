//interface for kTree ADT
//By Finn Button

#ifndef KTREE_H
#define KTREE_H



typedef struct kNode *Node;
typedef struct kTree *Tree;


// create a new tree
kTree NewQueue create_new_tree(void);

// free memory of node
void freeNode (kNode );

// free memory of tree struct
void freeTree (kTree);

// add a new sibling
kNode add_new_sibling(kNode *k, int value);

//add a new child
kNode add_new_child(kNode *k, int value);


#endif
