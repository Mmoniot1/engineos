//By Monica Moniot
#pragma once
#include "types.hh"

extern "C" {
inline TileCoord to_coord(int32 x, int32 y) {
	TileCoord ret = {x, y};
	return ret;
}
inline TileCoord to_coord_vec(vec2 pos) {
	TileCoord ret = {pos.x, pos.y};
	return ret;
}
inline TileCoord get_coord_from_screen(const UserData* user, vec2 screen) {
	return to_coord_vec(user->world_scale*screen + user->world_pos);
}
inline vec2 to_vec(TileCoord pos) {
	return gb_vec2(pos.x, pos.y);
}

Tile get_tile(const World* world, TileCoord pos, const Stack* game_stack);

void set_tile(World* world, TileCoord pos, Tile tile, Stack* game_stack);
}
