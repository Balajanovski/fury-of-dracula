//
// Created by james on 28/7/20.
//

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "kTree.h"


static void dfs_in_tree(Node curr_node, int *dfs_array, int *insert_index) {
    if (curr_node == NULL) {
        return;
    }

    dfs_array[(*insert_index)++] = *((int*) get_node_value_tree(curr_node).data);
    Node *children = get_children_tree(curr_node);
    int num_children = get_num_children_tree(curr_node);

    for (int i = 0; i < num_children; ++i) {
        dfs_in_tree(children[i], dfs_array, insert_index);
    }
}

void custom_free(void* data) {
    printf("I'M FREE!\n");
    free(data);
}

Item create_new_int_item(int value) {
    Item new_item;
    new_item.data = malloc(sizeof(int));
    *((int*) new_item.data) = value;
    new_item.custom_free = &custom_free;

    return new_item;
}

int main (void) {
    {
        printf("Testing tree creation and node insertion\n");

        Tree new = create_new_tree();
        Node n1 = create_new_node_tree(create_new_int_item(1));
        Node n2 = create_new_node_tree(create_new_int_item(2));
        Node n3 = create_new_node_tree(create_new_int_item(3));
        Node n4 = create_new_node_tree(create_new_int_item(4));

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
