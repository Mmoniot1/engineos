//By Monica Moniot
#include "world.hh"



static Tilemap* get_tilemap(World* world, TileCoord pos, Stack* game_stack) {
	int32 tilemap_x = (pos.x>>TILEMAP_BITSHIFT) + world->tilemaps_w/2;//center tilemap around 0, 0
	int32 tilemap_y = (pos.y>>TILEMAP_BITSHIFT) + world->tilemaps_h/2;
	assert(tilemap_x < world->tilemaps_w);
	assert(tilemap_y < world->tilemaps_h);
	relptr* tilemap = &ptr_get(relptr, game_stack, world->tilemaps)[world->tilemaps_w*tilemap_y + tilemap_x];
	if(!*tilemap) {
		*tilemap = stack_push_rel(game_stack, sizeof(Tilemap));
		memzero(ptr_get(byte, game_stack, *tilemap), world->tilemaps_w*world->tilemaps_h);
	}
	return ptr_get(Tilemap, game_stack, *tilemap);
}
static Tilemap* get_tilemap_or_null(const World* world, TileCoord pos, const Stack* game_stack) {//will never write
	int32 tilemap_x = (pos.x>>TILEMAP_BITSHIFT) + world->tilemaps_w/2;//center tilemap around 0, 0
	int32 tilemap_y = (pos.y>>TILEMAP_BITSHIFT) + world->tilemaps_h/2;
	assert(tilemap_x < world->tilemaps_w);
	assert(tilemap_y < world->tilemaps_h);
	relptr* tilemap = &ptr_get(relptr, game_stack, world->tilemaps)[world->tilemaps_w*tilemap_y + tilemap_x];
	if(!*tilemap) {
		return 0;
	}
	return ptr_get(Tilemap, game_stack, *tilemap);
}

Tile get_tile(const World* world, TileCoord pos, const Stack* game_stack) {
	uint32 tile_x = pos.x&TILEMAP_BITMASK;
	uint32 tile_y = pos.y&TILEMAP_BITMASK;
	auto tilemap = get_tilemap_or_null(world, pos, game_stack);
	if(!tilemap) {
		Tile ret = {TILE_INVALID, 0};
		return ret;
	}
	return tilemap->tiles[TILEMAP_SIDELENGTH*tile_y + tile_x];
}

void set_tile(World* world, TileCoord pos, Tile tile, Stack* game_stack) {
	uint32 tile_x = pos.x&TILEMAP_BITMASK;
	uint32 tile_y = pos.y&TILEMAP_BITMASK;
	get_tilemap(world, pos, game_stack)->tiles[TILEMAP_SIDELENGTH*tile_y + tile_x] = tile;
}
