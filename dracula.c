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

#include "dracula.h"
#include "DraculaView.h"
#include "Game.h"
#include "kTree.h"

#define NANO_SECOND_END_BUFFER 1.4e8
#define EXPLORATION_PARAMETER 1.41421356237f
#define NUMBER_OF_THREADS 5


/////////////////////
// Game state utility

typedef struct {
    DraculaView current_view;
    int wins;
    int visits;
    PlaceId move;
} GameState;

static void custom_game_state_free(void* value) {
    GameState* state = (GameState*) value;
    DvFree(state->current_view);
    free(state);
}

static Item create_game_state_item(DraculaView value, int wins, int visits, PlaceId move) {
    Item new_item;
    new_item.data = malloc(sizeof(GameState*));

    GameState* new_item_state = (GameState*) new_item.data;
    new_item_state->current_view = DvMakeCopy(value);
    new_item_state->wins = wins;
    new_item_state->visits = visits;
    new_item_state->move = move;
    new_item.custom_free = &custom_game_state_free;

    return new_item;
}

//////////////////////////////////
// Math helpers

static float compute_uct(int wins, int visits, int total_parent_node_simulations) {
    if (visits == 0) {
        return FLT_MAX;
    }

    return (float) wins / (float) visits + EXPLORATION_PARAMETER * sqrtf(2.f * logf((float) total_parent_node_simulations) / (float) visits);
}

//////////////////////////////////
// Monte carlo tree search helpers

static Node uct_select_child(Node node) {
    read_lock_node_tree(node);
    GameState* curr_node_game_state = (GameState*) get_node_value_tree(node).data;
    Node* node_children = get_children_tree(node);
    int num_children = get_num_children_tree(node);
    assert(num_children > 0);

    Node node_with_max_uct = node_children[0];
    float max_uct = -FLT_MAX;
    for (int i = 0; i < num_children; ++i) {
        read_lock_node_tree(node_children[i]);
        GameState* child_node_state = (GameState*) get_node_value_tree(node_children[i]).data;

        float uct = compute_uct(child_node_state->wins, child_node_state->visits, curr_node_game_state->visits);
        if (uct > max_uct) {
            node_with_max_uct = node_children[i];
            max_uct = uct;
        }
        unlock_node_tree(node_children[i]);
    }
    unlock_node_tree(node);

    return node_with_max_uct;
}

static Node select_promising_node(Tree mcts_tree) {
    Node curr_node = get_root_tree(mcts_tree); // Bug: Root is sometimes NULL?

    read_lock_node_tree(curr_node);
    int curr_node_children = get_num_children_tree(curr_node);
    while (curr_node_children > 0) {
        Node new_curr_node = uct_select_child(curr_node);

        unlock_node_tree(curr_node);
        read_lock_node_tree(new_curr_node);
        curr_node = new_curr_node;

        curr_node_children = get_num_children_tree(curr_node);
    }
    unlock_node_tree(curr_node);

    return curr_node;
}

static PlaceId get_move_location_id(char* full_move_string) {
    char* move_location_abbreviation = malloc(sizeof(char) * 3);
    strncpy(move_location_abbreviation, full_move_string + 1, 2);
    move_location_abbreviation[2] = '\0';
    PlaceId move_location_id = placeAbbrevToId(move_location_abbreviation);
    free(move_location_abbreviation);

    return move_location_id;
}

static Node* expand_node(Node node_to_expand, int* num_expanded_child_nodes) {
    write_lock_node_tree(node_to_expand);

    GameState* node_to_expand_game_state = (GameState*) get_node_value_tree(node_to_expand).data;
    GameCompletionState completion_state = DvGameState(node_to_expand_game_state->current_view);

    if (completion_state != GAME_NOT_OVER) {
        *num_expanded_child_nodes = 1;

        Node* children = malloc(sizeof(Node));
        children[0] = node_to_expand;

        return children;
    }

    int num_moves_returned = -1;
    char** move_buffer = DvComputePossibleMovesForPlayer(node_to_expand_game_state->current_view, &num_moves_returned);
    assert(num_moves_returned != -1);

    Node* new_nodes = malloc(sizeof(Node) * num_moves_returned);
    for (int move_index = 0; move_index < num_moves_returned; ++move_index) {
        DraculaView new_move_state = DvMakeCopy(node_to_expand_game_state->current_view);
        DvAdvanceStateByMoves(new_move_state, move_buffer[move_index]);

        PlaceId move_location_id = get_move_location_id(move_buffer[move_index]);

        Node expanded_child = create_new_node_tree(create_game_state_item(new_move_state, 0, 0, move_location_id));
        add_new_child_tree(node_to_expand, expanded_child);
        new_nodes[move_index] = expanded_child;

        free(move_buffer[move_index]);
    }
    free(move_buffer);

    *num_expanded_child_nodes = num_moves_returned;

    unlock_node_tree(node_to_expand);
    return new_nodes;
}

static GameCompletionState simulate_random_playout(Node node_to_explore, unsigned int* rand_generator_state) {
    DraculaView current_game_view = DvMakeCopy(((GameState*) get_node_value_tree(node_to_explore).data)->current_view);
    GameCompletionState game_completed_yet = DvGameState(current_game_view);

    while (game_completed_yet == GAME_NOT_OVER) {
        int num_moves_returned = -1;
        char** move_buffer = DvComputePossibleMovesForPlayer(current_game_view, &num_moves_returned);
        assert(num_moves_returned != -1);

        int random_move_index = rand_r(rand_generator_state) % num_moves_returned;
        char* random_move = move_buffer[random_move_index];
        DvAdvanceStateByMoves(current_game_view, random_move);

        for (int i = 0; i < num_moves_returned; ++i) {
            free(move_buffer[i]);
        }
        free(move_buffer);

        game_completed_yet = DvGameState(current_game_view);
    }

    return game_completed_yet;
}

static Node select_child_to_playout(Node* children, int num_children, unsigned int* rand_generator_state) {
    int rand_child_index = rand_r(rand_generator_state) % num_children;

    return children[rand_child_index];
}

static void backpropogate_state(Node played_out_node, GameCompletionState completion_state) {
    Node curr_node = played_out_node;

    while (curr_node != NULL) {
        write_lock_node_tree(curr_node);

        GameState* old_curr_node_game_state = (GameState*) get_node_value_tree(curr_node).data;
        Item new_game_state = create_game_state_item(old_curr_node_game_state->current_view, old_curr_node_game_state->wins + (completion_state == DRACULA_WINS), old_curr_node_game_state->visits + 1, old_curr_node_game_state->move);
        free(old_curr_node_game_state);

        set_node_value_tree(curr_node, new_game_state);

        unlock_node_tree(curr_node);

        curr_node = get_parent_tree(curr_node);
    }
}

static Node get_child_with_maximum_score(Node root) {
    float max_score = -FLT_MAX;
    Node max_child = NULL;

    int num_children = get_num_children_tree(root);
    Node* children = get_children_tree(root);

    for (int i = 0; i < num_children; ++i) {
        GameState *child_state = (GameState *) get_node_value_tree(children[i]).data;
        float curr_child_score = ((float) child_state->wins / (float) child_state->visits);

        if (max_score < curr_child_score) {
            if (max_score < curr_child_score) {
                max_score = curr_child_score;
                max_child = children[i];
            }
        }
    }

    assert(max_child != NULL);
    return max_child;
}

////////////
// Actual AI

#ifndef NDEBUG
int num_iter = 0;
#endif

static inline bool can_continue_running(struct timespec start_time, struct timespec curr_time) {
    return (curr_time.tv_sec * 1e9 + curr_time.tv_nsec) - (start_time.tv_sec * 1e9 + start_time.tv_nsec) < (TURN_LIMIT_MSECS * 1e6) - NANO_SECOND_END_BUFFER;
}

void* run_simulations(void* mcts_tree) {
    mcts_tree = (Tree) mcts_tree;
    unsigned int rand_generator_state = time(NULL) ^ getpid() ^ pthread_self();
    struct timespec start_time, curr_time;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);
    clock_gettime(CLOCK_MONOTONIC_RAW, &curr_time);
    while (can_continue_running(start_time, curr_time)) {
#ifndef NDEBUG
        __atomic_fetch_add(&num_iter, 1, __ATOMIC_SEQ_CST);
#endif

        Node promising_node = select_promising_node(mcts_tree);

        int num_expanded_child_nodes = -1;
        Node* expanded_child_nodes = expand_node(promising_node, &num_expanded_child_nodes);
        assert(num_expanded_child_nodes != -1);

        clock_gettime(CLOCK_MONOTONIC_RAW, &curr_time);
        if (!can_continue_running(start_time, curr_time)) {
            break;
        }

        Node node_to_explore = select_child_to_playout(expanded_child_nodes, num_expanded_child_nodes, &rand_generator_state);
        GameCompletionState playout_result = simulate_random_playout(node_to_explore, &rand_generator_state);

        backpropogate_state(node_to_explore, playout_result);

        clock_gettime(CLOCK_MONOTONIC_RAW, &curr_time);
    }

    return NULL;
}

void decideDraculaMove(DraculaView dv) {
    //Round current_round = DvGetRound(dv);
    PlaceId best_move;
    Tree mcts_tree = create_new_tree();
    set_root_tree(mcts_tree, create_new_node_tree(create_game_state_item(dv, 0, 0, NOWHERE)));

    // Create MCTS simulation threads
    pthread_t thread_ids[NUMBER_OF_THREADS-1];
    for (int i = 0; i < NUMBER_OF_THREADS-1; ++i) {
        pthread_create(&thread_ids[i], NULL, run_simulations, (void*) mcts_tree);
    }

    // Allow the main thread to also participate in the simulations as opposed to idling
    run_simulations((void*) mcts_tree);

    // Join the simulation threads to the main thread's execution on their completion
    for (int i = 0; i < NUMBER_OF_THREADS-1; ++i) {
        pthread_join(thread_ids[i], NULL);
    }

    Node best_node = get_child_with_maximum_score(get_root_tree(mcts_tree));
    GameState* best_node_state = (GameState*) get_node_value_tree(best_node).data;
    best_move = best_node_state->move;

    free(mcts_tree);

#ifndef NDEBUG
    printf("Num iter: %d\n", num_iter);
#endif

	registerBestPlay((char *)placeIdToAbbrev(best_move), "Mwahaha, you cannot defeat Count Monte Carlo!");
}
