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

#define NANO_SECOND_END_BUFFER 1.05e8
#define EXPLORATION_PARAMETER 1.45f
#define NUMBER_OF_THREADS 5
#define MAX_PLAY_DEPTH 100000

#define MIN(a,b) (((a)<(b))?(a):(b))


/////////////////////
// Game state utility

typedef struct {
    DraculaView current_view;
    int aggregate_dracula_score; // Dracula score is defined as start score - current gamestate score
    int num_games;
    bool min_dist_cache_filled;
    float min_dist_from_hunter;
    PlaceId move;
} GameState;

static void custom_game_state_free(void* value) {
    GameState* state = (GameState*) value;
    DvFree(state->current_view);
    free(state);
}

static Item create_game_state_item(DraculaView view, int aggregate_dracula_score, int num_games, PlaceId move, bool copy_view) {
    Item new_item;
    new_item.data = malloc(sizeof(GameState));

    GameState* new_item_state = (GameState*) new_item.data;
    new_item_state->current_view = copy_view ? DvMakeCopy(view) : view;
    new_item_state->aggregate_dracula_score = aggregate_dracula_score;
    new_item_state->num_games = num_games;
    new_item_state->move = move;
    new_item_state->min_dist_cache_filled = false;
    new_item.custom_free = &custom_game_state_free;

    return new_item;
}

//////////////////////////////////
// Math helpers

static float compute_uct(int aggregate_dracula_score, int num_games, int total_parent_node_simulations) {
    if (num_games == 0) {
        return FLT_MAX;
    }

    return ((float) aggregate_dracula_score / (float) num_games) + EXPLORATION_PARAMETER * sqrtf(2.f * logf((float) total_parent_node_simulations) / (float) num_games);
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

        float uct = compute_uct(child_node_state->aggregate_dracula_score, child_node_state->num_games, curr_node_game_state->num_games);
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
    Node curr_node = get_root_tree(mcts_tree);

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
    char *move_location_abbreviation = malloc(sizeof(char) * 3);
    strncpy(move_location_abbreviation, full_move_string + 1, 2);
    move_location_abbreviation[2] = '\0';
    PlaceId move_location_id = placeAbbrevToId(move_location_abbreviation);
    free(move_location_abbreviation);

    return move_location_id;
}

static void expand_node(Node node_to_expand, unsigned int* rand_generator_state) {
    write_lock_node_tree(node_to_expand);

    // Handle case for if two threads come to expanding the same node at the same time
    // only take the first one's results
    int num_current_node_to_expand_children = get_num_children_tree(node_to_expand);
    if (num_current_node_to_expand_children <= 0) {
        GameState* node_to_expand_game_state = (GameState*) get_node_value_tree(node_to_expand).data;
        GameCompletionState completion_state = DvGameState(node_to_expand_game_state->current_view);

        if (completion_state == GAME_NOT_OVER) {
            int num_moves_returned = -1;
            char** move_buffer = DvComputePossibleMovesForPlayer(node_to_expand_game_state->current_view, &num_moves_returned);
            assert(num_moves_returned != -1);

            int rand_move_index = rand_r(rand_generator_state) % num_moves_returned;
            DraculaView new_move_state = DvMakeCopy(node_to_expand_game_state->current_view);
            DvAdvanceStateByMoves(new_move_state, move_buffer[rand_move_index]);

            PlaceId move_location_id = get_move_location_id(move_buffer[rand_move_index]);

            Item expanded_game_state_item = create_game_state_item(new_move_state, 0, 0, move_location_id, false);

            Node expanded_child = create_new_node_tree(expanded_game_state_item);
            add_new_child_tree(node_to_expand, expanded_child);

            for (int move_index = 0; move_index < num_moves_returned; ++move_index) {
                free(move_buffer[move_index]);
            }
            free(move_buffer);
        }
    }

    unlock_node_tree(node_to_expand);
}

static int simulate_random_playout(Node node_to_explore, unsigned int* rand_generator_state) {
    read_lock_node_tree(node_to_explore);
    DraculaView current_game_view = DvMakeCopy(((GameState*) get_node_value_tree(node_to_explore).data)->current_view);
    unlock_node_tree(node_to_explore);
    GameCompletionState game_completed_yet = DvGameState(current_game_view);

    int playout_depth = 0;
    while (game_completed_yet == GAME_NOT_OVER && playout_depth < MAX_PLAY_DEPTH) {
        int num_moves_returned = -1;
        char** move_buffer = DvComputePossibleMovesForPlayer(current_game_view, &num_moves_returned);
        assert(num_moves_returned != -1);

        char* random_move = move_buffer[rand_r(rand_generator_state) % num_moves_returned];

        DvAdvanceStateByMoves(current_game_view, random_move);
        ++playout_depth;

        for (int i = 0; i < num_moves_returned; ++i) {
            free(move_buffer[i]);
        }
        free(move_buffer);

        game_completed_yet = DvGameState(current_game_view);
    }

    int current_game_view_score = DvGetScore(current_game_view);
    DvFree(current_game_view);

    return GAME_START_SCORE - current_game_view_score;
}

static Node select_child_to_playout(Node* children, int num_children, unsigned int* rand_generator_state) {
    int rand_child_index = rand_r(rand_generator_state) % num_children;

    return children[rand_child_index];
}

static void backpropogate_state(Node played_out_node, int dracula_score) {
    Node curr_node = played_out_node;

    while (curr_node != NULL) {
        write_lock_node_tree(curr_node);

        GameState* old_curr_node_game_state = (GameState*) get_node_value_tree(curr_node).data;
        Item new_game_state = create_game_state_item(old_curr_node_game_state->current_view, old_curr_node_game_state->aggregate_dracula_score + dracula_score, old_curr_node_game_state->num_games + 1, old_curr_node_game_state->move, false);
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
        float curr_child_score = ((float) child_state->aggregate_dracula_score / (float) child_state->num_games);

        if (max_score < curr_child_score) {
            if (max_score < curr_child_score) {
                max_score = curr_child_score;
                max_child = children[i];
            }
        }
    }

#ifndef NDEBUG
    printf("Avg dracula score for best move: %f\n", max_score);
#endif

    assert(max_child != NULL);
    return max_child;
}

///////////////
// Tree merging

Node merge_recursively(Node lhs_n, Node rhs_n) {
    if (lhs_n == NULL) {
        return rhs_n;
    } else if (rhs_n == NULL) {
        return lhs_n;
    } else {
        GameState* lhs_data = (GameState*) get_node_value_tree(lhs_n).data;
        GameState* rhs_data = (GameState*) get_node_value_tree(rhs_n).data;

        Node* lhs_children = get_children_tree(lhs_n);
        Node* rhs_children = get_children_tree(rhs_n);
        int num_lhs_children = get_num_children_tree(lhs_n);
        int num_rhs_children = get_num_children_tree(rhs_n);

        Item new_node_data = create_game_state_item(
                lhs_data->current_view,
                lhs_data->aggregate_dracula_score + rhs_data->aggregate_dracula_score,
                lhs_data->num_games + rhs_data->num_games,
                lhs_data->move,
                true);
        Node new_node = create_new_node_tree(new_node_data);

        int lhs_i = 0;
        int rhs_i = 0;
        while (lhs_i < num_lhs_children && rhs_i < num_rhs_children) {
            GameState* lhs_child_data = (GameState*) get_node_value_tree(lhs_children[lhs_i]).data;
            GameState* rhs_child_data = (GameState*) get_node_value_tree(rhs_children[rhs_i]).data;

            if (lhs_child_data->move == rhs_child_data->move) {
                add_new_child_tree(new_node, merge_recursively(lhs_children[lhs_i], rhs_children[rhs_i]));
                ++lhs_i; ++rhs_i;
            } else if (lhs_child_data->move < rhs_child_data->move) {
                add_new_child_tree(new_node, merge_recursively(lhs_children[lhs_i], NULL));
                ++lhs_i;
            } else {
                add_new_child_tree(new_node, merge_recursively(NULL, rhs_children[rhs_i]));
                ++rhs_i;
            }
        }

        while (lhs_i < num_lhs_children) {
            add_new_child_tree(new_node, merge_recursively(lhs_children[lhs_i], NULL));
            ++lhs_i;
        }

        while (rhs_i < num_rhs_children) {
            add_new_child_tree(new_node, merge_recursively(NULL, rhs_children[rhs_i]));
            ++rhs_i;
        }

        free_tree_node(lhs_n);
        free_tree_node(rhs_n);

        return new_node;
    }
}

Tree merge_trees(Tree lhs, Tree rhs) {
    Node root = merge_recursively(get_root_tree(lhs), get_root_tree(rhs));
    Tree new_tree = create_new_tree();
    set_root_tree(new_tree, root);

    free(lhs);
    free(rhs);

    return new_tree;
}

////////////
// Actual AI

#ifndef NDEBUG
int num_iter = 0;
#endif

static inline bool can_continue_running(struct timespec start_time, struct timespec curr_time) {
    return (curr_time.tv_sec * 1e9 + curr_time.tv_nsec) - (start_time.tv_sec * 1e9 + start_time.tv_nsec) < (TURN_LIMIT_MSECS * 1e6) - NANO_SECOND_END_BUFFER;
}

void* run_simulations(void* dv) {
    Tree mcts_tree = create_new_tree();
    set_root_tree(mcts_tree, create_new_node_tree(create_game_state_item(dv, 0, 0, NOWHERE, true)));

    unsigned int rand_generator_state = (long) time(NULL) ^ (long) getpid() ^ (long) pthread_self();

    struct timespec start_time, curr_time;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);
    clock_gettime(CLOCK_MONOTONIC_RAW, &curr_time);
    while (can_continue_running(start_time, curr_time)) {
#ifndef NDEBUG
        __atomic_fetch_add(&num_iter, 1, __ATOMIC_SEQ_CST);
#endif

        Node promising_node = select_promising_node(mcts_tree);

        expand_node(promising_node, &rand_generator_state);

        read_lock_node_tree(promising_node);
        Node* expanded_child_nodes = get_children_tree(promising_node);
        int num_expanded_child_nodes = get_num_children_tree(promising_node);
        unlock_node_tree(promising_node);

        clock_gettime(CLOCK_MONOTONIC_RAW, &curr_time);
        if (!can_continue_running(start_time, curr_time)) {
            break;
        }

        if (num_expanded_child_nodes <= 0) {
            continue;
        }

        Node node_to_explore = select_child_to_playout(expanded_child_nodes, num_expanded_child_nodes, &rand_generator_state);
        GameCompletionState playout_result = simulate_random_playout(node_to_explore, &rand_generator_state);

        backpropogate_state(node_to_explore, playout_result);

        clock_gettime(CLOCK_MONOTONIC_RAW, &curr_time);
    }

    pthread_exit(mcts_tree);
}

void decideDraculaMove(DraculaView dv) {
    Player current_player = DvGetPlayer(dv);
    if (current_player != PLAYER_DRACULA) {
        fprintf(stderr, "Invalid past plays string provided to initial dracula view, where the person to make a move is not dracula. Aborting...\n");
        exit(EXIT_FAILURE);
    }


    // Create MCTS simulation threads
    Tree trees[NUMBER_OF_THREADS];
    pthread_t thread_ids[NUMBER_OF_THREADS];
    for (int i = 0; i < NUMBER_OF_THREADS; ++i) {
        pthread_create(&thread_ids[i], NULL, run_simulations, (void*) dv);
    }

    // Join the simulation threads to the main thread's execution on their completion
    for (int i = 0; i < NUMBER_OF_THREADS; ++i) {
        pthread_join(thread_ids[i], (void**) &trees[i]);
    }

    for (int i = 1; i < NUMBER_OF_THREADS; ++i) {
        trees[0] = merge_trees(trees[0], trees[i]);
    }

    Node best_node = get_child_with_maximum_score(get_root_tree(trees[0]));
    GameState* best_node_state = (GameState*) get_node_value_tree(best_node).data;
    PlaceId best_move = best_node_state->move;

#ifndef NDEBUG
    printf("Num iter: %d\n", num_iter);
#endif

    free_tree(trees[0]);

    registerBestPlay(placeIdToAbbrev(best_move), "Mwahaha, you cannot defeat Count Monte Carlo!");
}