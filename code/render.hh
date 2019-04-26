//By Monica Moniot
#ifndef RENDER__H_INCLUDE
#define RENDER__H_INCLUDE

#include "game.hh"


void push_vertex(VBuffer* vertexes, vec2 pos, vec4 color, vec2 texture);
void draw_box_textured(VBuffer* vertexes, vec2 pos0, vec2 pos1, vec4 color, vec2 texture0, vec2 texture1);

void draw_box(VBuffer* vertexes, vec2 pos0, vec2 pos1, vec4 color);
void draw_tile(VBuffer* vertexes, TileCoord pos, vec4 color);

void print_string_colored(VBuffer* vertexes, Font* font, vec2 pos, const char* str, uint32 str_size, vec4 color);

inline void print_string(VBuffer* vertexes, Font* font, vec2 pos, const char* str, uint32 str_size) {
	print_string_colored(vertexes, font, pos, str, str_size, COLOR_WHITE);
}

char* get_sci_from_n(double n, uint32 figs, uint32* ret_size, MamStack* stack);

char* get_string_from_resource(double n, uint32* ret_size, MamStack* stack);

#endif

#ifdef RENDER_IMPLEMENTATION
#undef RENDER_IMPLEMENTATION


static const float SQUARE_VERTICES[] = {
	1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
};

void push_vertex(VBuffer* vertexes, vec2 pos, vec4 color, vec2 texture) {
	*cast(vec2*, &vertexes->buffer[vertexes->size]) = pos;
	vertexes->size += 2;
	*cast(vec4*, &vertexes->buffer[vertexes->size]) = color;
	vertexes->size += 4;
	*cast(vec2*, &vertexes->buffer[vertexes->size]) = texture;
	vertexes->size += 2;
}
void draw_triangle(VBuffer* vertexes, vec2 pos0, vec2 pos1, vec2 pos2, vec4 color) {
	push_vertex(vertexes, pos0, color, NEG);
	push_vertex(vertexes, pos1, color, NEG);
	push_vertex(vertexes, pos2, color, NEG);
}
// static void gui_draw_line(VBuffer* vertexes, vec2 pos0, vec2 pos1) {
// 	static vec2 line_epsilon = gb_vec2(1.0, 1.0);
// 	push_vertex(vertexes, pos0);
// 	push_vertex(vertexes, pos1);
// 	push_vertex(vertexes, pos0 + line_epsilon);
// 	push_vertex(vertexes, pos1 + line_epsilon);
// 	push_vertex(vertexes, pos1);
// 	push_vertex(vertexes, pos0 + line_epsilon);
// }
void draw_box_textured(VBuffer* vertexes, vec2 pos0, vec2 pos1, vec4 color, vec2 texture0, vec2 texture1) {
	push_vertex(vertexes, pos0, color, texture0);
	push_vertex(vertexes, pos1, color, texture1);
	push_vertex(vertexes, gb_vec2(pos0.x, pos1.y), color, gb_vec2(texture0.x, texture1.y));
	push_vertex(vertexes, pos0, color, texture0);
	push_vertex(vertexes, pos1, color, texture1);
	push_vertex(vertexes, gb_vec2(pos1.x, pos0.y), color, gb_vec2(texture1.x, texture0.y));
}

void draw_line(VBuffer* vertexes, vec2 pos0, vec2 pos1, float thickness, vec4 color) {
	vec2 perp = gb_vec2(-(pos1.y - pos0.y), pos1.x - pos0.x);
	float l = sqrtf(perp.x*perp.x + perp.y*perp.y);
	perp = (thickness/l/2)*perp;

	vec2 c0 = pos0 + perp;
	vec2 c1 = pos0 - perp;
	vec2 c2 = pos0 + perp;
	vec2 c3 = pos0 - perp;
	draw_triangle(vertexes, c0, c1, c2, color);
	draw_triangle(vertexes, c1, c2, c3, color);
}

void draw_box(VBuffer* vertexes, vec2 pos0, vec2 pos1, vec4 color) {
	draw_box_textured(vertexes, pos0, pos1, color, NEG, NEG);
}
void draw_tile(VBuffer* vertexes, TileCoord pos, vec4 color) {
	draw_box(vertexes, gb_vec2(pos.x - .5, pos.y - .5), gb_vec2(pos.x + .5, pos.y + .5), color);
}
void draw_circle(VBuffer* vertexes, vec2 center, float radius, vec4 color) {
	constexpr float CIRCLE_RESOLUTION = 16.0;
	vec2 cur = gb_vec2(radius + center.x, center.y);
	float angle = 2*PI/CIRCLE_RESOLUTION;
	for_each_lt(_, 16) {
		vec2 next = gb_vec2(radius*cosf(angle) + center.x, radius*sinf(angle) + center.y);
		draw_triangle(vertexes, cur, next, center, color);
		cur = next;
		angle += 2*PI/CIRCLE_RESOLUTION;
	}
}


void print_string_colored(VBuffer* vertexes, Font* font, vec2 pos, const char* str, uint32 str_size, vec4 color) {
	pos.y = -pos.y;
	for_each_in(ch, str, str_size) {
		stbtt_aligned_quad q;
		stbtt_GetBakedQuad(font->char_data, font->bitmap_w, font->bitmap_h, *ch - 32, &pos.x, &pos.y, &q, 1);
		draw_box_textured(vertexes, gb_vec2(q.x0, -q.y1), gb_vec2(q.x1, -q.y0), color, gb_vec2(q.s0, q.t1), gb_vec2(q.s1, q.t0));
	}
}

/*
static char* read_file_to_stack(const char* filename, MamStack* stack) {
	SDL_RWops* file = SDL_RWFromFile(filename, "r");
	int32 size = SDL_RWsize(file);
	char* buffer = mam_stack_push(char, stack, size);
	SDL_RWread(file, buffer, 1, size);
	return buffer;
}
*/

char* get_sci_from_n(double n, uint32 figs, uint32* ret_size, MamStack* stack) {
	char* str;
	uint str_size;
	bool is_neg;
	if(n < -EPSILON) {
		is_neg = 1;
		n = -n;
	} else if(n > EPSILON) {
		is_neg = 0;
	} else {
		str_size = 1;
		str = mam_stack_push(char, stack, str_size);
		str[0] = '0';
		*ret_size = str_size;
		return str;
	}
	int32 scale = mam_floor_to_int32(mam_log10(n));
	double adjusted = mam_pow10(-scale)*n;
	int32 num_to_write = mam_floor_to_int32(mam_pow10(figs)*adjusted);
	bool scale_is_neg;
	if(scale < 0) {
		scale_is_neg = 1;
		scale = -scale;
	} else if (scale > 0) {
		scale_is_neg = 0;
	} else {
		str_size = figs + 2 + is_neg;
		str = mam_stack_push(char, stack, str_size);
		int32 i = 0;
		if(is_neg) {
			str[i] = '-';
			i += 1;
		}
		int32 num = num_to_write;
		for_each_in_range_bw(j, i + 2, i + figs + 1) {
			char c = '0' + (num%10);
			num = num/10;
			str[j] = c;
		}
		str[i] = '0' + (num%10);
		i += 1;
		str[i] = '.';

		*ret_size = str_size;
		return str;
	}

	int32 scale_size = mam_floor_to_int32(mam_log10f(scale)) + 1;
	str_size = figs + 3 + scale_size + is_neg + scale_is_neg;
	str = mam_stack_push(char, stack, str_size);
	int32 i = 0;
	if(is_neg) {
		str[i] = '-';
		i += 1;
	}
	int32 num = num_to_write;
	for_each_in_range_bw(j, i + 2, i + figs + 1) {
		char c = '0' + (num%10);
		num = num/10;
		str[j] = c;
	}
	str[i] = '0' + (num%10);
	i += 1;
	str[i] = '.';
	i += 1 + figs;
	str[i] = 'e';
	i += 1;
	if(scale_is_neg) {
		str[i] = '-';
		i += 1;
	}
	num = scale;
	for_each_in_range_bw(j, i, i + scale_size - 1) {
		char c = '0' + (num%10);
		num = num/10;
		str[j] = c;
	}

	*ret_size = str_size;
	return str;
}
char* get_string_from_resource(double n, uint32* ret_size, MamStack* stack) {
	bool is_neg = 0;
	if(n < -EPSILON) {
		is_neg = 1;
		n = -n;
	}
	char* str;
	uint str_size;
	int32 num_to_write;
	char unit;
	bool has_unit = 1;
	if(n < 1.0) {
		str_size = 2;
		str = mam_stack_push(char, stack, str_size);
		str[0] = '0';
		str[1] = 'g';
		*ret_size = str_size;
		return str;
	// } else if(n < 1.0) {
	// 	num_to_write = mam_floor_to_int32(mam_pow10(3)*n);
	// 	has_unit = 0;
	} else if(n < mam_pow10(6)) {
		num_to_write = mam_floor_to_int32(n);
		unit = 'k';
	} else if(n < mam_pow10(9)) {
		num_to_write = mam_floor_to_int32(n/mam_pow10(3));
		unit = 'M';
	} else if(n < mam_pow10(12)) {
		num_to_write = mam_floor_to_int32(n/mam_pow10(6));
		unit = 'G';
	} else if(n < mam_pow10(15)) {
		num_to_write = mam_floor_to_int32(n/mam_pow10(9));
		unit = 'T';
	} else {
		str_size = 2;
		str = mam_stack_push(char, stack, str_size);
		str[0] = '1';
		str[1] = 'X';
		*ret_size = str_size;
		return str;
	}
	int32 num = num_to_write;
	int32 i = 0;//TODO: log10
	while(num > 0) {
		num = num/10;
		i += 1;
	}
	i += is_neg;
	str_size = i + has_unit + 1;
	str = mam_stack_push(char, stack, str_size);
	if(is_neg) {
		str[0] = '-';
	}
	if(has_unit) {
		str[i] = unit;
		str[i + 1] = 'g';
	} else {
		str[i] = 'g';
	}

	i -= 1;
	num = num_to_write;
	while(num > 0) {
		char c = '0' + (num%10);
		num = num/10;
		str[i] = c;
		i -= 1;
	}
	*ret_size = str_size;
	return str;
}

#endif
