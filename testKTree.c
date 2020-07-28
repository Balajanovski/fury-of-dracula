//
// Created by james on 28/7/20.
//

#include <stdio.h>
#include <assert.h>

#define KTREE_NODE_CONTENTS int
#include "kTree.h"


static void dfs_in_tree(Node curr_node, int *dfs_array, int *insert_index) {
    if (curr_node == NULL) {
        return;
    }

    dfs_array[(*insert_index)++] = get_node_value_tree(curr_node);
    Node *children = get_children_tree(curr_node);
    int num_children = get_num_children_tree(curr_node);

    for (int i = 0; i < num_children; ++i) {
        dfs_in_tree(children[i], dfs_array, insert_index);
    }
}

int main (void) {
    {
        printf("Testing tree creation and node insertion\n");

        Tree new = create_new_tree();
        Node n1 = create_new_node_tree(1);
        Node n2 = create_new_node_tree(2);
        Node n3 = create_new_node_tree(3);
        Node n4 = create_new_node_tree(4);

        set_root_tree(new, n1);
        add_new_child_tree(n1, n2);
        add_new_child_tree(n1, n3);
        add_new_child_tree(n2, n4);

        int dfs_nodes[4];
        int expected_dfs_nodes[4] = {1, 2, 4, 3};

        int insert_index = 0;
        dfs_in_tree(n1, dfs_nodes, &insert_index);

        for (int i = 0; i < 4; ++i) {
            assert(dfs_nodes[i] == expected_dfs_nodes[i]);
        }

        free_tree(new);

        printf("Test passed!\n");
    }


    return 0;
}
