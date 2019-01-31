//By Monica Moniot
#pragma once
#include <math.h>
#include "random.hh"
#include "basic.h"
#include "gb_math.h"
#include "SDL.h"
#include "stb_truetype.h"

extern "C" {
typedef gbVec2 vec2;
typedef gbVec3 vec3;
typedef gbVec4 vec4;
typedef gbMat2 mat2;
typedef gbMat3 mat3;
typedef gbMat4 mat4;
constexpr vec2 ORIGIN = {0.0, 0.0};
static inline mat4 gb_mat4(void) {mat4 ret; gb_mat4_identity(&ret); return ret;}
constexpr float default_ms_per_frame = 1000.0/60.0;
constexpr uint32 default_updates_per_frame = 30;
constexpr float ms_per_update = default_ms_per_frame/default_updates_per_frame;//this is currently .555...
constexpr float update_per_ms = 1/ms_per_update;
constexpr auto PIXEL_FORMAT = SDL_PIXELFORMAT_RGBA32;

constexpr uint32 GAME_GENMEM_CAPACITY = 16*MEGABYTE;


struct TileCoord {
	int32 x;
	int32 y;
};
enum TileId : int32 {//must interchange with a relptr
	TILE_EMPTY = 0,
	TILE_INVALID = -1,
	TILE_SOMETHING = 1,
};


constexpr float TILE_LENGTH = 1.0;
constexpr uint32 TILEMAP_BITSHIFT = 6;
constexpr uint32 TILEMAP_BITMASK = 1<<TILEMAP_BITSHIFT - 1;
constexpr uint32 TILEMAP_SIDELENGTH = 1<<TILEMAP_BITSHIFT;
struct Tile {
	TileId id;
	relptr data;
};
struct Tilemap {
	Tile tiles[TILEMAP_SIDELENGTH];
};

struct World {
	relptr tilemaps;
	uint32 tilemaps_w;
	uint32 tilemaps_h;
};


enum : int32 {
	BUTTON_M1 = 1,
	BUTTON_M2 = 2,
	BUTTON_M3 = 3,
	BUTTON_0 = 0x10,
	BUTTON_1 = 0x11,
	BUTTON_2 = 0x12,
	BUTTON_3 = 0x13,
	BUTTON_4 = 0x14,
	BUTTON_5 = 0x15,
	BUTTON_6 = 0x16,
	BUTTON_7 = 0x17,
	BUTTON_8 = 0x18,
	BUTTON_9 = 0x19,
};

struct Input {//0 is init
	char* text;
	uint32 text_size;
	vec2 screen;
	vec2 mouse_pos;
	bool did_mouse_move;
	uint32 button_transitions[0x19];
	bool button_state[0x19];
};

enum : char {
	CHAR_BACKSPACE = 8,
	CHAR_DELETE = 7,
	CHAR_TAB = 11,
	CHAR_RETURN = 12,
	CHAR_UP = 14,
	CHAR_DOWN = 15,
	CHAR_RIGHT = 16,
	CHAR_LEFT = 17,
};


struct UserData {
	relptr cmd_text;
	uint32 cmd_text_size;
	uint32 cmd_text_capacity;
	int32 text_cursor_pos;
	vec2 world_pos;
	float world_scale;//pixels_per_unit
};
struct GameState {
	UserData user;
	relptr slab;
	World world;
	PCG rng;
};

struct Font {
	stbtt_fontinfo info;
	uint32 pixel_size;
};
struct VBuffer {
	uint32 size;
	uint32 capacity;
	float* buffer;
	mat3 trans;
};
struct GLData {
	uint32 gui_attributes;
	uint32 gui_handle;
	uint32 gui_shader;
	uint32 text_texture;
	uint32 world_graphics_shader;
	uint32 world_graphics_attributes;
	uint32 world_graphics_handle;
};
struct Output {
	VBuffer gui;
	VBuffer world_graphics;
};
struct Platform {
	Font cmd_font;
	vec2 debug_draw;
};
}
