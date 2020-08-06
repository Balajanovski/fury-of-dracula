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

#define NANO_SECOND_END_BUFFER 1.4e8
#define EXPLORATION_PARAMETER 1.41421356237f
#define NUMBER_OF_THREADS 1
#define HEURISTIC_RATIO_THRESHOLD 0.90f

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

    return (float) aggregate_dracula_score / (float) num_games + EXPLORATION_PARAMETER * sqrtf(2.f * logf((float) total_parent_node_simulations) / (float) num_games);
}

//////////////////////////////////
// Heuristics

static float min_distance_from_hunter_view(DraculaView dv) {
    PlaceId dracula_loc = DvGetPlayerLocation(dv, PLAYER_DRACULA);

    int min_distance = INT_MAX;
    for (Player hunter = 0; hunter < PLAYER_DRACULA; ++hunter) {
        Round original_round = DvGetRound(dv);

        Queue bfs_queue = NewQueue();
        AddtoQueue(bfs_queue, DvGetPlayerLocation(dv, hunter));
        MoveSet visited_set = new_move_set();

        int current_hunter_to_dracula_distance_in_rounds = 0;
        bool found_dracula = false;
        while (QueueSize(bfs_queue) > 0 && !found_dracula) {
            int q_size = QueueSize(bfs_queue);

            for (int i = 0; i < q_size; ++i) {
                PlaceId current_pos = RemovefromQueue(bfs_queue);
                if (current_pos == dracula_loc) {
                    found_dracula = true;
                    break;
                }
                if (is_move_in_set(visited_set, current_pos)) {
                    continue;
                }
                insert_move_set(visited_set, current_pos);

                int num_returned_places;
                PlaceId* places = DvWhereCanTheyGoByTypeFromLocationAndRound(dv, hunter, original_round + current_hunter_to_dracula_distance_in_rounds + 1, current_pos, true, true, true, &num_returned_places);
                for (int place_index = 0; place_index < num_returned_places; ++place_index) {
                    AddtoQueue(bfs_queue, places[place_index]);
                }
                free(places);
            }

            ++current_hunter_to_dracula_distance_in_rounds;
        }

        free_move_set(visited_set);

        min_distance = MIN(min_distance, current_hunter_to_dracula_distance_in_rounds);
        FreeQueue(bfs_queue);
    }

    return (float) min_distance;
}

// Min distance from a hunter in moves
static float min_distance_from_hunter(GameState* state) {
    // If result cached simply return
    if (state->min_dist_cache_filled) {
        return state->min_dist_from_hunter;
    }

    DraculaView dv = state->current_view;
    float min_distance = min_distance_from_hunter_view(dv);

    // Update cache
    state->min_dist_from_hunter = min_distance;
    state->min_dist_cache_filled = true;

    return min_distance;
}

static inline float tree_culling_heuristic(Player parent_player, float min_dist_from_hunter) {
    float heuristic_score = (min_dist_from_hunter + 1.0f);
    if (parent_player != PLAYER_DRACULA) {
        heuristic_score = 1.0f / heuristic_score;
    }

    return heuristic_score;
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

static void expand_node(Node node_to_expand) {
    write_lock_node_tree(node_to_expand);

    // Handle case for if two threads come to expanding the same node at the same time
    // only take the first one's results
    int num_current_node_to_expand_children = get_num_children_tree(node_to_expand);
    if (num_current_node_to_expand_children <= 0) {
        GameState* node_to_expand_game_state = (GameState*) get_node_value_tree(node_to_expand).data;
        GameCompletionState completion_state = DvGameState(node_to_expand_game_state->current_view);
        Player parent_player = DvGetPlayer(node_to_expand_game_state->current_view);

        float parent_min_dist_from_hunter = min_distance_from_hunter(node_to_expand_game_state);
        float parent_heuristic_score = tree_culling_heuristic(parent_player, parent_min_dist_from_hunter);

        if (completion_state == GAME_NOT_OVER) {
            int num_moves_returned = -1;
            char** move_buffer = DvComputePossibleMovesForPlayer(node_to_expand_game_state->current_view, &num_moves_returned);
            assert(num_moves_returned != -1);

            for (int move_index = 0; move_index < num_moves_returned; ++move_index) {
                DraculaView new_move_state = DvMakeCopy(node_to_expand_game_state->current_view);
                DvAdvanceStateByMoves(new_move_state, move_buffer[move_index]);

                PlaceId move_location_id = get_move_location_id(move_buffer[move_index]);

                Item expanded_game_state_item = create_game_state_item(new_move_state, 0, 0, move_location_id, false);
                GameState* expanded_node_state = (GameState*) expanded_game_state_item.data;

                float expanded_min_dist_from_hunter = min_distance_from_hunter(expanded_node_state);
                float expanded_heuristic_score = tree_culling_heuristic(parent_player, expanded_min_dist_from_hunter);

                if (expanded_heuristic_score < parent_heuristic_score * HEURISTIC_RATIO_THRESHOLD) {
                    custom_game_state_free(expanded_node_state);
                } else {
                    Node expanded_child = create_new_node_tree(expanded_game_state_item);
                    add_new_child_tree(node_to_expand, expanded_child);
                }

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
    while (game_completed_yet == GAME_NOT_OVER) {
        int num_moves_returned = -1;
        Player current_player = DvGetPlayer(current_game_view);
        char** move_buffer = DvComputePossibleMovesForPlayer(current_game_view, &num_moves_returned);
        assert(num_moves_returned != -1);

        // Weighted random move selection based on heuristic
        float aggregate_heuristic_scores = 0.0f;
        float* heuristic_scores_per_move = (float*) malloc(sizeof(float) * num_moves_returned);
        for (int move = 0; move < num_moves_returned; ++move) {
            DraculaView game_view_for_move = DvMakeCopy(current_game_view);
            DvAdvanceStateByMoves(game_view_for_move, move_buffer[move]);

            heuristic_scores_per_move[move] = tree_culling_heuristic(current_player, min_distance_from_hunter_view(game_view_for_move));

            DvFree(game_view_for_move);
        }
        float random_heuristic_score_index = (float) rand_r(rand_generator_state)/((float) RAND_MAX / aggregate_heuristic_scores);
        char* random_move = move_buffer[0];
        for (int move = 0; move < num_moves_returned; ++move) {
            if (heuristic_scores_per_move[move] > random_heuristic_score_index) {
                random_move = move_buffer[move];
                break;
            }
        }

        DvAdvanceStateByMoves(current_game_view, random_move);
        ++playout_depth;

        for (int i = 0; i < num_moves_returned; ++i) {
            free(move_buffer[i]);
        }
        free(move_buffer);
        free(heuristic_scores_per_move);

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

    unsigned int rand_generator_state = (long) time(NULL) ^ (long) getpid() ^ (long) pthread_self();

    struct timespec start_time, curr_time;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);
    clock_gettime(CLOCK_MONOTONIC_RAW, &curr_time);
    while (can_continue_running(start_time, curr_time)) {
#ifndef NDEBUG
        __atomic_fetch_add(&num_iter, 1, __ATOMIC_SEQ_CST);
#endif

        Node promising_node = select_promising_node(mcts_tree);

        expand_node(promising_node);

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

    return NULL;
}

PlaceId get_best_starting_location(DraculaView dv, Tree mcts_tree) {
    Node root_node = get_root_tree(mcts_tree);
    unsigned int rand_generator_state = (long) time(NULL) ^ (long) getpid() ^ (long) pthread_self();

    expand_node(root_node);
    int num_child_nodes = get_num_children_tree(root_node);
    Node* child_nodes = get_children_tree(root_node);

    int best_place_index = rand_r(&rand_generator_state) % num_child_nodes;
    float best_place_min_dist = -FLT_MAX;
    PlaceId* possible_places = (PlaceId*) malloc(sizeof(PlaceId) * num_child_nodes);

    for (int i = 0; i < num_child_nodes; ++i) {
        GameState* child_state = (GameState*) get_node_value_tree(child_nodes[i]).data;
        possible_places[i] = child_state->move;
    }

    for (int i = 0; i < num_child_nodes; ++i) {
        PlaceId curr_place = possible_places[i];

        if (!placeIsReal(curr_place) || placeIsSea(curr_place)) {
            continue;
        }

        float curr_min_dist = min_distance_from_hunter(get_node_value_tree(child_nodes[i]).data);
        if (curr_min_dist > best_place_min_dist) {
            best_place_index = i;
            best_place_min_dist = curr_min_dist;
        }
    }

    PlaceId best_starting_loc = possible_places[best_place_index];
    free(possible_places);

    return best_starting_loc;
}

void decideDraculaMove(DraculaView dv) {
    Player current_player = DvGetPlayer(dv);
    if (current_player != PLAYER_DRACULA) {
        fprintf(stderr, "Invalid past plays string provided to initial dracula view, where the person to make a move is not dracula. Aborting...\n");
        exit(EXIT_FAILURE);
    }

    PlaceId best_move;
    Round current_round = DvGetRound(dv);
    Tree mcts_tree = create_new_tree();
    set_root_tree(mcts_tree, create_new_node_tree(create_game_state_item(dv, 0, 0, NOWHERE, true)));

    if (current_round > 0) {
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

#ifndef NDEBUG
        printf("Num iter: %d\n", num_iter);
#endif
    } else {
        best_move = get_best_starting_location(dv, mcts_tree);
    }

    free_tree(mcts_tree);

	registerBestPlay(placeIdToAbbrev(best_move), "Mwahaha, you cannot defeat Count Monte Carlo!");
}