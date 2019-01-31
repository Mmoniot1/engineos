//By Monica Moniot
#include "game.hh"
#include <stdio.h>


static const float SQUARE_VERTICES[] = {
	1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
};


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

static void push_vertex(VBuffer* vertexes, vec2 pos) {
	*cast(vec2*, &vertexes->buffer[vertexes->size]) = pos;
	vertexes->size += 2;
	assert(vertexes->size <= vertexes->capacity);
}
static void push_color(VBuffer* vertexes, vec4 pos) {
	*cast(vec4*, &vertexes->buffer[vertexes->size]) = pos;
	vertexes->size += 4;
	assert(vertexes->size <= vertexes->capacity);
}
static void gui_draw_line(VBuffer* vertexes, vec2 pos0, vec2 pos1) {
	static vec2 line_epsilon = gb_vec2(1.0, 1.0);
	push_vertex(vertexes, pos0);
	push_vertex(vertexes, pos1);
	push_vertex(vertexes, pos0 + line_epsilon);
	push_vertex(vertexes, pos1 + line_epsilon);
	push_vertex(vertexes, pos1);
	push_vertex(vertexes, pos0 + line_epsilon);
}
static void draw_box(VBuffer* vertexes, vec2 pos0, vec2 pos1, vec4 color) {
	push_vertex(vertexes, pos0);
	push_color(vertexes, color);
	push_vertex(vertexes, pos1);
	push_color(vertexes, color);
	push_vertex(vertexes, gb_vec2(pos0.x, pos1.y));
	push_color(vertexes, color);
	push_vertex(vertexes, pos0);
	push_color(vertexes, color);
	push_vertex(vertexes, pos1);
	push_color(vertexes, color);
	push_vertex(vertexes, gb_vec2(pos1.x, pos0.y));
	push_color(vertexes, color);
}

static void render_text_with_cursor(GLContext* context, const Font* font, const char* text, uint32 size, uint32 cursor, vec2 pos, Stack* stack) {
	auto info = &font->info;
	int ascent;
	int descent;
	float scale = stbtt_ScaleForPixelHeight(info, font->pixel_size);
	stbtt_GetFontVMetrics(info, &ascent, &descent, 0);
	int baseline = cast(int, ascent*scale);

	float cur_pos = 0;
	float cursor_pos = 0;
	uint32 max_width = 0;
	for_each_lt(i, size) {
		if(cursor == i) {
			cursor_pos = cur_pos;
		}
		int advance;
		int x0, y0, x1, y1;
		stbtt_GetCodepointHMetrics(info, text[i], &advance, 0);
		if(i + 1 < size) {
			cur_pos += advance*scale;
			cur_pos += scale*stbtt_GetCodepointKernAdvance(info, text[i], text[i + 1]);
		} else {
			uint cur_pos_i = floor(cur_pos);
			float offset = cur_pos - cur_pos_i;
			stbtt_GetCodepointBitmapBoxSubpixel(info, text[i], scale, scale, offset, 0, &x0, &y0, &x1, &y1);
			max_width = (x1 - x0) - 1 + cur_pos_i + x0;
			if(cursor == size) {
				cursor_pos = cur_pos;
				cursor_pos += advance*scale;
			}
		}
	}

	uint32 bitmap_width = max_width + 1;
	uint32 bitmap_height = cast(uint32, floor(scale*(ascent - descent))) + 1;
	auto bitmap = stack_push(uint32, stack, bitmap_height*bitmap_width);
	memzero(bitmap, sizeof(uint32)*bitmap_height*bitmap_width);

	cur_pos = 0;
	for_each_lt(i, size) {//TODO: translate characters to unicode codepoints
		int advance;
		int x0, y0, x1, y1;
		stbtt_GetCodepointHMetrics(info, text[i], &advance, 0);
		uint cur_pos_i = floor(cur_pos);
		float offset = cur_pos - cur_pos_i;
		stbtt_GetCodepointBitmapBoxSubpixel(info, text[i], scale, scale, offset, 0, &x0, &y0, &x1, &y1);

		uint32 glyph_width = x1 - x0;
		uint32 glyph_height = y1 - y0;
		uint32 glyph_size = glyph_width*glyph_height;
		uint8* glyph = stack_push(uint8, stack, glyph_size);
		memzero(glyph, glyph_size);
		stbtt_MakeCodepointBitmapSubpixel(info, glyph, glyph_width, glyph_height, glyph_width, scale, scale, offset, 0, text[i]);
		// printf("g: %d, %d\n", glyph_width, glyph_height);
		// printf("b: %d, %d\n", bitmap_width, bitmap_height);

		for_each_lt(y, glyph_height) {
			for_each_lt(x, glyph_width) {
				uint32* target = &bitmap[bitmap_width*(bitmap_height - (y + baseline + y0)) + x + cur_pos_i + x0];
				uint8 cur = glyph[glyph_width*y + x];
				uint8 next = max(cur, *target);
				*target = (next<<24)|(next<<16)|(next<<8)|(next);
			}
		}

		cur_pos += advance*scale;
		if(i + 1 < size) {
			cur_pos += scale*stbtt_GetCodepointKernAdvance(info, text[i], text[i + 1]);
		}
		stack_pop(stack);
	}

	gui_draw_line(&context->gui, pos + gb_vec2(cursor_pos, scale*ascent - scale*descent), pos + gb_vec2(cursor_pos, -scale*descent));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, context->text_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bitmap_width, bitmap_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap);

	glUseProgram(context->text_program);
	glUniform1i(glGetUniformLocation(context->text_program, "image"), 0);
	mat4 trans;
	gb_mat4_identity(&trans);
	gb_mat4_translate(&trans, gb_vec3(pos.x*context->units_per_pixel.w - 1.0, pos.y*context->units_per_pixel.h - 1.0, 0));
	gb_mat4_scale(&trans, gb_vec3(bitmap_width*context->units_per_pixel.w, bitmap_height*context->units_per_pixel.h, 0));

	glUniformMatrix4fv(glGetUniformLocation(context->text_program, "projection"), 1, GL_FALSE, cast(float*, &trans.e));

	glBindVertexArray(context->text_attributes);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	stack_pop(stack);
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

extern "C" {
void init_platform(byte* game_memory, byte* plat_memory, const Input* input) {
	auto game = cast(GameState*, game_memory);
	auto game_stack = cast(Stack*, game_memory + sizeof(GameState));
	auto platform = cast(Platform*, plat_memory);
	auto platform_stack = cast(Stack*, plat_memory + sizeof(Platform));

	memzero(platform, sizeof(Platform));
	stack_init(platform_stack, tape_size(plat_memory) - sizeof(Platform));

	stbtt_InitFont(&platform->cmd_font.info, cast(unsigned char*, read_file_to_stack("NotoMono.ttf", platform_stack)), 0);
	platform->cmd_font.pixel_size = 24;
}

void update_platform(byte* game_memory, byte* plat_memory, Stack* trans_stack, uint32 dt, Input* input, Output* output) {
	auto game = cast(const GameState*, game_memory);
	auto game_stack = cast(const Stack*, game_memory + sizeof(GameState));
	auto platform = cast(Platform*, plat_memory);
	auto platform_stack = cast(Stack*, plat_memory + sizeof(Platform));
	auto trans_stack = cast(Stack*, trans_memory);

	auto user = &game->user;
	auto context = &platform->gl_context;
	auto screen = platform->screen;
	auto game_slab = ptr_get(Slab, game_stack, game->slab);

	auto world = &game->world;
	auto gui = &output->gui;
	auto graphics = &output->world_graphics;

	{
		vec4 background_color = {.5, .5, .5, 1.0};
		TileCoord bottom_left = get_coord_from_screen(user, ORIGIN);
		TileCoord top_right = get_coord_from_screen(user, input->screen);
		draw_box(graphics, gb_vec2(bottom_left.x - .5, bottom_left.y - .5), gb_vec2(top_right.x + .5, top_right.y + .5), background_color);
		for_each_in_range(y, bottom_left.y, top_right.y) {
			for_each_in_range(x, bottom_left.x, top_right.x) {

				auto tile = get_tile(world, to_coord(x, y), game_stack);
				if(tile.id == TILE_SOMETHING) {
					vec4 color = {.5, .1, .5, 1.0};
					draw_box(graphics, gb_vec2(x - .5, y - .5), gb_vec2(x + .5, y + .5), color);
				} else if(tile.id == TILE_EMPTY) {
					vec4 color = {.1, .5, .5, 1.0};
					draw_box(graphics, gb_vec2(x - .5, y - .5), gb_vec2(x + .5, y + .5), color);
				}
			}
		}
	}

	/*
	if(user->cmd_text_size > 0) {
		render_text_with_cursor(context, &platform->cmd_font, ptr_get(char, game_slab, user->cmd_text), user->cmd_text_size, user->text_cursor_pos, screen/2, trans_stack);
	} else {
	// render_text(, &platform->cmd_font, from_cstr("Hello Sailor, how are you"), screen/2, trans_stack);
	}
	// render_text(context, &platform->cmd_font, from_cstr("Hello world"), screen/3, trans_stack);
*/
}

}
