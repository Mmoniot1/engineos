//By Monica Moniot
#include "game.hh"
#include <stdio.h>


void init_game(byte* game_memory) {
	GameState* game = cast(GameState*, game_memory);
	Stack* game_stack = cast(Stack*, game_memory + sizeof(GameState));

	UserData* user = &game->user;
	auto world = &game->world;

	memzero(game, sizeof(GameState));
	stack_init(game_stack, tape_size(game_memory) - sizeof(GameState));
	pcg_seed(&game->rng, 12);
	game->slab = stack_push_rel(game_stack, GAME_GENMEM_CAPACITY);
	auto game_slab = ptr_get(Slab, game_stack, game->slab);

	//TODO: Proper sparseness
	world->tilemaps_h = 1024;
	world->tilemaps_w = 1024;
	world->tilemaps = stack_push_rel(game_stack, sizeof(relptr)*world->tilemaps_h*world->tilemaps_w);
	memzero(ptr_get(byte, game_stack, world->tilemaps), sizeof(relptr)*world->tilemaps_h*world->tilemaps_w);

	user->cmd_text_capacity = KILOBYTE;
	user->cmd_text_size = 0;
	user->cmd_text = slab_alloc_rel(game_slab, user->cmd_text_capacity);
	user->text_cursor_pos = 0;
	user->world_pos = gb_vec2(0.0, 0.0);
	user->world_scale = 100.0;
}


void update_game(byte* game_memory, byte* plat_memory, byte* trans_memory, uint32 dt, const Input* input) {
	auto game = cast(GameState*, game_memory);
	auto game_stack = cast(Stack*, game_memory + sizeof(GameState));
	auto platform = cast(Platform*, plat_memory);
	auto platform_stack = cast(Stack*, plat_memory + sizeof(Platform));
	auto trans_stack = cast(Stack*, trans_memory);
	stack_init(trans_stack, tape_size(trans_memory));

	auto user = &game->user;
	auto world = &game->world;
	auto game_slab = ptr_get(Slab, game_stack, game->slab);

	if(input->button_state[BUTTON_M1]) {
		TileCoord tile_coord = get_coord_from_screen(user, input->mouse_pos);
		Tile tile = {TILE_SOMETHING, 0};
		set_tile(world, tile_coord, tile, game_stack);
	}


	if(input->text) {
		if(user->cmd_text_size + input->text_size > user->cmd_text_capacity) {
			user->cmd_text_capacity *= 2;
			user->cmd_text = slab_realloc_rel(game_slab, user->cmd_text, user->cmd_text_capacity);
		}
		auto text_buffer = ptr_get(char, game_slab, user->cmd_text);
		auto input_text = input->text;
		for_each_lt(i, input->text_size) {
			char ch = input_text[i];
			if(ch == CHAR_RIGHT) {
				if(user->text_cursor_pos < user->cmd_text_size) {
					user->text_cursor_pos += 1;
				}
			} else if(ch == CHAR_LEFT) {
				if(user->text_cursor_pos > 0) {
					user->text_cursor_pos -= 1;
				}
			} else if(ch == CHAR_UP) {
			} else if(ch == CHAR_DOWN) {
			} else if(ch == CHAR_BACKSPACE) {
				if(user->text_cursor_pos > 0) {
					for_each_in_range(j, user->text_cursor_pos, user->cmd_text_size - 1) {
						text_buffer[j - 1] = text_buffer[j];
					}
					user->text_cursor_pos -= 1;
					user->cmd_text_size -= 1;
				}
			} else if(ch == CHAR_DELETE) {
				if(user->text_cursor_pos < user->cmd_text_size) {
					for_each_in_range(j, user->text_cursor_pos, user->cmd_text_size - 1) {
						text_buffer[j] = text_buffer[j + 1];
					}
					user->cmd_text_size -= 1;
				}
			} else if(ch == CHAR_RETURN) {
				user->text_cursor_pos = 0;
				user->cmd_text_size = 0;
			} else {
				for(int j = user->cmd_text_size - 1; j >= user->text_cursor_pos; j -= 1) {
					text_buffer[j + 1] = text_buffer[j];
				}
				text_buffer[user->text_cursor_pos] = ch;
				user->text_cursor_pos += 1;
				user->cmd_text_size += 1;
			}
		}
	}
}
