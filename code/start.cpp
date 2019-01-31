//By Monica Moniot
#include <stdio.h>
#define GB_MATH_IMPLEMENTATION
#define NK_IMPLEMENTATION
#define BASIC_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#define PCG_IMPLEMENTATION
#include "game.hh"
#include "glew.h"
#undef main

static float get_delta_ms(uint64 t0, uint64 t1) {
	return (1000.0f*(t1 - t0))/SDL_GetPerformanceFrequency();
}

static void input_add_text(Input* input, const char* text, uint32 text_size, Stack* stack) {
	char* new_text;
	if(input->text) {
		stack_extend(stack, text_size);
		new_text = &input->text[input->text_size];
	} else {
		new_text = stack_push(char, stack, text_size);
		input->text = new_text;
	}
	input->text_size += text_size;
	memcpy(new_text, text, text_size);
}
static void input_add_char(Input* input, char ch, Stack* stack) {
	input_add_text(input, &ch, 1, stack);
}


static char* read_file_to_stack(const char* filename, Stack* stack) {
	SDL_RWops* file = SDL_RWFromFile(filename, "r");
	int32 size = SDL_RWsize(file);
	char* buffer = stack_push(char, stack, size);
	SDL_RWread(file, buffer, 1, size);
	return buffer;
}
static int32 compile_shader(char* source, GLenum type) {
	for(uint i = 0;; i += 1) {
		if(source[i]) {
			if(source[i] == '|') {
				source[i] = 0;
				break;
			}
		} else break;
	}
	uint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, 0);
	glCompileShader(shader);
	int succ;
	char log[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &succ);
	if(!succ) {
	    glGetShaderInfoLog(shader, 512, NULL, log);
	    printf("GL ERROR: %s\n", log);
		assert(false);//Failed to compile shader
		return 0;
	}
	return shader;
}
static int32 compile_shader_program_from_file(const char* filename0, const char* filename1, Stack* stack) {
	char* shader_source = read_file_to_stack(filename0, stack);
	uint32 vertex_shader = compile_shader(shader_source, GL_VERTEX_SHADER);
	shader_source = read_file_to_stack(filename1, stack);
	uint32 fragment_shader = compile_shader(shader_source, GL_FRAGMENT_SHADER);

	uint32 shader_program = glCreateProgram();
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);
	glLinkProgram(shader_program);
	int succ;
	glGetProgramiv(shader_program, GL_LINK_STATUS, &succ);
	if(!succ) {
		char log[512];
		glGetProgramInfoLog(shader_program, 512, NULL, log);
		printf("GL ERROR3:%s\n", log);
		assert(false);
	}
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	stack_pop(stack);
	stack_pop(stack);
	return shader_program;
}


int main(int argc, char** argv) {
	if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		//TODO: Handle failure
		printf("Could not initialize SDL: %s.\n", SDL_GetError());
		return -1;
	}


	vec2 screen_dim = gb_vec2(1800, 1000);

	auto window_options = SDL_WINDOW_SHOWN|SDL_WINDOW_OPENGL;//|SDL_WINDOW_RESIZABLE;
	SDL_Window* window = SDL_CreateWindow("EngineOS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screen_dim.w, screen_dim.h, window_options);
	if(!window) {
		printf("Could not create window: %s.\n", SDL_GetError());
		SDL_Quit();
		return -1;
	}

	SDL_GLContext gl_context = SDL_GL_CreateContext(window);

	const unsigned char* version = glGetString(GL_VERSION);
	if(!version) {
		printf("There was an error creating the OpenGL context!\n");
		return 1;
	}
	printf("%s\n", version);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	glewExperimental = GL_TRUE;
	glewInit();


	float ms_per_frame = default_ms_per_frame;
	uint32 updates_per_frame = default_updates_per_frame;
	float update_lag_per_frame = 0;
	int display_index = SDL_GetWindowDisplayIndex(window);
	SDL_DisplayMode dm;
	if(SDL_GetDesktopDisplayMode(display_index, &dm) < 0) {
	    SDL_Log("SDL_GetDesktopDisplayMode failed: %s", SDL_GetError());
	} else {
		ms_per_frame = 1000.0/dm.refresh_rate;
		updates_per_frame = floor(ms_per_frame/ms_per_update);
		update_lag_per_frame = fmod(ms_per_frame, ms_per_update);
	}
	uint32 sleep_resolution_ms = 1;

	byte* game_memory = 0;
	byte* plat_memory = 0;
	byte* trans_memory = 0;
	tape_reserve(&game_memory,  32*MEGABYTE);//TODO: check for failure
	tape_reserve(&plat_memory,  32*MEGABYTE);
	tape_reserve(&trans_memory, 32*MEGABYTE);
	Input game_input = {}; {
		game_input.screen = screen_dim;
	}
	Output game_output = {};
	GLData gl_data; {//init gl
		glClearColor(.1, .1, .1, 0);
		// glClear(GL_COLOR_BUFFER_BIT);
		glViewport(0, 0, screen_dim.w, screen_dim.h);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		auto temp_stack = cast(Stack*, trans_memory);
		stack_init(temp_stack, tape_size(temp_stack));
		gl_data.gui_shader = compile_shader_program_from_file("text_vertex.glsl", "text_fragment.glsl", temp_stack);//--<--
		gl_data.world_graphics_shader = compile_shader_program_from_file("graphics_vertex.glsl", "graphics_fragment.glsl", temp_stack);
		{//init world buffer
			game_output.world_graphics.capacity = MEGABYTE;
			glGenBuffers(1, &gl_data.world_graphics_handle);
			glGenVertexArrays(1, &gl_data.world_graphics_attributes);
			glBindVertexArray(gl_data.world_graphics_attributes);
			glBindBuffer(GL_ARRAY_BUFFER, gl_data.world_graphics_handle);
			glBufferData(GL_ARRAY_BUFFER, game_output.world_graphics.capacity, 0, GL_DYNAMIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6*sizeof(float), cast(void*, 0));
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6*sizeof(float), cast(void*, 2*sizeof(float)));
			// glBindBuffer(GL_ARRAY_BUFFER, 0);
			// glBindVertexArray(0);
		}
		{//init gui buffer
			game_output.gui.capacity = MEGABYTE;
			glGenBuffers(1, &gl_data.gui_handle);
			glGenVertexArrays(1, &gl_data.gui_atrributes);
			glBindVertexArray(gl_data.gui_atrributes);
			glBindBuffer(GL_ARRAY_BUFFER, gl_data.gui_handle);
			glBufferData(GL_ARRAY_BUFFER, game_output.gui.capacity, 0, GL_DYNAMIC_DRAW);
			glEnableVertexAttribArray(0);
		    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), 0);
			// glBindBuffer(GL_ARRAY_BUFFER, 0);
			// glBindVertexArray(0);
		}
 		{//init text texture
			glGenTextures(1, &gl_data.text_texture);
			glBindTexture(GL_TEXTURE_2D, gl_data.text_texture);
			float border_color[] = {0.0f, 0.0f, 0.0f, 0.0f};
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
	}

	init_game(game_memory);
	init_platform(game_memory, plat_memory, game_input);


	uint64 start_of_frame = SDL_GetPerformanceCounter();
	uint64 end_of_compute;
	bool is_game_running = 1;
	float total_update_lag = 0;
	char* text_input = 0;
	while(true) {
		//process events
		game_input.text = 0;
		game_input.text_size = 0;
		game_input.did_mouse_move = 0;
		memzero(&game_input.button_transitions, sizeof(game_input.button_transitions));

		Stack* trans_stack = cast(Stack*, trans_memory);
		stack_init(trans_stack, tape_size(trans_memory));
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			if(event.type == SDL_QUIT) {
				is_game_running = 0;
				break;
			} else if(event.type == SDL_TEXTINPUT) {
				uint32 text_size = strlen(event.text.text);
				if(text_size > 0) {
					input_add_text(&game_input, event.text.text, text_size, trans_stack);
				}
			} else if(event.type == SDL_TEXTEDITING) {
				// printf("e:%d, %d, %s\n", event.edit.start, event.edit.length, event.edit.text);
			} else if(event.type == SDL_MOUSEMOTION) {
				//TODO: find proper fudge
				game_input.mouse_pos = gb_vec2(event.motion.x, game_input.screen.y - event.motion.y - 1);
				game_input.did_mouse_move = 1;
			} else if(event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
				int32 button_id;
				bool flag = 0;
				if(event.button.button == SDL_BUTTON_LEFT) {
					flag = 1;
					button_id = BUTTON_M1;
				} else if(event.button.button == SDL_BUTTON_RIGHT) {
					flag = 1;
					button_id = BUTTON_M2;
				} else if(event.button.button == SDL_BUTTON_MIDDLE) {
					flag = 1;
					button_id = BUTTON_M3;
				}
				if(flag && ((event.button.state == SDL_PRESSED) ^ (game_input.button_state[BUTTON_M1] == 1))) {
					game_input.button_state[button_id] = (event.button.state == SDL_PRESSED);
					game_input.button_transitions[button_id] += 1;
				}
			} else if(event.type == SDL_KEYDOWN) {
				auto scancode = event.key.keysym.scancode;
				auto keycode = event.key.keysym.sym;
				//TODO: Support full suite of keys
				if(keycode == SDLK_ESCAPE) {
					is_game_running = 0;
					break;
				} else if(keycode == SDLK_BACKSPACE) {
					input_add_char(&game_input, CHAR_BACKSPACE, trans_stack);
				} else if(keycode == SDLK_DELETE) {
					input_add_char(&game_input, CHAR_DELETE, trans_stack);
				} else if(keycode == SDLK_TAB) {
					input_add_char(&game_input, CHAR_TAB, trans_stack);
				} else if(keycode == SDLK_RETURN) {//NOTE: there's a SDLK_RETURN2??
					input_add_char(&game_input, CHAR_RETURN, trans_stack);
				} else if(keycode == SDLK_DOWN) {
					input_add_char(&game_input, CHAR_DOWN, trans_stack);
				} else if(keycode == SDLK_UP) {
					input_add_char(&game_input, CHAR_UP, trans_stack);
				} else if(keycode == SDLK_RIGHT) {
					input_add_char(&game_input, CHAR_RIGHT, trans_stack);
				} else if(keycode == SDLK_LEFT) {//NOTE: there's a SDLK_RETURN2??
					input_add_char(&game_input, CHAR_LEFT, trans_stack);
				}
			}

		}
		if(!is_game_running) break;

		// run game
		uint32 ticks = updates_per_frame;
		total_update_lag += update_lag_per_frame;
		if(total_update_lag >= 1) {
			total_update_lag -= 1;
			ticks += 1;
		}

		{//prepare vertex buffers
			glBindBuffer(GL_ARRAY_BUFFER, gl_data.world_handle);
			game_output.world_graphics.buffer = cast(float*, glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
			game_output.world_graphics.size = 0;
			glBindBuffer(GL_ARRAY_BUFFER, gl_data.gui_handle);
			game_output.gui.buffer = cast(float*, glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
			game_output.gui.size = 0;
		}
		update_game(game_memory, plat_memory, &trans_memory[trans_stack->size], updates_per_frame, &game_input);
		update_platform(game_memory, plat_memory, &trans_memory[trans_stack->size], updates_per_frame, &game_input, &game_output);

		end_of_compute = SDL_GetPerformanceCounter();
		float time_to_compute = get_delta_ms(start_of_frame, end_of_compute);
		uint64 end_of_frame;
		if(time_to_compute < ms_per_frame) {
			uint32 time_to_wait = cast(uint32, ms_per_frame - time_to_compute);
			if(time_to_wait > sleep_resolution_ms) {//TODO: decide sleep resolution
				SDL_Delay(time_to_wait - sleep_resolution_ms);
			}
			end_of_frame = SDL_GetPerformanceCounter();
			//TODO: make get_delta faster?
			while(get_delta_ms(start_of_frame, end_of_frame) < ms_per_frame) {
				end_of_frame = SDL_GetPerformanceCounter();
			}
		} else {
			//TODO: framerate manipulation
			// printf("frame took longer than expected: %2.2f", time_to_compute);
			end_of_frame = end_of_compute;
		}
		// printf("%2.2f\n", time_to_compute);
		start_of_frame = end_of_frame;

		{//draw vertex buffer
			glClear(GL_COLOR_BUFFER_BIT);

			glUseProgram(gl_data.world_graphics_shader);
			glBindVertexArray(gl_data.world_graphics_attributes);
			assert(glUnmapBuffer(GL_ARRAY_BUFFER) == GL_TRUE);//could fail??
			glDrawArrays(GL_TRIANGLES, 0, game_output.world_graphics.size);

			glUseProgram(gl_data.gui_shader);
			glBindVertexArray(gl_data.gui_attributes);
			assert(glUnmapBuffer(GL_ARRAY_BUFFER) == GL_TRUE);//could fail??
			glDrawArrays(GL_TRIANGLES, 0, game_output.gui.size);
			SDL_GL_SwapWindow(platform->window);
			gl_check();
		}

		// #ifdef DEBUG
		memzero(trans_memory, tape_size(trans_memory));//only for debugging, prevents transient data from being using between frames
		// #endif
	}

	//only program exit point
	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
