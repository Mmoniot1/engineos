//By Monica Moniot
#include "game.hh"
#include "glew.h"
#undef main

static double get_delta_time(uint64 t0, uint64 t1) {
	return (t1 - t0)/cast(double, SDL_GetPerformanceFrequency());
}


// static void input_add_text(Input* input, const char* text, uint32 text_size, MamStack* stack) {
// 	char* new_text;
// 	if(input->text) {
// 		mam_stack_extend(stack, text_size);
// 		new_text = &input->text[input->text_size];
// 	} else {
// 		new_text = mam_stack_push(char, stack, text_size);
// 		input->text = new_text;
// 	}
// 	input->text_size += text_size;
// 	memcpy(new_text, text, text_size);
// }
// static void input_add_char(Input* input, char ch, MamStack* stack) {
// 	input_add_text(input, &ch, 1, stack);
// }
static void input_press_button(Input* input, ButtonId id, bool button_state) {
	if(input->button_state[id] != button_state) {
		input->button_state[id] = button_state;
		input->button_trans[id] += 1;
	}
}


static bool gl_check_(const char *file, int line) {
    auto code = glGetError();
    if(code != GL_NO_ERROR) {
        const char* error;
        switch (code) {
            case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
            case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
            case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }
		printf("gl error occurred: %s, check occurred in %s at line %d\n", error, file, line);
		return false;
    }
	return true;
}
#define gl_check() assert(gl_check_(__FILE__, __LINE__))


static char* read_file_to_stack(const char* filename, MamStack* stack) {
	SDL_RWops* file = SDL_RWFromFile(filename, "r");
	int32 size = SDL_RWsize(file);
	char* buffer = mam_stack_push(char, stack, size);
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
static uint32 compile_shader_program_from_file(const char* filename0, const char* filename1, MamStack* stack) {
	int32 pre_stack_size = stack->size;
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
		printf("GL ERROR3: %s\n", log);
		assert(false);
	}
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	mam_stack_set_size(stack, pre_stack_size);
	return shader_program;
}

// static uint32 init_font(Font* font, float psize, const char* filename, MamStack* stack) {
// 	//leaves char data behind on stack
// 	//TODO: Better font code
// 	char first_char = 32;
// 	char last_char = 126;
// 	font->bitmap_w = KILOBYTE;
// 	font->bitmap_h = KILOBYTE;
// 	font->char_data = mam_stack_push(stbtt_bakedchar, stack, last_char - first_char + 1);
// 	uint8* data = cast(uint8*, read_file_to_stack("NotoSans.ttf", stack));
// 	font->pixel_size = psize;
// 	byte* temp_bitmap = mam_stack_push(byte, stack, font->bitmap_w*font->bitmap_h);
// 	ASSERT(stbtt_BakeFontBitmap(data, 0, psize, temp_bitmap, font->bitmap_w, font->bitmap_h, first_char, last_char - first_char + 1, font->char_data) > 0); //no guarantee this fits!
//
// 	uint32 ret;
// 	glGenTextures(1, &ret);
// 	glBindTexture(GL_TEXTURE_2D, ret);
// 	float border_color[] = {0.0f, 0.0f, 0.0f, 1.0f};
// 	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
// 	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, font->bitmap_w, font->bitmap_h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, temp_bitmap);
//
// 	mam_stack_pop(stack);
// 	return ret;
// }

int main(int argc, char** argv) {
	if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		//TODO: Handle failure
		printf("Could not initialize SDL: %s.\n", SDL_GetError());
		return -1;
	}


 	auto screen_dim = gb_vec2(1200, 800);

	auto window_options = SDL_WINDOW_SHOWN|SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE;
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
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	glewExperimental = GL_TRUE;
	glewInit();


	double time_per_frame = default_time_per_frame;
	int display_index = SDL_GetWindowDisplayIndex(window);
	SDL_DisplayMode dm;
	//TODO: handle change in monitor
	if(SDL_GetDesktopDisplayMode(display_index, &dm) < 0) {
	    printf("SDL_GetDesktopDisplayMode failed: %s\n", SDL_GetError());
	} else {
		time_per_frame = 1.0/dm.refresh_rate;
	}
	double sleep_resolution = 0.0008;


	GameState game_state = {};

	MamStack* trans_stack = mam_stack_init(malloc(300*MEGABYTE), 300*MEGABYTE);

	Input input = {};

	Output output = {};
	input.screen = screen_dim;


	GLData gl_data; {//init gl
		glClearColor(1.0, 1.0, 0, 0);
		glViewport(0, 0, screen_dim.w, screen_dim.h);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		gl_data.vbuffer_shader = compile_shader_program_from_file("graphics_vertex.glsl", "graphics_fragment.glsl", trans_stack);
		gl_data.vbuffer_trans_handle = glGetUniformLocation(gl_data.vbuffer_shader, "projection");
		{//init world buffer
			output.vbuffer.capacity = 32*MEGABYTE;
			glGenBuffers(1, &gl_data.vbuffer_handle);
			glGenVertexArrays(1, &gl_data.vbuffer_attributes);
			glBindVertexArray(gl_data.vbuffer_attributes);
			glBindBuffer(GL_ARRAY_BUFFER, gl_data.vbuffer_handle);
			glBufferData(GL_ARRAY_BUFFER, output.vbuffer.capacity, 0, GL_DYNAMIC_DRAW);
			gl_check();
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, FLOATS_PER_VBUFFER_VERTEX*sizeof(float), cast(void*, 0));
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, FLOATS_PER_VBUFFER_VERTEX*sizeof(float), cast(void*, 2*sizeof(float)));
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, FLOATS_PER_VBUFFER_VERTEX*sizeof(float), cast(void*, 4*sizeof(float)));
			gl_check();
		}
		glGenTextures(1, &gl_data.texture);
 		{//init text texture
			// gl_data.mono_texture = init_font(&output.sans, 24, "NotoSans.ttf", trans_stack);
		}
	}

	init_game(&game_state, trans_stack, &input);

	uint64 start_of_frame = SDL_GetPerformanceCounter();
	uint64 start_of_compute = start_of_frame;
	bool is_game_running = 1;
	char* text_input = 0;

	int64 debug_ticks = 0;
	while(true) {
		uint64 new_start = SDL_GetPerformanceCounter();
		double frame_duration = get_delta_time(start_of_compute, new_start);
		start_of_compute = new_start;
		//process events
		// input.text = 0;
		// input.text_size = 0;
		input.did_mouse_move = 0;
		// input.did_screen_resize = 0;
		input.pre_screen = GBORIGIN;
		memzero(&input.button_trans, 1);

		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			if(event.type == SDL_QUIT) {
				is_game_running = 0;
				break;
			} else if(event.type == SDL_TEXTINPUT) {
				// uint32 text_size = strlen(event.text.text);
				// if(text_size > 0) {
				// 	input_add_text(&input, event.text.text, text_size, startup_stack);
				// }
			} else if(event.type == SDL_TEXTEDITING) {
				// printf("e:%d, %d, %s\n", event.edit.start, event.edit.length, event.edit.text);
			} else if(event.type == SDL_MOUSEMOTION) {
				//TODO: find proper fudge
				input.mouse_pos = gb_vec2(event.motion.x, input.screen.y - event.motion.y - 1);
				input.did_mouse_move = 1;
			} else if(event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
				// printf("%f, %f\n", input.mouse_pos.x, input.mouse_pos.y);
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
				if(flag && ((event.button.state == SDL_PRESSED) ^ (input.button_state[BUTTON_M1] == 1))) {
					input.button_state[button_id] = (event.button.state == SDL_PRESSED);
					input.button_trans[button_id] += 1;
				}
			} else if(event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
				auto scancode = event.key.keysym.scancode;
				auto keycode = event.key.keysym.sym;
				bool state = (event.key.state == SDL_PRESSED);
				bool is_repeat = (event.key.repeat > 0);
				//TODO: Support full suite of keys
				bool flag = 0;
				if(state) {
					if(keycode == SDLK_ESCAPE) {
						is_game_running = 0;
						break;
					// } else if(keycode == SDLK_BACKSPACE) {
					// 	input_add_char(&input, CHAR_BACKSPACE, startup_stack);
					// } else if(keycode == SDLK_DELETE) {
					// 	input_add_char(&input, CHAR_DELETE, startup_stack);
					// } else if(keycode == SDLK_TAB) {
					// 	input_add_char(&input, CHAR_TAB, startup_stack);
					// } else if(keycode == SDLK_RETURN) {
					// 	//NOTE: there's a SDLK_RETURN2??
					// 	input_add_char(&input, CHAR_RETURN, startup_stack);
					// } else if(keycode == SDLK_DOWN) {
					// 	input_add_char(&input, CHAR_DOWN, startup_stack);
					// } else if(keycode == SDLK_UP) {
					// 	input_add_char(&input, CHAR_UP, startup_stack);
					// } else if(keycode == SDLK_RIGHT) {
					// 	input_add_char(&input, CHAR_RIGHT, startup_stack);
					// } else if(keycode == SDLK_LEFT) {
					// 	//NOTE: there's a SDLK_RETURN2??
					// 	input_add_char(&input, CHAR_LEFT, startup_stack);
					} else {
						flag = 1;
					}
				} else {
					flag = 1;
				}
				if(flag) {
					if(!is_repeat) {
						if(keycode == SDLK_LEFT) {
							input_press_button(&input, BUTTON_LEFT, state);
						} else if(keycode == SDLK_RIGHT) {
							input_press_button(&input, BUTTON_RIGHT, state);
						} else if(keycode == SDLK_UP) {
							input_press_button(&input, BUTTON_UP, state);
						} else if(keycode == SDLK_DOWN) {
							input_press_button(&input, BUTTON_DOWN, state);
						} else if(keycode == SDLK_LSHIFT) {
							input_press_button(&input, BUTTON_LEFT_SHIFT, state);
						} else if(keycode == SDLK_LCTRL) {
							input_press_button(&input, BUTTON_LEFT_CTRL, state);
						} else if(keycode == SDLK_0) {
							input_press_button(&input, BUTTON_0, state);
						} else if(keycode == SDLK_1) {
							input_press_button(&input, BUTTON_1, state);
						} else if(keycode == SDLK_2) {
							input_press_button(&input, BUTTON_2, state);
						} else if(keycode == SDLK_3) {
							input_press_button(&input, BUTTON_3, state);
						} else if(keycode == SDLK_4) {
							input_press_button(&input, BUTTON_4, state);
						} else if(keycode == SDLK_5) {
							input_press_button(&input, BUTTON_5, state);
						} else if(keycode == SDLK_6) {
							input_press_button(&input, BUTTON_6, state);
						} else if(keycode == SDLK_7) {
							input_press_button(&input, BUTTON_7, state);
						} else if(keycode == SDLK_8) {
							input_press_button(&input, BUTTON_8, state);
						} else if(keycode == SDLK_9) {
							input_press_button(&input, BUTTON_9, state);
						}
					}
				}
			} else if (event.type == SDL_WINDOWEVENT) {
				auto window_id = event.window.event;
				if(window_id == SDL_WINDOWEVENT_SIZE_CHANGED) {
					if(input.pre_screen == GBORIGIN) {
						input.pre_screen = input.screen;
					}
					input.screen = gb_vec2(event.window.data1, event.window.data2);
					//correct opengl
					glViewport(0, 0, input.screen.w, input.screen.h);
					//correct mouse
					int32 mouse_x, mouse_y;
					SDL_GetMouseState(&mouse_x, &mouse_y);
					input.mouse_pos.x = cast(float, mouse_x);
					input.mouse_pos.y = cast(float, mouse_y);
				} else if (window_id == SDL_WINDOWEVENT_ENTER) {//TODO: handle input lossing focus?
				} else if (window_id == SDL_WINDOWEVENT_LEAVE) {
				} else if (window_id == SDL_WINDOWEVENT_FOCUS_GAINED) {
				} else if (window_id == SDL_WINDOWEVENT_FOCUS_LOST) {
				}
			}
		}
		if(!is_game_running) break;

		{//prepare vertex buffers
			glBindBuffer(GL_ARRAY_BUFFER, gl_data.vbuffer_handle);
			output.vbuffer.buffer = cast(float*, glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
			output.vbuffer.size = 0;
		}
		//run game
		update_game(&game_state, trans_stack, frame_duration, &input, &output);
		// input_pop_text(&input, startup_stack);

		{//draw vertex buffer
			// printf("size: %d\n", output.world_graphics.size);
			glClear(GL_COLOR_BUFFER_BIT);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, gl_data.texture);
			gl_check();
			float border_color[] = {1.0f, 1.0f, 1.0f, 1.0f};
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, output.next_texture.x, output.next_texture.y, 0, GL_RGBA, GL_FLOAT, output.next_texture.buffer);
			gl_check();

			glBindBuffer(GL_ARRAY_BUFFER, gl_data.vbuffer_handle);
			glUnmapBuffer(GL_ARRAY_BUFFER);//could fail??
			gl_check();
			glUseProgram(gl_data.vbuffer_shader);
			glUniformMatrix3fv(gl_data.vbuffer_trans_handle, 1, GL_TRUE, cast(float*, &output.vbuffer.trans));
			glBindVertexArray(gl_data.vbuffer_attributes);
			glDrawArrays(GL_TRIANGLES, 0, output.vbuffer.size/FLOATS_PER_VBUFFER_VERTEX);
			gl_check();
		}
		{//control framerate
			debug_ticks += 1;
			uint64 end_of_compute = SDL_GetPerformanceCounter();
			double time_to_compute = get_delta_time(start_of_frame, end_of_compute);
			if(debug_ticks%128 == 0) {
				printf("compute time: %2.5fms\n", time_to_compute*1000.0);
			}
			if(time_to_compute < time_per_frame) {
				double time_to_wait = time_per_frame - time_to_compute;
				if(time_to_wait > sleep_resolution) {
					SDL_Delay(cast(uint32, 1000.0*(time_to_wait - sleep_resolution)));
				}
				uint64 end_of_frame = SDL_GetPerformanceCounter();
				//TODO: make get_delta faster?
				int64 ticks_per_frame = cast(int64, gb_round(time_per_frame*SDL_GetPerformanceFrequency()));
				while((end_of_frame - start_of_frame) < ticks_per_frame) {
					end_of_frame = SDL_GetPerformanceCounter();
				}
				start_of_frame += ticks_per_frame;
			} else {
				//TODO: framerate manipulation
				printf("frame took longer than expected: %2.3f\n", time_to_compute);
				start_of_frame = end_of_compute;
			}
			// printf("%2.3f\n", time_to_compute);
		}
		//flip frame
		SDL_GL_SwapWindow(window);

		// #ifdef DEBUG
			mam_stack_set_size(trans_stack, 0);
		// #endif
	}

	//only program exit point
	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
