//By Monica Moniot
#ifndef GAME__H_INCLUDE
#define GAME__H_INCLUDE

#include "types.hh"

extern "C" void init_game(GameState* game_state, MamStack* trans_stack, Input* input);

extern "C" void update_game(GameState* game_state, MamStack* trans_stack, float dt, Input* input, Output* output);

#endif
