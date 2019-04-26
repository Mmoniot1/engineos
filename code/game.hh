//By Monica Moniot
#pragma once
#include "types.hh"
#include "world.hh"

extern "C" {
void init_game(byte* game_memory, byte* plat_memory, const Input* input);

void update_game(byte* game_memory, byte* plat_memory, byte* trans_memory, float dt, const Input* input, Output* output);
}
