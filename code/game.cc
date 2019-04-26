//By Monica Moniot
#define GB_MATH_IMPLEMENTATION
#define WORLD_IMPLEMENTATION
#define MACHINES_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#define PCG_IMPLEMENTATION
#define MAM_ALLOC_IMPLEMENTATION
#define RENDER_IMPLEMENTATION
#include "game.hh"
#include "machines.hh"
#include "render.hh"


inline bool just_down(const Input* input, uint32 button) {
	return ((input->button_trans[button] == 1) && (input->button_state[button])) || (input->button_trans[button] > 1);
}
inline bool just_up  (const Input* input, uint32 button) {
	return ((input->button_trans[button] == 1) && !(input->button_state[button])) || (input->button_trans[button] > 1);
}



void init_platform(const byte* game_memory, byte* plat_memory, const Input* input) {
	auto game = cast(GameState*, game_memory);
	auto game_stack = cast(MamStack*, game_memory + sizeof(GameState));
	auto platform = cast(Platform*, plat_memory);
	auto platform_stack = &platform->stack;

	memzero(platform, sizeof(Platform));
	mam_stack_init(platform_stack, platform->mem_capacity - sizeof(Platform));
	platform->ave_frame_time = 0.0;
}

void update_platform(GameState* game, Platform* platform, MamStack* trans_stack, double dt, const Input* input, Output* output) {
	auto game_stack = &game->stack;
	auto platform_stack = &platform->stack;

	auto user = &game->user;
	auto screen = input->screen;
	auto world = &game->world;
	auto game_heap = mam_get_ptr(MamHeap, game_stack, game->heap);

	Font* debug_font = &output->sans;
	auto gui = &output->gui;
	auto graphics = &output->world_graphics;

	const double ave_weight = 1.0/32.0;
	platform->ave_frame_time = (1 - ave_weight)*platform->ave_frame_time + ave_weight*dt;

	output->gui.trans = get_gl_from_screen_trans(screen);
	output->world_graphics.trans = get_gl_from_world_trans(user->pixels_per_tile, user->world_pos, screen);
	const char* selected_machine_str = "None";
	{//draw placement tiles
		TileCoord bottom_left = get_coord_from_screenu(user, screen, ORIGIN);
		TileCoord top_right = get_coord_from_screenu(user, screen, screen);
		draw_box(graphics, gb_vec2(bottom_left.x - .5, bottom_left.y - .5), gb_vec2(top_right.x + .5, top_right.y + .5), COLOR_PURPLE);

		for_each_in_range(y, bottom_left.y, top_right.y) {
			for_each_in_range(x, bottom_left.x, top_right.x) {
				auto coord = to_coord(x, y);
				auto tile = world_get_tile(game, coord);
				if(tile.id == TILE_EMPTY) {
					draw_tile(graphics, coord, COLOR_BLACK_TRANS);
				} else if(tile.id == TILE_INVALID) {
					draw_tile(graphics, coord, COLOR_RED);
				} else if(tile.id == TILE_IRON_ORE) {
					draw_tile(graphics, coord, COLOR_WHITE_TRANS);
				} else if(tile.id == TILE_COPPER_ORE) {
					draw_tile(graphics, coord, COLOR_ORANGE_TRANS);
				} else {
					assert(0);
				}
			}
		}
		for_each_in_range(y, bottom_left.y, top_right.y) {
			for_each_in_range(x, bottom_left.x, top_right.x) {
				auto coord = to_coord(x, y);
				auto machine_tile = world_get_machine(game, coord);
				if(machine_tile.id != MACHINE_EMPTY) {
					draw_machine(game, mam_get_ptr(Machine, game_stack, machine_tile.data), coord, machine_tile.id, output);
				}
			}
		}
		TileCoord tile_coord = get_coord_from_screenu(user, screen, input->mouse_pos);
		Tile cur_machine = world_get_machine(game, tile_coord);
		Tile machine = {user->selected_machine, 0};
		bool flag = can_build_machine(game, tile_coord, user->selected_machine);
		if(cur_machine.id == MACHINE_EMPTY) {
			if(machine.id != MACHINE_EMPTY) {
				draw_ghost_machine(game, tile_coord, machine.id, output);
			}
		}
	}

	auto reso_vec = game->reso_vec;
	// print_string(gui, debug_font, gb_vec2(50, 50), from_cstr("hello world"));
	float buff = 16.0;
	char* str;
	uint str_size;
	float spacing = 20.0;
	str = get_sci_from_n(1000*platform->ave_frame_time, 2, &str_size, trans_stack);
	print_string(gui, debug_font, gb_vec2(3, screen.y - buff), from_cstr("frame:"));
	print_string(gui, debug_font, gb_vec2(100, screen.y - buff), str, str_size);
	buff += spacing;

	str = get_string_from_resource(reso_vec[COPPER_ORE], &str_size, trans_stack);
	print_string(gui, debug_font, gb_vec2(3, screen.y - buff), from_cstr("copper ore:"));
	print_string(gui, debug_font, gb_vec2(100, screen.y - buff), str, str_size);
	buff += spacing;
	str = get_string_from_resource(reso_vec[COPPER], &str_size, trans_stack);
	print_string(gui, debug_font, gb_vec2(3, screen.y - buff), from_cstr("copper:"));
	print_string(gui, debug_font, gb_vec2(100, screen.y - buff), str, str_size);
	buff += spacing;
	str = get_string_from_resource(reso_vec[IRON_ORE], &str_size, trans_stack);
	print_string(gui, debug_font, gb_vec2(3, screen.y - buff), from_cstr("iron ore:"));
	print_string(gui, debug_font, gb_vec2(100, screen.y - buff), str, str_size);
	buff += spacing;
	str = get_string_from_resource(reso_vec[IRON], &str_size, trans_stack);
	print_string(gui, debug_font, gb_vec2(3, screen.y - buff), from_cstr("iron:"));
	print_string(gui, debug_font, gb_vec2(100, screen.y - buff), str, str_size);
	buff += spacing;
	str = get_string_from_resource(reso_vec[CIRCUITRY], &str_size, trans_stack);
	print_string(gui, debug_font, gb_vec2(3, screen.y - buff), from_cstr("circuitry:"));
	print_string(gui, debug_font, gb_vec2(100, screen.y - buff), str, str_size);
	buff += spacing;
	print_string(gui, debug_font, gb_vec2(3, screen.y - buff), from_cstr(selected_machine_str));
	/*
	if(user->cmd_text_size > 0) {
		render_text_with_cursor(context, &platform->cmd_font, mam_get_ptr(char, game_heap, user->cmd_text), user->cmd_text_size, user->text_cursor_pos, screen/2, trans_stack);
	} else {
	// render_text(, &platform->cmd_font, from_cstr("Hello Sailor, how are you"), screen/2, trans_stack);
	}
	// render_text(context, &platform->cmd_font, from_cstr("Hello world"), screen/3, trans_stack);
*/
}

void init_game(byte* game_memory, byte* plat_memory, const Input* input) {
	init_func_tables();

	GameState* game = cast(GameState*, game_memory);
	MamStack* game_stack = &game->stack;;

	UserData* user = &game->user;
	auto world = &game->world;

	memzero(game, sizeof(GameState));
	mam_stack_init(game_stack, game->mem_capacity - sizeof(GameState));
	pcg_seed(&game->rng, 12);
	game->heap = mam_stack_pushi(game_stack, GAME_GENMEM_CAPACITY);
	auto game_heap = mam_get_ptr(MamHeap, game_stack, game->heap);
	mam_heap_init(game_heap, GAME_GENMEM_CAPACITY);

	world_init(game);

	auto reso_vec = game->reso_vec;
	memzero(reso_vec, sizeof(double)*RESOS_TOTAL);
	reso_vec[COPPER] += 1000.0;
	reso_vec[IRON] += 1000.0;

	memzero(game->built_machines, sizeof(game->built_machines));

	user->cmd_text_capacity = KILOBYTE;
	user->cmd_text_size = 0;
	user->cmd_text = mam_heap_alloci(game_heap, user->cmd_text_capacity);
	user->text_cursor_pos = 0;
	user->world_pos = gb_vec2(0.0, 0.0);
	user->pixels_per_tile = 32.0;
	user->selected_machine = MACHINE_EMPTY;

	init_platform(game_memory, plat_memory, input);
}


void update_game(byte* game_memory, byte* plat_memory, byte* trans_memory, float dt, const Input* input, Output* output) {
	auto game = cast(GameState*, game_memory);
	auto game_stack = &game->stack;
	auto platform = cast(Platform*, plat_memory);
	auto platform_stack = &platform->stack;
	auto trans_stack = cast(MamStack*, trans_memory);
	mam_stack_init(trans_stack, *cast(uint32*, trans_memory));

	auto user = &game->user;
	auto screen = input->screen;
	auto world = &game->world;
	auto game_heap = mam_get_ptr(MamHeap, game_stack, game->heap);

	for_each_in_range(button, BUTTON_0, BUTTON_9) {
		if(just_down(input, button)) {
			if(button == BUTTON_0) {
				// user->selected_machine = MACHINE_STRUCTURE;
			} else if(button == BUTTON_1) {
				user->selected_machine = user->selected_machine == MACHINE_MINER ? 0 : MACHINE_MINER;
			} else if(button == BUTTON_2) {
				user->selected_machine = user->selected_machine == MACHINE_SMELTER ? 0 : MACHINE_SMELTER;
			} else if(button == BUTTON_3) {
				user->selected_machine = user->selected_machine == MACHINE_FACTORY ? 0 : MACHINE_FACTORY;
			// } else if(button == BUTTON_4) {
			// 	user->selected_machine = MACHINE_COMPUTER;
			}
		}
	}

	{
		TileCoord bottom_left = get_coord_from_screenu(user, screen, ORIGIN);
		TileCoord top_right = get_coord_from_screenu(user, screen, screen);
		generate_world_in_range(game, bottom_left, top_right);
	}

	if(just_down(input, BUTTON_M1)) {
		if(user->selected_machine) {
			TileCoord tile_coord = get_coord_from_screenu(user, screen, input->mouse_pos);
			if(can_build_machine(game, tile_coord, user->selected_machine)) {
				build_machine(game, tile_coord, user->selected_machine);
			}
		} else {
			vec2 pos = get_world_from_screenu(user, screen, input->mouse_pos);
			//figure out if player clicked on a port
			bool port_is_in;
			MachinePort port = get_machine_port_at(game, pos, &port_is_in);
			if(port.machine_data) {
				if(user->port.machine_data && connect_machine_ports(game, user->port, user->port_is_in, port, port_is_in)) {
					user->port = {0, 0};
					user->port_is_in = 0;
				} else {
					user->port = port;
					user->port_is_in = port_is_in;
				}
			}
		}
	}

	game->time_since_last_tick += dt;
	while(game->time_since_last_tick > machine_tick_time) {
		game->time_since_last_tick -= machine_tick_time;
		process_all_machines(game);
	}

	/*
	if(input->text) {
		if(user->cmd_text_size + input->text_size > user->cmd_text_capacity) {
			user->cmd_text_capacity *= 2;
			user->cmd_text = heap_realloc_rel(game_heap, user->cmd_text, user->cmd_text_capacity);
		}
		auto text_buffer = mam_ptr_get(char, game_heap, user->cmd_text);
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
	*/
	update_platform(game, platform, trans_stack, dt, input, output);
}
