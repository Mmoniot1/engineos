//By Monica Moniot
// #define STB_TRUETYPE_IMPLEMENTATION
#define PCG_IMPLEMENTATION
#include "pcg.h"
#define MAMLIB_IMPLEMENTATION
#include "mamlib.h"
#define GB_MATH_IMPLEMENTATION
#include "gb_math.h"
#include "game.hh"
#define STB_DIVIDE_IMPLEMENTATION
#include "stb_divide.h"
#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"


inline gbVec2 gb_vec2_addx(gbVec2 v, float x) {
	return gb_vec2(v.x + x, v.y);
}
inline gbVec2 gb_vec2_addy(gbVec2 v, float x) {
	return gb_vec2(v.x, v.y + x);
}


inline bool just_down(const Input* input, uint32 button) {
	return ((input->button_trans[button] == 1) && (input->button_state[button])) || (input->button_trans[button] > 1);
}
inline bool just_up  (const Input* input, uint32 button) {
	return ((input->button_trans[button] == 1) && !(input->button_state[button])) || (input->button_trans[button] > 1);
}
inline bool is_down(const Input* input, uint32 button) {
	return input->button_state[button];
}
inline bool is_up  (const Input* input, uint32 button) {
	return !input->button_state[button];
}


static void push_vertex(VBuffer* vbuffer, gbVec2 pos, gbVec4 color = COLOR_WHITE, gbVec2 texture = -GBDIAG) {
	ASSERT(vbuffer->size + FLOATS_PER_VBUFFER_VERTEX <= vbuffer->capacity);
	*cast(gbVec2*, vbuffer->buffer + vbuffer->size) = pos;
	*cast(gbVec2*, vbuffer->buffer + vbuffer->size + 2) = texture;
	*cast(gbVec4*, vbuffer->buffer + vbuffer->size + 4) = color;
	vbuffer->size += FLOATS_PER_VBUFFER_VERTEX;
}
static void push_triangle(VBuffer* vbuffer, gbVec2 pos0, gbVec2 pos1, gbVec2 pos2, gbVec4 color = COLOR_WHITE, gbVec2 texture0 = -GBDIAG, gbVec2 texture1 = -GBDIAG, gbVec2 texture2 = -GBDIAG) {
	push_vertex(vbuffer, pos0, color, texture0);
	push_vertex(vbuffer, pos1, color, texture1);
	push_vertex(vbuffer, pos2, color, texture2);
}
static void push_rect(VBuffer* vbuffer, gbRect2 rect, gbVec4 color = COLOR_WHITE, gbRect2 texture = gb_rect2(-GBDIAG, GBORIGIN)) {
	gbVec2 a = rect.pos;
	gbVec2 b = gb_vec2_addx(rect.pos, rect.dim.x);
	gbVec2 c = gb_vec2_addy(rect.pos, rect.dim.y);
	gbVec2 d = rect.pos + rect.dim;
	gbVec2 t_a = texture.pos;
	gbVec2 t_b = gb_vec2_addx(texture.pos, texture.dim.x);
	gbVec2 t_c = gb_vec2_addy(texture.pos, texture.dim.y);
	gbVec2 t_d = texture.pos + texture.dim;
	push_triangle(vbuffer, a, b, c, color, t_a, t_b, t_c);
	push_triangle(vbuffer, b, c, d, color, t_b, t_c, t_d);
}


void render_tilemap(VBuffer* vbuffer, TileMap* tilemap) {
	for_each_lt(tile_y, TILEMAP_DIM) {
		for_each_lt(tile_x, TILEMAP_DIM) {
			auto tile = tilemap->tiles[TILEMAP_DIM*tile_y + tile_x];
			gbVec4 color = COLOR_RED;
			if(tile == 0) {
				color = COLOR_GREEN;
			} else if(tile == 1) {
				color = COLOR_PURPLE;
			} else if(tile == 2) {
				color = COLOR_ORANGE;
			}
			auto tile_pos = gb_vec2(TILEMAP_DIM*tilemap_x + tile_x, TILEMAP_DIM*tilemap_y + tile_y);
			push_rect(vbuffer, gb_rect2(game_state->pixels_per_tile*(tile_pos - offset), game_state->pixels_per_tile*GBDIAG), color);
		}
	}
}


void update_game(GameState* game_state, MamStack* trans_stack, float dt, Input* input, Output* output) {
	auto user = &game_state->user;

	user->view = input->screen/game_state->pixels_per_tile;

	{//framerate printout
		game_state->debug_ticks += 1;
		// game_state->ave_frame_time = .75*game_state->ave_frame_time + .25*dt;
		// if(game_state->debug_ticks%(200) == 0) {
		// 	printf("frame_rate: %fms\n", game_state->ave_frame_time*1000);
		// }
	}

	if(just_down(input, BUTTON_LEFT_SHIFT)) {
		user->speed *= 2;
	} else if(just_up(input, BUTTON_LEFT_SHIFT)) {
		user->speed /= 2;
	}

	if(is_down(input, BUTTON_LEFT)) {
		user->pos.x -= user->speed*dt;
	} else if(is_down(input, BUTTON_RIGHT)) {
		user->pos.x += user->speed*dt;
	}
	if(is_down(input, BUTTON_UP)) {
		user->pos.y += user->speed*dt;
	} else if(is_down(input, BUTTON_DOWN)) {
		user->pos.y -= user->speed*dt;
	}

	gbMat3 vertex_trans = {
		2.0/input->screen.x, 0, -1.0,
		0, 2.0/input->screen.y, -1.0,
		0, 0, 0
	};
	output->vbuffer.trans = vertex_trans;

	Texture* out_texture = &output->next_texture;
	{
		gbVec2 offset = user->pos - user->view/2;
		gbVec2 bot_corner = user->pos - user->view/2;
		gbVec2 top_corner = bot_corner + user->view;
		bot_corner /= TILEMAP_DIM;
		top_corner /= TILEMAP_DIM;
		bot_corner = gb_vec2(gb_floor(bot_corner.x), gb_floor(bot_corner.y));
		top_corner = gb_vec2(gb_floor(top_corner.x), gb_floor(top_corner.y));
		for_each_in_range(tilemap_y, cast(int32, bot_corner.y), cast(int32, top_corner.y)) {
			for_each_in_range(tilemap_x, cast(int32, bot_corner.x), cast(int32, top_corner.x)) {

				auto tilemap = &game_state->world[game_state->world_dim*stb_mod_floor(tilemap_y, game_state->world_dim) + stb_mod_floor(tilemap_x, game_state->world_dim)];
				render_tilemap()

			}
		}
	}

	game_state->total_time += dt;
	return;
}


void init_game(GameState* game_state, MamStack* trans_stack, Input* input) {
	UserData* user = &game_state->user;

	memzero(game_state, 1);
	pcg_seed(&game_state->rng, 12);
	user->speed = 20;

	game_state->pixels_per_tile = 32;
	game_state->world_dim = WORLD_DIM;

	// {//noise test
	// 	out_texture->x = input->screen.x/2;
	// 	out_texture->y = input->screen.y/2;
	// 	out_texture->buffer = (gbVec4*)realloc(out_texture->buffer, sizeof(gbVec4)*out_texture->x*out_texture->y);
	// 	for_each_lt(y, out_texture->y) {
	// 		for_each_lt(x, out_texture->x) {
	// 			float zoom = 200;
	// 			float r = stb_perlin_ridge_noise3(cast(float, x)/zoom, cast(float, y)/zoom, game_state->total_time/16,
	// 				2, .5, 1, 5,
	// 				0, 0, 0
	// 			);
	// 			r = r < .14;
	// 			auto pixel = &out_texture->buffer[x + cast(int32, out_texture->x)*y];
	// 			*pixel = gb_vec4(r, r, r, 1);
	// 		}
	// 	}
	//
	// 	push_rect(&output->vbuffer, gb_rect2(GBORIGIN, input->screen), COLOR_PURPLE_TRANS, gb_rect2(GBORIGIN, GBDIAG));
	// }
	for_each_lt(tilemap_y, WORLD_DIM) {
		for_each_lt(tilemap_x, WORLD_DIM) {
			auto tilemap = &game_state->world[WORLD_DIM*tilemap_y + tilemap_x];
			for_each_lt(tile_y, TILEMAP_DIM) {
				for_each_lt(tile_x, TILEMAP_DIM) {
					auto tile = &tilemap->tiles[TILEMAP_DIM*tile_y + tile_x];
					gbVec2 abs_pos = gb_vec2(TILEMAP_DIM*tilemap_x + tile_x, TILEMAP_DIM*tilemap_y + tile_y);

					float zoom = 32;
					float r = stb_perlin_ridge_noise3(abs_pos.x/zoom, abs_pos.y/zoom, 0,
						2, .5, 1, 5,
						0, 0, 0
					);
					if(r < .3) {
						*tile = 2;
					}
				}
			}
			auto tile = &tilemap->tiles[TILEMAP_DIM*tilemap_y + tilemap_x];
			*tile = 1;
			tile = &tilemap->tiles[0];
			*tile = 1;
			// for_each_lt(tile_x, TILEMAP_DIM) {
			// 	auto tile = &tilemap->tiles[TILEMAP_DIM*0 + tile_x];
			// 	*tile = 0;
			// }
			// for_each_lt(tile_y, TILEMAP_DIM) {
			// 	auto tile = &tilemap->tiles[TILEMAP_DIM*tile_y + 0];
			// 	*tile = 0;
			// }
		}
	}

	return;
}
