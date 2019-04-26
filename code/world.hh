//By Monica Moniot
// extern "C" {
#ifndef WORLD__H_INCLUDE
#define WORLD__H_INCLUDE

#include "types.hh"

constexpr float TILE_LENGTH = 1.0;
constexpr uint32 TILEMAP_BITSHIFT = 6;
constexpr uint32 TILEMAP_SIDELENGTH = 1<<TILEMAP_BITSHIFT;
constexpr uint32 TILEMAP_BITMASK = TILEMAP_SIDELENGTH - 1;
constexpr uint32 TILEMAP_SIZE = TILEMAP_SIDELENGTH*TILEMAP_SIDELENGTH;

struct Tilemap {
	Tile tiles[TILEMAP_SIZE];
};

inline TileCoord to_coord(int32 x, int32 y) {
	TileCoord ret = {x, y};
	return ret;
}
inline TileCoord to_coordv(vec2 pos) {
	TileCoord ret = {roundf(pos.x), roundf(pos.y)};
	return ret;
}
inline vec2 to_vec(TileCoord pos) {
	return gb_vec2(pos.x, pos.y);
}

inline mat3 get_world_from_screen_trans(float ppt, vec2 world_origin, vec2 screen) {
	mat3 trans = {
		1.0/ppt, 0.0, -(screen.x/2)/ppt + world_origin.x,
		0.0, 1.0/ppt, -(screen.y/2)/ppt + world_origin.y,
		0.0, 0.0, 0.0,
	};
	return trans;
}
inline mat3 get_screen_from_world_trans(float ppt, vec2 world_origin, vec2 screen) {
	mat3 trans = {
		ppt, 0.0, screen.x/2 - ppt*world_origin.x,
		0.0, ppt, screen.y/2 - ppt*world_origin.y,
		0.0, 0.0, 0.0,
	};
	return trans;
}
inline mat3 get_gl_from_screen_trans(vec2 screen) {
	mat3 trans = {
		2.0/screen.x, 0.0, -1.0,
		0.0, 2.0/screen.y, -1.0,
		0.0, 0.0, 0.0,
	};
	// mat3 trans = {
	// 	1.0, 0.0, -1.0,
	// 	0.0, 1.0, -1.0,
	// 	0.0, 0.0, 0.0,
	// };
	return trans;
}
inline mat3 get_gl_from_world_trans(float ppt, vec2 world_origin, vec2 screen) {
	mat3 trans = {
		2*ppt/screen.x, 0.0, -ppt*world_origin.x/screen.x,
		0.0, 2*ppt/screen.y, -ppt*world_origin.y/screen.y,
		0.0, 0.0, 0.0,
	};
	return trans;
}

inline vec2 get_world_from_screen(float ppt, vec2 world_origin, vec2 screen, vec2 screen_pos) {
	auto trans = get_world_from_screen_trans(ppt, world_origin, screen);
	auto up = gb_vec3(screen_pos.x, screen_pos.y, 1.0);
	return (trans*up).xy;
}
inline TileCoord get_coord_from_screen(float ppt, vec2 world_origin, vec2 screen, vec2 screen_pos) {
	return to_coordv(get_world_from_screen(ppt, world_origin, screen, screen_pos));
}
inline vec2 get_screen_from_world(float ppt, vec2 world_origin, vec2 screen, vec2 world_pos) {
	auto trans = get_screen_from_world_trans(ppt, world_origin, screen);
	auto up = gb_vec3(world_pos.x, world_pos.y, 1.0);
	return (trans*up).xy;
}

inline vec2 get_world_from_screenu(const UserData* user, vec2 screen, vec2 screen_pos) {
	return get_world_from_screen(user->pixels_per_tile, user->world_pos, screen, screen_pos);
}
inline TileCoord get_coord_from_screenu(const UserData* user, vec2 screen, vec2 screen_pos) {
	return get_coord_from_screen(user->pixels_per_tile, user->world_pos, screen, screen_pos);
}
inline vec2 get_screen_from_worldu(const UserData* user, vec2 screen, vec2 world_pos) {
	return get_screen_from_world(user->pixels_per_tile, user->world_pos, screen, world_pos);
}

void world_init(GameState* game);

Tile world_get_tile(const GameState* game, TileCoord pos);

bool world_set_tile(GameState* game, TileCoord pos, Tile tile);

void generate_world_in_range(GameState* game, TileCoord pos0, TileCoord pos1);

Tile world_get_machine(const GameState* game, TileCoord pos);

void world_set_machine(GameState* game, TileCoord pos, Tile machine);

#endif

#ifdef WORLD_IMPLEMENTATION
#undef WORLD_IMPLEMENTATION

struct Coord {
	uint32 x, y;
};
struct FullCoord {
	Coord map, tile;
};

inline FullCoord get_full_from_coord(const WorldLayer* world, TileCoord pos) {
	FullCoord ret;
	ret.map.x = (pos.x>>TILEMAP_BITSHIFT) + world->tilemaps_w/2;//center tilemap around 0, 0
	ret.map.y = (pos.y>>TILEMAP_BITSHIFT) + world->tilemaps_h/2;
	assert(ret.map.x < world->tilemaps_w);
	assert(ret.map.y < world->tilemaps_h);
	ret.tile.x = pos.x&TILEMAP_BITMASK;
	ret.tile.y = pos.y&TILEMAP_BITMASK;
	return ret;
}
inline Tile* get_tile_ptr(const WorldLayer* world, relptr tilemap, Coord tile_coord, const MamStack* game_stack) {
	return &mam_get_ptr(Tilemap, game_stack, tilemap)->tiles[TILEMAP_SIDELENGTH*tile_coord.y + tile_coord.x];
}
inline relptr* get_tilemap_ptr(const WorldLayer* world, Coord map_coord, const MamStack* game_stack) {
	return &mam_get_ptr(relptr, game_stack, world->tilemaps)[world->tilemaps_w*map_coord.y + map_coord.x];
}

inline float hash_tile(FullCoord coord, uint64 key) {
	PCG rng;
	uint64 item = (cast(uint64, (coord.map.x<<TILEMAP_BITSHIFT)|(coord.tile.x))<<32)|(coord.map.y<<TILEMAP_BITSHIFT)|(coord.tile.y);
	pcg_seeds(&rng, key, item);
	return pcg_random_uniform(&rng);
}

static void tilemap_init(Tilemap* tilemap, Coord map, uint64 seed) {
	FullCoord coord;
	coord.map = map;
	for_each_lt(y, TILEMAP_SIDELENGTH) {
		coord.tile.y = y;
		for_each_lt(x, TILEMAP_SIDELENGTH) {
			coord.tile.x = x;
			float r = hash_tile(coord, seed);
			Tile tile = {TILE_EMPTY, 0};
			if(r < .02) {
				tile.id = TILE_IRON_ORE;
			} else if(r < .03) {
				tile.id = TILE_COPPER_ORE;
			}
			tilemap->tiles[TILEMAP_SIDELENGTH*y + x] = tile;
		}
	}
}

static void world_layer_init(WorldLayer* world, MamStack* game_stack) {
	//TODO: Proper sparseness
	world->tilemaps_h = 1024;
	world->tilemaps_w = 1024;
	world->tilemaps = mam_stack_pushi(game_stack, sizeof(relptr)*world->tilemaps_h*world->tilemaps_w);
	memzero(mam_get_ptr(byte, game_stack, world->tilemaps), sizeof(relptr)*world->tilemaps_h*world->tilemaps_w);
}
void world_init(GameState* game) {
	auto game_stack = &game->stack;
	auto world = &game->world;
	world_layer_init(&world->terrain, game_stack);
	world_layer_init(&world->machines, game_stack);
}

static Tile terrain_get_tile(const WorldLayer* world, TileCoord pos, const MamStack* game_stack) {
	auto coord = get_full_from_coord(world, pos);
	auto tilemap = get_tilemap_ptr(world, coord.map, game_stack);
	if(*tilemap) {
		return *get_tile_ptr(world, *tilemap, coord.tile, game_stack);
	} else {
		Tile ret = {TILE_INVALID, 0};
		return ret;
	}
}

static bool terrain_set_tile(WorldLayer* world, TileCoord pos, Tile tile, const MamStack* game_stack) {
	auto coord = get_full_from_coord(world, pos);
	auto tilemap = get_tilemap_ptr(world, coord.map, game_stack);
	if(*tilemap) {
		*get_tile_ptr(world, *tilemap, coord.tile, game_stack) = tile;
		return 1;
	} else {
		return 0;
	}
}

Tile world_get_tile(const GameState* game, TileCoord pos) {
	auto game_stack = &game->stack;
	auto world = &game->world;
	return terrain_get_tile(&world->terrain, pos, game_stack);
}
bool world_set_tile(GameState* game, TileCoord pos, Tile tile) {
	auto game_stack = &game->stack;
	auto world = &game->world;
	return terrain_set_tile(&world->terrain, pos, tile, game_stack);
}

void generate_world_in_range(GameState* game, TileCoord pos0, TileCoord pos1) {
	auto game_stack = &game->stack;
	auto world = &game->world;
	auto terrain = &world->terrain;
	auto coord0 = get_full_from_coord(terrain, pos0);
	auto coord1 = get_full_from_coord(terrain, pos1);
	for_each_in_range(y, coord0.map.y, coord1.map.y) {
		for_each_in_range(x, coord0.map.x, coord1.map.x) {
			Coord coord = {x, y};
			auto tilemap = get_tilemap_ptr(terrain, coord, game_stack);
			if(!*tilemap) {
				*tilemap = mam_stack_pushi(game_stack, sizeof(Tilemap));
				tilemap_init(mam_get_ptr(Tilemap, game_stack, *tilemap), coord, terrain->seed);
				mam_checki(game_stack, *tilemap, sizeof(Tilemap));
			}
		}
	}
}

static Tile layer_get_machine(const WorldLayer* engine, TileCoord pos, const MamStack* game_stack) {
	auto coord = get_full_from_coord(engine, pos);
	auto tilemap = get_tilemap_ptr(engine, coord.map, game_stack);
	if(*tilemap) {
		return *get_tile_ptr(engine, *tilemap, coord.tile, game_stack);
	} else {
		Tile ret = {MACHINE_EMPTY, 0};
		return ret;
	}
}
static void layer_set_machine(WorldLayer* engine, TileCoord pos, Tile machine, MamStack* game_stack) {
	auto coord = get_full_from_coord(engine, pos);
	auto tilemap = get_tilemap_ptr(engine, coord.map, game_stack);
	if(!*tilemap) {
		*tilemap = mam_stack_pushi(game_stack, sizeof(Tilemap));
		memzero(mam_get_ptr(Tilemap, game_stack, *tilemap), sizeof(Tilemap));
		mam_checki(game_stack, *tilemap, sizeof(Tilemap));
	}
	*get_tile_ptr(engine, *tilemap, coord.tile, game_stack) = machine;
}

Tile world_get_machine(const GameState* game, TileCoord pos) {
	auto game_stack = &game->stack;
	auto world = &game->world;
	return layer_get_machine(&world->machines, pos, game_stack);
}
void world_set_machine(GameState* game, TileCoord pos, Tile machine) {
	auto game_stack = &game->stack;
	auto world = &game->world;
	return layer_set_machine(&world->machines, pos, machine, game_stack);
}


#endif
// }
