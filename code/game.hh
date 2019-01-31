//By Monica Moniot
#pragma once
#include "types.hh"
#include "world.hh"

extern "C" {
void init_game(byte* game_memory);
void init_platform(const byte* game_memory, byte* plat_memory, const Input* input);

void update_game(byte* game_memory, byte* plat_memory, byte* trans_memory, uint32 dt, const Input* input);
void update_platform(byte* game_memory, byte* plat_memory, byte* trans_memory, uint32 dt, const Input* input, Output* output);
}
