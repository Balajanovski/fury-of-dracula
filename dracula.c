//
// Created by james on 30/7/20.
//

#include <time.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>

#include "dracula.h"
#include "DraculaView.h"
#include "Game.h"
#include "kTree.h"
#include "Queue.h"
#include "MoveSet.h"

#define SERIALIZED_FILE_NAME "serialized_tree"
#define MIN(a,b) (((a)<(b))?(a):(b))


/////////////////////
// Game state utility

typedef struct {
    float dracula_score_ratio;
    PlaceId move;
} GameState;

static void custom_game_state_free(void* value) {
    GameState* state = (GameState*) value;
    free(state);
}

static Item create_game_state_item(float dracula_score_ratio, PlaceId move) {
    Item new_item;
    new_item.data = malloc(sizeof(GameState));

    GameState* new_item_state = (GameState*) new_item.data;
    new_item_state->move = move;
    new_item.custom_free = &custom_game_state_free;

    return new_item;
}

///////////////////
// Deserialization

static inline bool read_32_bits_from_file(FILE* fp, uint32_t *bits) {
    uint32_t bitset = 0x0;

    int shift_amount = 24;
    for (int i = 0; i < 4; ++i) {
        int read = fgetc(fp);
        if (read == EOF) {
            return true;
        }
        bitset |= ((uint32_t) read & 0xFF) << shift_amount;

        shift_amount -= 8;
    }

    *bits = bitset;
    return false;
}

typedef union {
    float f;
    uint32_t bits;
} MungedFloat;

static Node recursively_deserialize_tree(FILE* fp) {
    // Read number of children
    uint32_t bits;
    bool eof = read_32_bits_from_file(fp, &bits);
    if (eof) {
        return NULL;
    }
    uint32_t num_children = bits;

    // Read win rate of current node
    eof = read_32_bits_from_file(fp, &bits);
    assert(!eof);
    MungedFloat win_rate = {.bits = bits};

    // Read move of current node
    eof = read_32_bits_from_file(fp, &bits);
    assert(!eof);
    PlaceId move = (PlaceId) bits;

    // Recurse on children
    Item current_node_item = create_game_state_item(win_rate.f, move);
    Node current_node = create_new_node_tree(current_node_item);
    for (int i = 0; i < num_children; ++i) {
        Node child = recursively_deserialize_tree(fp);
        if (child != NULL) {
            add_new_child_tree(current_node, child);
        }
    }

    return current_node;
}

static Tree deserialize_tree(FILE* fp) {
    Tree t = create_new_tree();
    Node root = recursively_deserialize_tree(fp);
    set_root_tree(t, root);

    return t;
}

////////////
// Actual AI

static Node get_current_state_node(Node curr_node, int move_idx, const PlaceId* loc_hist, int num_locs) {
    if (move_idx >= num_locs) {
        return curr_node;
    }

    PlaceId next_place = loc_hist[move_idx];
    int num_children = get_num_children_tree(curr_node);
    Node* children = get_children_tree(curr_node);

    int low = 0; int high = num_children-1;
    int found_idx = -1;
    while (low <= high) {
        int mid = low + ((high - low) >> 1);
        GameState* mid_state = get_node_value_tree(children[mid]).data;

        if (mid_state->move == next_place) {
            found_idx = mid;
            break;;
        } else if (mid_state->move < next_place) {
            low = mid+1;
        } else {
            high = mid-1;
        }
    }

    if (found_idx == -1) {
        return NULL;
    } else {
        return get_current_state_node(children[found_idx], move_idx+1, loc_hist, num_locs);
    }
}

static Node get_child_with_maximum_score(Node root) {
    float max_score = -FLT_MAX;
    Node max_child = NULL;

    int num_children = get_num_children_tree(root);
    Node* children = get_children_tree(root);

    for (int i = 0; i < num_children; ++i) {
        GameState *child_state = (GameState *) get_node_value_tree(children[i]).data;
        float curr_child_score = child_state->dracula_score_ratio;

        if (max_score < curr_child_score) {
            if (max_score < curr_child_score) {
                max_score = curr_child_score;
                max_child = children[i];
            }
        }
    }

#ifndef NDEBUG
    printf("Score of best move: %f\n", max_score);
#endif

    return max_child;
}

static PlaceId randomly_pick_next_move(DraculaView dv) {
    int num_returned_moves;
    PlaceId* moves = DvGetValidMoves(dv, &num_returned_moves);
    unsigned int rand_generator_state = (long) time(NULL) ^ (long) getpid() ^ (long) pthread_self();

    PlaceId move = moves[rand_r(&rand_generator_state) % num_returned_moves];
    free(moves);

#ifndef NDEBUG
    printf("Choosing random move\n");
#endif

    return move;
}

void decideDraculaMove(DraculaView dv) {
    FILE* serialized_tree = fopen(SERIALIZED_FILE_NAME, "rb");
    if (serialized_tree == NULL) {
        fprintf(stderr, "Could not open serialized tree file. Aborting...\n");
        exit(EXIT_FAILURE);
    }

    Tree mcts_tree = deserialize_tree(serialized_tree);
    int num_locations;
    const PlaceId* chronological_loc_hist = DvGetChronologicalLocationHistory(dv, &num_locations);

    Node current_state_node = get_current_state_node(current_state_node, 0, chronological_loc_hist, num_locations);
    PlaceId best_move;
    if (current_state_node != NULL) {
        Node best_next_move_node = get_child_with_maximum_score(current_state_node);
        if (best_next_move_node == NULL) {
            best_move = randomly_pick_next_move(dv);
        } else {
            GameState* best_state = get_node_value_tree(best_next_move_node).data;
            best_move = best_state->move;
        }
    } else {
        best_move = randomly_pick_next_move(dv);
    }

    free_tree(mcts_tree);

    registerBestPlay(placeIdToAbbrev(best_move), "Mwahaha! You cannot defeat precomputed Count Monte Carlo!");
}