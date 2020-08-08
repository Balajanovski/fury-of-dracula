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
#include <signal.h>

#include "dracula.h"
#include "DraculaView.h"
#include "Game.h"
#include "kTree.h"
#include "Queue.h"
#include "MoveSet.h"

#define EXPLORATION_PARAMETER 1.41421356237f
#define NUMBER_OF_THREADS 1
#define SERIALIZED_FILE_NAME "serialized_tree"

#define MIN(a,b) (((a)<(b))?(a):(b))


/////////////////////
// Game state utility

typedef struct {
    DraculaView current_view;
    int aggregate_dracula_score; // Dracula score is defined as start score - current gamestate score
    int num_games;
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

        if (completion_state == GAME_NOT_OVER) {
            int num_moves_returned = -1;
            char** move_buffer = DvComputePossibleMovesForPlayer(node_to_expand_game_state->current_view, &num_moves_returned);
            assert(num_moves_returned != -1);

            for (int move_index = 0; move_index < num_moves_returned; ++move_index) {
                DraculaView new_move_state = DvMakeCopy(node_to_expand_game_state->current_view);
                DvAdvanceStateByMoves(new_move_state, move_buffer[move_index]);

                PlaceId move_location_id = get_move_location_id(move_buffer[move_index]);

                Item expanded_game_state_item = create_game_state_item(new_move_state, 0, 0, move_location_id, false);

                Node expanded_child = create_new_node_tree(expanded_game_state_item);
                add_new_child_tree(node_to_expand, expanded_child);

                free(move_buffer[move_index]);
            }

            free(move_buffer);

            // The copy view is no longer needed as the node is not a leaf which can be played out
            if (num_moves_returned > 0 && DvIsCopy(node_to_expand_game_state->current_view)) {
                DvFree(node_to_expand_game_state->current_view);
            }
        }
    }

    unlock_node_tree(node_to_expand);
}

static int simulate_random_playout(Node node_to_explore, unsigned int* rand_generator_state) {
    read_lock_node_tree(node_to_explore);
    DraculaView current_game_view = DvMakeCopy(((GameState*) get_node_value_tree(node_to_explore).data)->current_view);
    unlock_node_tree(node_to_explore);
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

////////////////
// Serialization

static inline void write_32_bits_to_file(uint32_t bit_set, FILE* fp) {
    fputc(bit_set >> 24 & 0xFF, fp);
    fputc(bit_set >> 16 & 0xFF, fp);
    fputc(bit_set >> 8 & 0xFF, fp);
    fputc(bit_set & 0xFF, fp);
}

typedef union {
    float f;
    uint32_t bits;
} MungedFloat;

static void recursively_serialize_tree(Node node, FILE* fp) {
    read_lock_node_tree(node);
    uint32_t num_children = get_num_children_tree(node);

    // Write number of children
    write_32_bits_to_file(num_children, fp);

    // Write win rate of current node
    GameState* curr_node_state = ((GameState*)get_node_value_tree(node).data);
    MungedFloat win_rate = {.f = (float) curr_node_state->aggregate_dracula_score / (float) curr_node_state->num_games};
    write_32_bits_to_file(win_rate.bits, fp);

    // Write move of current node
    write_32_bits_to_file(curr_node_state->move, fp);

    // Recurse on children
    Node* children = get_children_tree(node);
    unlock_node_tree(node);
    for (int i = 0; i < num_children; ++i) {
        recursively_serialize_tree(children[i], fp);
    }
}

static void serialize_tree(Tree mcts_tree, FILE* fp) {
    Node root = get_root_tree(mcts_tree);
    recursively_serialize_tree(root, fp);
}

////////////
// Actual AI
int num_iter = 0;

bool interrupted = false;
void signal_interrupt_handler(int signum) {
    printf("Interrupt signal detected. Serializing tree then closing...\n");
    interrupted = true;
}

void* run_simulations(void* mcts_tree) {
    mcts_tree = (Tree) mcts_tree;

    unsigned int rand_generator_state = (long) time(NULL) ^ (long) getpid() ^ (long) pthread_self();
    struct sigaction action = {.sa_handler = signal_interrupt_handler};
    sigaction(SIGINT, &action, NULL);

    while (!interrupted) {
        __atomic_fetch_add(&num_iter, 1, __ATOMIC_SEQ_CST);

        Node promising_node = select_promising_node(mcts_tree);

        expand_node(promising_node);

        read_lock_node_tree(promising_node);
        Node* expanded_child_nodes = get_children_tree(promising_node);
        int num_expanded_child_nodes = get_num_children_tree(promising_node);
        unlock_node_tree(promising_node);
        if (num_expanded_child_nodes <= 0) {
            continue;
        }

        Node node_to_explore = select_child_to_playout(expanded_child_nodes, num_expanded_child_nodes, &rand_generator_state);
        int playout_result = simulate_random_playout(node_to_explore, &rand_generator_state);

        backpropogate_state(node_to_explore, playout_result);
    }

    return NULL;
}

typedef struct {
    Tree tree;
    FILE* serialized_tree_file;
} NumIterArg;

void* print_num_iter(void* data) {
    NumIterArg* iter_arg = (NumIterArg*) data;

    while (!interrupted) {
        printf("Number of iterations ran: %d\n", num_iter);
        sleep(60);
    }
    serialize_tree(iter_arg->tree, iter_arg->serialized_tree_file);

    return NULL;
}

int main() {
    Message messages[] = {};
    DraculaView dv = DvNew("", messages);

    Tree mcts_tree = create_new_tree();
    set_root_tree(mcts_tree, create_new_node_tree(create_game_state_item(dv, 0, 0, NOWHERE, false)));
    FILE* serialized_tree_file = fopen(SERIALIZED_FILE_NAME, "wb");
    if (serialized_tree_file == NULL) {
        fprintf(stderr, "Unable to open serialized tree file for writing. Aborting...\n");
        exit(EXIT_FAILURE);
    }
    NumIterArg iter_arg = {.tree = mcts_tree, .serialized_tree_file = serialized_tree_file};

    // Create MCTS simulation threads
    pthread_t thread_ids[NUMBER_OF_THREADS-1];
    for (int i = 0; i < NUMBER_OF_THREADS-1; ++i) {
        pthread_create(&thread_ids[i], NULL, run_simulations, (void*) mcts_tree);
    }

    // Create periodic update thread
    pthread_attr_t attrs;
    pthread_attr_init(&attrs);
    pthread_attr_setdetachstate(&attrs, 1);
    pthread_t periodic_update;
    pthread_create(&periodic_update, &attrs, print_num_iter, &iter_arg);

    // Allow the main thread to also participate in the simulations as opposed to idling
    run_simulations((void*) mcts_tree);

    // Join the simulation threads to the main thread's execution on their completion
    for (int i = 0; i < NUMBER_OF_THREADS-1; ++i) {
        pthread_join(thread_ids[i], NULL);
    }
    pthread_cancel(periodic_update);

    printf("Final number of iterations ran: %d\n", num_iter);

    fclose(serialized_tree_file);

    printf("Finished serializing...\n");

    free_tree(mcts_tree);

    return EXIT_SUCCESS;
}