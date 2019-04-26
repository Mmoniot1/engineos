//By Monica Moniot
#ifndef TYPES__H_INCLUDE
#define TYPES__H_INCLUDE

#include <stdio.h>
#include "random.hh"
#include "basic.h"
#include "mam_intrinsics.h"
#define MAM_ALLOC_SIZE_T int32
#define MAM_ALLOC_DEBUG
#include "mam_alloc.h"
#include "gb_math.h"
#include "SDL.h"
#include "stb_truetype.h"

extern "C" {
typedef int32 relptr;
typedef gbVec2 vec2;
typedef gbVec3 vec3;
typedef gbVec4 vec4;
typedef gbMat2 mat2;
typedef gbMat3 mat3;
typedef gbMat4 mat4;
const vec2 ORIGIN = {0.0, 0.0};
const vec2 POS = {1.0, 1.0};
const vec2 NEG = {-1.0, -1.0};
constexpr float PI = 3.14159265359;
inline mat4 gb_mat4(void) {mat4 ret; gb_mat4_identity(&ret); return ret;}
constexpr double default_time_per_frame = 1000.0/60.0;
constexpr auto PIXEL_FORMAT = SDL_PIXELFORMAT_RGBA32;

constexpr uint32 GAME_GENMEM_CAPACITY = 16*MEGABYTE;

static float vec2_length(vec2 p) {return sqrtf(p.x*p.x + p.y*p.y);}

enum ButtonId: int32 {
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

struct Input {//0 is init
	char* text;
	uint32 text_size;
	vec2 screen;
	vec2 pre_screen;
	vec2 mouse_pos;
	bool did_mouse_move;
	uint8 button_trans[0x19];
	bool button_state[0x19];
};


struct TileCoord {
	int32 x;
	int32 y;
};
enum TileId : int32 {//must interchange with a relptr
	TILE_INVALID = -1,
	TILE_EMPTY = 0,
	TILE_IRON_ORE,
	TILE_COPPER_ORE,
};
enum : int32 {
	MACHINE_EMPTY = 0,
	MACHINE_STRUCTURE,
	MACHINE_MINER,
	MACHINE_SMELTER,
	MACHINE_FACTORY,
	MACHINE_COMPUTER,
	MACHINE_POWER,
	MACHINES_TOTAL,
};
typedef int32 MachineId;

struct Tile {
	int32 id;
	relptr data;
};

struct MachinePort {//refers to the machine connected by a specific port
	relptr machine_data;
	int32 this_portno;
};

constexpr uint32 MAX_IN_PORTS = 4;
constexpr uint32 MAX_OUT_PORTS = 4;
struct Machine {
	MachineId id;//the machine identifier enum
	// vec2 world_pos;
	//beginning of process data
	float rate;//the current crafting rate of the machine; 1 for maximum speed, 0 for off
	float preprocessed_time;//the amount of time the machine spends running outside of the main processing loop
	uint32 in_ports_size;
	uint32 out_ports_size;
	float max_in[MAX_IN_PORTS];//one machine per port for now
	int32 in_reso_ids[MAX_IN_PORTS];
	float in_store[MAX_IN_PORTS];
	float max_in_store[MAX_IN_PORTS];
	MachinePort in_ports[MAX_IN_PORTS];
	vec2 in_port_poses[MAX_IN_PORTS];
	MachinePort out_ports[MAX_OUT_PORTS];
	float max_out[MAX_OUT_PORTS];
	int32 out_reso_ids[MAX_OUT_PORTS];
	vec2 out_port_poses[MAX_OUT_PORTS];
};


enum ResoId : int32 {
	COPPER_ORE,
	COPPER,
	IRON_ORE,
	IRON,
	CIRCUITRY,
	RESOS_TOTAL,
};
enum RecipeId : int32 {
	EXTRACT_IRON_ORE,
	EXTRACT_COPPER_ORE,
	SMELT_IRON,
	SMELT_COPPER,
	RECIPES_TOTAL,
};


struct WorldLayer {
	relptr tilemaps;
	uint32 tilemaps_w;
	uint32 tilemaps_h;
	uint64 seed;
};
struct World {
	WorldLayer terrain;
	WorldLayer machines;
};
struct UserData {
	relptr cmd_text;
	uint32 cmd_text_size;
	uint32 cmd_text_capacity;
	int32 text_cursor_pos;
	vec2 world_pos;
	float pixels_per_tile;
	int32 selected_machine;
	//port selection
	MachinePort port;
	bool port_is_in;
};

// struct Engine {
	// relptr prod_mat;//RESOS_TOTAL*RECIPES_TOTAL matrix storing the prod of all recipes
	// relptr prod_vec;//RESOS_TOTAL vector storing the current prod of all resos, must agree with prod_mat
	// relptr reso_vec;//RESOS_TOTAL vector storing the current storage of all resos
// };
constexpr uint32 BUILT_MACHINES_CAPACITY = 32;
struct GameState {
	uint32 mem_capacity;//the total amount of memory available for the game
	UserData user;
	relptr heap;
	World world;
	// Engine engine;
	float time_since_last_tick;
	float reso_vec[RESOS_TOTAL];
	uint32 built_machines_size;
	relptr built_machines[BUILT_MACHINES_CAPACITY];
	relptr built_machine_coords[BUILT_MACHINES_CAPACITY];
	PCG rng;
	MamStack stack;//the rest of game memory
};

constexpr int32 FLOATS_PER_WORLD_VERTEX = 8;//NOTE: floats should be added to the world vbuffer this many at a time
constexpr int32 FLOATS_PER_GUI_VERTEX = 8;//NOTE: floats should be added to the world vbuffer this many at a time
struct Font {
	uint bitmap_w;
	uint bitmap_h;
	stbtt_bakedchar* char_data;
	float pixel_size;
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
	uint32 mono_texture;
	uint32 world_graphics_shader;
	uint32 world_graphics_attributes;
	uint32 world_graphics_handle;
	int32 world_graphics_trans_handle;
	int32 gui_trans_handle;
};
struct Output {
	VBuffer gui;
	VBuffer world_graphics;
	Font sans;
};
struct Platform {
	uint32 mem_capacity;
	vec2 origin_screen;
	vec2 debug_draw;
	double ave_frame_time;
	MamStack stack;
};

const vec4 COLOR_WHITE   = {1.0, 1.0, 1.0, 1.0};
const vec4 COLOR_GRAY    = {0.5, 0.5, 0.5, 1.0};
const vec4 COLOR_BLACK   = {0.0, 0.0, 0.0, 1.0};
const vec4 COLOR_RED     = {1.0, 0.0, 0.0, 1.0};
const vec4 COLOR_GREEN   = {0.0, 1.0, 0.0, 1.0};
const vec4 COLOR_BLUE    = {0.0, 0.0, 1.0, 1.0};
const vec4 COLOR_YELLOW  = {1.0, 1.0, 0.0, 1.0};
const vec4 COLOR_CYAN    = {0.0, 1.0, 1.0, 1.0};
const vec4 COLOR_MAGENTA = {1.0, 0.0, 1.0, 1.0};
const vec4 COLOR_ORANGE  = {1.0, 0.5, 0.0, 1.0};
const vec4 COLOR_PURPLE  = {0.5, 0.0, 1.0, 1.0};

const vec4 COLOR_WHITE_TRANS   = {1.0, 1.0, 1.0, 0.5};
const vec4 COLOR_GRAY_TRANS    = {0.5, 0.5, 0.5, 0.5};
const vec4 COLOR_BLACK_TRANS   = {0.0, 0.0, 0.0, 0.5};
const vec4 COLOR_RED_TRANS     = {1.0, 0.0, 0.0, 0.5};
const vec4 COLOR_GREEN_TRANS   = {0.0, 1.0, 0.0, 0.5};
const vec4 COLOR_BLUE_TRANS    = {0.0, 0.0, 1.0, 0.5};
const vec4 COLOR_YELLOW_TRANS  = {1.0, 1.0, 0.0, 0.5};
const vec4 COLOR_CYAN_TRANS    = {0.0, 1.0, 1.0, 0.5};
const vec4 COLOR_MAGENTA_TRANS = {1.0, 0.0, 1.0, 0.5};
const vec4 COLOR_ORANGE_TRANS  = {1.0, 0.5, 0.0, 0.5};
const vec4 COLOR_PURPLE_TRANS  = {0.5, 0.0, 1.0, 0.5};
}

#define DECL_MINMAX(type) inline type min(type a, type b) {return (a < b) ? a : b;}; inline type max(type a, type b) {return (a > b) ? a : b;}

DECL_MINMAX(float)
DECL_MINMAX(double)
DECL_MINMAX(int32)
DECL_MINMAX(int64)


#endif
