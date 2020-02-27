//By Monica Moniot
#ifndef TYPES__H_INCLUDE
#define TYPES__H_INCLUDE

#include "stdio.h"
#include "pcg.h"
#include "basic.h"
#include "mamlib.h"
#include "gb_math.h"
#include "SDL.h"
#include "stb_truetype.h"

const gbVec2 GBORIGIN = {0.0, 0.0};
const gbVec2 GBLEFT  = {-1.0, 0.0};
const gbVec2 GBRIGHT = {1.0, 0.0};
const gbVec2 GBUP    = {0.0, 1.0};
const gbVec2 GBDOWN  = {0.0, -1.0};
const gbVec2 GBDIAG  = {1.0, 1.0};
constexpr float PI = 3.14159265359;
constexpr double default_time_per_frame = 1000.0/60.0;
constexpr auto PIXEL_FORMAT = SDL_PIXELFORMAT_RGBA32;

enum ButtonId: uint32 {
	BUTTON_M1 = 1,
	BUTTON_M2 = 2,
	BUTTON_M3 = 3,
	BUTTON_LEFT,
	BUTTON_RIGHT,
	BUTTON_UP,
	BUTTON_DOWN,
	BUTTON_0,
	BUTTON_1,
	BUTTON_2,
	BUTTON_3,
	BUTTON_4,
	BUTTON_5,
	BUTTON_6,
	BUTTON_7,
	BUTTON_8,
	BUTTON_9,
	BUTTON_LEFT_SHIFT,
	BUTTON_LEFT_CTRL,
	BUTTON_LAST,
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
	gbVec2 screen;
	gbVec2 pre_screen;
	gbVec2 mouse_pos;
	bool did_mouse_move;
	int8 button_trans[BUTTON_LAST];
	bool button_state[BUTTON_LAST];
};


constexpr int32 TILEMAP_DIM = 32;
struct TileMap {
	int32 tiles[TILEMAP_DIM*TILEMAP_DIM];
	struct VBuffer pre_render;
	float buffer[TILEMAP_DIM*8*3*2]
};

struct UserData {
	gbVec2 pos;
	float speed;
	gbVec2 view;
};


constexpr int32 WORLD_DIM = 8;
struct GameState {
	UserData user;
	PCG rng;
	float pixels_per_tile;
	int32 world_dim;
	TileMap world[WORLD_DIM*WORLD_DIM];
	float total_time;
	int32 debug_ticks;
	// uint32 mem_capacity;
	// gbVec2 origin_screen;
	// gbVec2 debug_draw;
	float ave_frame_time;
	// MamStack* trans_stack;
};



// struct Font {
// 	uint bitmap_w;
// 	uint bitmap_h;
// 	stbtt_bakedchar* char_data;
// 	float pixel_size;
// };
struct VBuffer {
	int32 size;
	int32 capacity;
	float* buffer;
	gbMat3 trans;
};
struct Texture {
	int32 x;
	int32 y;
	gbVec4* buffer;
};
struct Output {
	VBuffer vbuffer;
	Texture next_texture;
	// Font sans;
};




const gbVec4 COLOR_WHITE   = {1.0, 1.0, 1.0, 1.0};
const gbVec4 COLOR_GRAY    = {0.5, 0.5, 0.5, 1.0};
const gbVec4 COLOR_BLACK   = {0.0, 0.0, 0.0, 1.0};
const gbVec4 COLOR_RED     = {1.0, 0.0, 0.0, 1.0};
const gbVec4 COLOR_GREEN   = {0.0, 1.0, 0.0, 1.0};
const gbVec4 COLOR_BLUE    = {0.0, 0.0, 1.0, 1.0};
const gbVec4 COLOR_YELLOW  = {1.0, 1.0, 0.0, 1.0};
const gbVec4 COLOR_CYAN    = {0.0, 1.0, 1.0, 1.0};
const gbVec4 COLOR_MAGENTA = {1.0, 0.0, 1.0, 1.0};
const gbVec4 COLOR_ORANGE  = {1.0, 0.5, 0.0, 1.0};
const gbVec4 COLOR_PURPLE  = {0.5, 0.0, 1.0, 1.0};

const gbVec4 COLOR_WHITE_TRANS   = {1.0, 1.0, 1.0, 0.25};
const gbVec4 COLOR_GRAY_TRANS    = {0.5, 0.5, 0.5, 0.25};
const gbVec4 COLOR_BLACK_TRANS   = {0.0, 0.0, 0.0, 0.25};
const gbVec4 COLOR_RED_TRANS     = {1.0, 0.0, 0.0, 0.25};
const gbVec4 COLOR_GREEN_TRANS   = {0.0, 1.0, 0.0, 0.25};
const gbVec4 COLOR_BLUE_TRANS    = {0.0, 0.0, 1.0, 0.25};
const gbVec4 COLOR_YELLOW_TRANS  = {1.0, 1.0, 0.0, 0.25};
const gbVec4 COLOR_CYAN_TRANS    = {0.0, 1.0, 1.0, 0.25};
const gbVec4 COLOR_MAGENTA_TRANS = {1.0, 0.0, 1.0, 0.25};
const gbVec4 COLOR_ORANGE_TRANS  = {1.0, 0.5, 0.0, 0.25};
const gbVec4 COLOR_BROWN_TRANS   = {1.0, 0.5, 0.5, 0.25};
const gbVec4 COLOR_PURPLE_TRANS  = {0.5, 0.0, 1.0, 0.25};


constexpr int32 FLOATS_PER_VBUFFER_VERTEX = 8;
struct GLData {
	uint32 texture;
	uint32 vbuffer_shader;
	uint32 vbuffer_attributes;
	uint32 vbuffer_handle;
	int32 vbuffer_trans_handle;
};

#define DECL_MINMAX(type) inline type min(type a, type b) {return (a < b) ? a : b;}; inline type max(type a, type b) {return (a > b) ? a : b;}

DECL_MINMAX(float)
DECL_MINMAX(double)
DECL_MINMAX(int32)
DECL_MINMAX(int64)





#endif
