//By Monica Moniot
#ifndef MAMLIB__H_INCLUDE
#define MAMLIB__H_INCLUDE

#ifdef __cplusplus
 #ifdef MAMLIB_STATIC
  #define MAMLIB__DECLR extern "C" static
 #else
  #define MAMLIB__DECLR extern "C"
 #endif
#else
 #ifdef MAMLIB_STATIC
  #define MAMLIB__DECLR static
 #else
  #define MAMLIB__DECLR
 #endif
#endif
#define MAMLIB__DECLS static inline

#define MAMLIB_CHECK_COOKIE 0x44334433

#define MAMLIB_ALIGNMENT sizeof(void*)
#ifdef MAMLIB_NOSTD
 #define MAMLIB_MEMINT long long
 #define MAMLIB_BYTE char
#else
 #include "inttypes.h"
 #define MAMLIB_MEMINT int64_t
 #define MAMLIB_BYTE int8_t
 #include "stdlib.h"
 #define MAMLIB_MALLOC malloc
 #define MAMLIB_REALLOC realloc
 #define MAMLIB_FREE free
 #include "string.h"
 #define MAMLIB_STRLEN strlen
 #define MAMLIB_MEMCPY memcpy
 #define MAMLIB_MEMCMP memcmp
 #include "assert.h"
 #define MAMLIB_ASSERT(b, m) assert(b && m)
 #include "stdio.h"
 #define MAMLIB_PUTCHAR putchar
#endif



typedef MAMLIB_MEMINT mam_int;
typedef MAMLIB_BYTE mam_byte;

typedef struct MamString {
	char* ptr;
	mam_int count;
} MamString;

typedef enum MamAllocMode {
	MAM_MODE_ALLOC,
	MAM_MODE_REALLOC,
	MAM_MODE_FREE,
	MAM_MODE_FREEALL,
} MamAllocMode;

typedef enum MamLoggerMode {
	MAM_MODE_DEFAULT = 0,
	MAM_MODE_VERBOSE,
	MAM_MODE_DEBUG,
	MAM_MODE_WARNING,
	MAM_MODE_ERROR,
} MamLoggerMode;

typedef void* MamAllocator(MamAllocMode mode, mam_int alloc_size, void* old_ptr, mam_int old_size, void* allocator_data);

typedef void MamLogger(MamLoggerMode mode, MamString str, void* logger_data);

typedef struct MamStack {
	mam_int size;
	mam_int max_size;
	mam_int capacity;
} MamStack;


#define MAM_BUILDER_BUFFER_CAPACITY (4096 - sizeof(struct MamBuilderBuffer*) - sizeof(mam_int))
typedef struct MamBuilderBuffer {
	struct MamBuilderBuffer* next;
	mam_int size;
	mam_byte mem[MAM_BUILDER_BUFFER_CAPACITY];
} MamBuilderBuffer;

typedef struct MamBuilder {
	MamBuilderBuffer head;
	MamBuilderBuffer* tail;
	MamAllocator* allocator;
	void* allocator_data;
	mam_int size;
	mam_int failed;
} MamBuilder;




MAMLIB__DECLS MamString mam_tostr(char* str) {
	MamString ret = {str};
	ret.count = MAMLIB_STRLEN(str);
	return ret;
}
MAMLIB__DECLS MamString mam_consttostr(const char* str) {
	MamString ret;
	ret.ptr = (char*)str;//lol
	ret.count = MAMLIB_STRLEN(str);
	return ret;
}
MAMLIB__DECLS int mam_streq(MamString str0, MamString str1) {
	return str0.count == str1.count && MAMLIB_MEMCMP(str0.ptr, str1.ptr, str0.count) == 0;
}
MAMLIB__DECLS int mam_strcmp(MamString str0, MamString str1) {
	return str0.count > str1.count ? 1 : (str0.count < str1.count ? -1 : MAMLIB_MEMCMP(str0.ptr, str1.ptr, str0.count));
}
MAMLIB__DECLS int mam_cstreq(MamString str0, const char* str1) {
	return mam_streq(str0, mam_consttostr(str1));
}
MAMLIB__DECLS int mam_cstrcmp(MamString str0, const char* str1) {
	return mam_strcmp(str0, mam_consttostr(str1));
}

MAMLIB__DECLR int mam_toint(MamString str, mam_int* ret_n);

MAMLIB__DECLS mam_int mamlib_align(mam_int a) {
	//raises a to the nearest multiple of MAMLIB_ALIGNMENT
	return MAMLIB_ALIGNMENT*((a + (MAMLIB_ALIGNMENT - 1))/MAMLIB_ALIGNMENT);
}

#define mam_ptr_add(type, ptr, n) ((type*)((mam_byte*)(ptr) + (n)))


#ifndef MAMLIB_RELEASE
	#define mam__underflow_msg "mamlib: invalid buffer detected; either an invalid pointer was used, a buffer underflow occured, or an overflow of an adjacent buffer occured"
	#define mam__overflow_msg "mamlib: buffer overflow detected"

	#define MAM_CHECK_BASE_SIZE (mamlib_align(sizeof(int)))
	#define MAM_CHECK_MARK_SIZE (MAM_CHECK_BASE_SIZE + sizeof(int))

	MAMLIB__DECLS void mam_check_mark(void* buffer, mam_int buffer_size) {
		*mam_ptr_add(int, buffer, -(mam_int)sizeof(int)) = MAMLIB_CHECK_COOKIE;
		*mam_ptr_add(int, buffer, buffer_size) = MAMLIB_CHECK_COOKIE;
	}
	MAMLIB__DECLS void* mam_check_allocn(void* mem, mam_int buffer_size) {
		void* buffer = mam_ptr_add(void*, mem, MAM_CHECK_BASE_SIZE);
		mam_check_mark(buffer, buffer_size);
		return buffer;
	}

	/*******************************************************************************
	If your debugger takes you here because of an assertion, look at the assertion
	message for more information, and walk the stack back up to your own code.
	This library provides the following memory check functions to do quick and
	dirty memory intergrity tests while the library is in debug mode.
	They catch the most common bugs such as buffer overflows or use-after-free,
	and they never trigger an assert unless an actual memory bug occurred.
	This works by placing memory cookies above and below every allocated buffer,
	and checking their existance regularly.
	The default cookie is 0x44334433 == 1144210483 == "3D3D"
	********************************************************************************/
	MAMLIB__DECLS void mam_check(void* buffer, mam_int buffer_size) {
		MAMLIB_ASSERT(*(int*)((mam_byte*)buffer - sizeof(int)) == MAMLIB_CHECK_COOKIE, mam__underflow_msg);
		MAMLIB_ASSERT(*(int*)((mam_byte*)buffer + buffer_size) == MAMLIB_CHECK_COOKIE, mam__overflow_msg);
	}
	MAMLIB__DECLS void mam_check_base(void* buffer) {
		MAMLIB_ASSERT(*(int*)((mam_byte*)buffer - sizeof(int)) == MAMLIB_CHECK_COOKIE, mam__underflow_msg);
	}



	MAMLIB__DECLS void mam_check_unmark(void* buffer, mam_int buffer_size) {
		mam_check(buffer, buffer_size);
		*mam_ptr_add(int, buffer, -(mam_int)sizeof(int)) = 0;
		*mam_ptr_add(int, buffer, buffer_size) = 0;
	}
	MAMLIB__DECLS void mam_check_unmark_base(void* buffer) {
		mam_check_base(buffer);
		*mam_ptr_add(int, buffer, -(mam_int)sizeof(int)) = 0;
	}
#else
	#define MAM_CHECK_BASE_SIZE 0
	#define MAM_CHECK_MARK_SIZE 0

	#define mam_check_mark(buffer, buffer_size) 0
	#define mam_check_allocn(mem, buffer_size) ((void*)(mem))

	#define mam_check(buffer, buffer_size) 0
	#define mam_check_below(buffer) 0

	#define mam_check_unmark(buffer, buffer_size) 0
	#define mam_check_unmark_base(buffer) 0
#endif
#define mam_check_alloc(type, mem, buffer_size) ((type*)mam_check_allocn(mem, buffer_size))



MAMLIB__DECLR void* mam_system_allocator(MamAllocMode mode, mam_int alloc_size, void* old_ptr, mam_int old_size, void* allocator_data);

MAMLIB__DECLR void mam_system_logger(MamLoggerMode mode, MamString str, void* logger_data);



MAMLIB__DECLR MamStack* mam_stack_init(void* alloc_ptr, mam_int alloc_size);

MAMLIB__DECLR void* mam_stack_pushn(MamStack* stack, mam_int buffer_size);
#define mam_stack_push(type, stack, size) ((type*)mam_stack_pushn(stack, sizeof(type)*(size)))

MAMLIB__DECLR void mam_stack_pop(MamStack* stack, void* buffer);

MAMLIB__DECLR void mam_stack_set_size(MamStack* stack, mam_int new_size);

MAMLIB__DECLR void* mam_stack_allocator(MamAllocMode mode, mam_int alloc_size, void* old_ptr, mam_int old_size, void* allocator_data);



MAMLIB__DECLR void mam_builder_init(MamBuilder* builder);
MAMLIB__DECLR void mam_builder_push_char(MamBuilder* builder, char ch);
MAMLIB__DECLR void mam_builder_push_mem(MamBuilder* builder, const void* mem, mam_int mem_size);
MAMLIB__DECLS void mam_builder_push_str(MamBuilder* builder, MamString str) {
	mam_builder_push_mem(builder, str.ptr, str.count);
}
MAMLIB__DECLS void mam_builder_push_cstr(MamBuilder* builder, const char* str) {
	mam_builder_push_str(builder, mam_consttostr(str));
}
MAMLIB__DECLR void mam_builder_push_uint(MamBuilder* builder, mam_int i);
MAMLIB__DECLR void mam_builder_push_int(MamBuilder* builder, mam_int i);
MAMLIB__DECLR void mam_builder_push_hex_(MamBuilder* builder, mam_int i);
MAMLIB__DECLR void mam_builder_push_hex(MamBuilder* builder, mam_int i);

MAMLIB__DECLR void mam_builder_push_charf(MamBuilder* builder, const char* origin, char source);
MAMLIB__DECLR void mam_builder_push_strf(MamBuilder* builder, const char* origin, MamString source);
MAMLIB__DECLS void mam_builder_push_cstrf(MamBuilder* builder, const char* origin, const char* source) {
	mam_builder_push_strf(builder, origin, mam_consttostr(source));
}
MAMLIB__DECLR void mam_builder_push_intf(MamBuilder* builder, const char* origin, mam_int source);
MAMLIB__DECLR void mam_builder_push_hexf(MamBuilder* builder, const char* origin, mam_int source);


MAMLIB__DECLR void mam_builder_print(MamBuilder* builder, MamLogger* logger, void* logger_data);
MAMLIB__DECLS void mam_builder_system_print(MamBuilder* builder) {
	mam_builder_print(builder, &mam_system_logger, 0);
}


MAMLIB__DECLR MamString mam_builder_to_string(MamBuilder* builder, MamAllocator* allocator, void* allocator_data);


#ifdef __cplusplus
 MAMLIB__DECLS MamString mam_tostr(const char* str) {return mam_consttostr(str);}

 MAMLIB__DECLS void mam_builder_print(MamBuilder* builder) {mam_builder_system_print(builder);}
 MAMLIB__DECLS MamString mam_builder_to_string(MamBuilder* builder) {return mam_builder_to_string(builder, &mam_system_allocator, 0);}

 MAMLIB__DECLS void mam_builder_push(MamBuilder* builder, char ch) {mam_builder_push_char(builder, ch);}
 MAMLIB__DECLS void mam_builder_push(MamBuilder* builder, const void* mem, mam_int mem_size) {mam_builder_push_mem(builder, mem, mem_size);}
 MAMLIB__DECLS void mam_builder_push(MamBuilder* builder, MamString str) {mam_builder_push_str(builder, str);}
 MAMLIB__DECLS void mam_builder_push(MamBuilder* builder, const char* str) {mam_builder_push_cstr(builder, str);}
 MAMLIB__DECLS void mam_builder_push(MamBuilder* builder, mam_int i) {mam_builder_push_int(builder, i);}

 MAMLIB__DECLS void mam_builder_pushf(MamBuilder* builder, const char* origin, char ch) {mam_builder_push_charf(builder, origin, ch);}
 MAMLIB__DECLS void mam_builder_pushf(MamBuilder* builder, const char* origin, MamString str) {mam_builder_push_strf(builder, origin, str);}
 MAMLIB__DECLS void mam_builder_pushf(MamBuilder* builder, const char* origin, const char* str) {mam_builder_push_cstrf(builder, origin, str);}
 MAMLIB__DECLS void mam_builder_pushf(MamBuilder* builder, const char* origin, mam_int i) {mam_builder_push_intf(builder, origin, i);}
#endif



#endif

#ifdef MAMLIB_IMPLEMENTATION
#undef MAMLIB_IMPLEMENTATION


MAMLIB__DECLR int mam_toint(MamString str, mam_int* ret_n) {
	int is_neg = str.ptr[0] == '-';
	mam_int n = 0;
	mam_int i = 0;
	i += is_neg;
	for(;i < str.count; i += 1) {
		char ch = str.ptr[i];
		if('0' <= ch && ch <= '9') {
			n *= 10;
			n += ch - '0';
		} else {
			return 0;
		}
	}
	*ret_n = (2*is_neg)*n;
	return 1;
}



MAMLIB__DECLR void* mam_system_allocator(MamAllocMode mode, mam_int alloc_size, void* old_ptr, mam_int old_size, void* allocator_data) {
	if(mode == MAM_MODE_ALLOC) {
		return MAMLIB_MALLOC(alloc_size);
	} else if(mode == MAM_MODE_REALLOC) {
		return MAMLIB_REALLOC(old_ptr, alloc_size);
	} else if(mode == MAM_MODE_FREE) {
		MAMLIB_FREE(old_ptr);
	} else if(mode == MAM_MODE_FREEALL) {
	}
	return 0;
}

MAMLIB__DECLR void mam_system_logger(MamLoggerMode mode, MamString str, void* logger_data) {
	for(mam_int i = 0; i < str.count; i += 1) MAMLIB_PUTCHAR(str.ptr[i]);
}



MAMLIB__DECLR MamStack* mam_stack_init(void* alloc_ptr, mam_int alloc_size) {
	MamStack* stack = (MamStack*)alloc_ptr;
	stack->size = 0;
	stack->max_size = 0;
	stack->capacity = alloc_size - sizeof(MamStack);
	return stack;
}

MAMLIB__DECLR void* mam_stack_pushn(MamStack* stack, mam_int buffer_size) {
	mam_int alloc_size = mamlib_align(buffer_size + MAM_CHECK_MARK_SIZE);
	mam_int new_size = stack->size + alloc_size;

	void* mem = mam_ptr_add(void, stack, sizeof(MamStack) + stack->size);
	stack->size = new_size;
	if(stack->max_size < new_size) stack->max_size = new_size;
	if(new_size > stack->capacity) {
		MAMLIB_ASSERT(new_size > stack->capacity, "mamlib: stack is out of memory");
		mem = MAMLIB_MALLOC(alloc_size);
	}

	return mam_check_allocn(mem, buffer_size);
}
#define mam_stack_push(type, stack, size) ((type*)mam_stack_pushn(stack, sizeof(type)*(size)))

MAMLIB__DECLR void mam_stack_pop(MamStack* stack, void* buffer) {
	mam_check_unmark_base(buffer);
	mam_int new_size = mam_ptr_add(mam_byte, buffer, -MAM_CHECK_BASE_SIZE) - mam_ptr_add(mam_byte, stack, sizeof(MamStack));

	stack->size += new_size;
	MAMLIB_ASSERT(new_size > 0, "mamlib: attempt to pop stack when stack is empty");
}

MAMLIB__DECLR void mam_stack_set_size(MamStack* stack, mam_int new_size) {
	stack->size = new_size;
	if(stack->max_size < new_size) stack->max_size = new_size;
	MAMLIB_ASSERT(new_size <= stack->capacity, "mamlib: stack is out of memory");
}

MAMLIB__DECLR void* mam_stack_allocator(MamAllocMode mode, mam_int alloc_size, void* old_ptr, mam_int old_size, void* allocator_data) {
	MamStack* stack = (MamStack*)allocator_data;
	if(mode == MAM_MODE_ALLOC) {
		return mam_stack_pushn(stack, alloc_size);
	} else if(mode == MAM_MODE_REALLOC) {
		mam_stack_pop(stack, old_ptr);
		return mam_stack_pushn(stack, alloc_size);
	} else if(mode == MAM_MODE_FREE) {
		mam_stack_pop(stack, old_ptr);
	} else if(mode == MAM_MODE_FREEALL) {
		stack->size = 0;
	}
	return 0;
}



MAMLIB__DECLR void mam_builder_init(MamBuilder* builder) {
	builder->head.next = 0;
	builder->head.size = 0;
	builder->tail = &builder->head;
	builder->allocator = &mam_system_allocator;
	builder->allocator_data = 0;
	builder->failed = 0;
}

MAMLIB__DECLR void mam_builder_push_char(MamBuilder* builder, char ch) {
	MamBuilderBuffer* tail = builder->tail;
	if(tail->size >= MAM_BUILDER_BUFFER_CAPACITY) {
		MamBuilderBuffer* new_tail = (MamBuilderBuffer*)builder->allocator(MAM_MODE_ALLOC, sizeof(MamBuilderBuffer), 0, 0, builder->allocator_data);
		if(!new_tail) {builder->failed = 1; return;}
		new_tail->size = 0;
		new_tail->next = 0;
		tail->next = new_tail;
		tail = new_tail;
	}
	tail->mem[tail->size] = ch;
	tail->size += 1;
	builder->size += 1;
}
MAMLIB__DECLR void mam_builder_push_mem(MamBuilder* builder, const void* mem, mam_int mem_size) {
	MamBuilderBuffer* tail = builder->tail;
	while(1) {
		mam_int size_left = MAM_BUILDER_BUFFER_CAPACITY - tail->size;
		if(mem_size > size_left) {
			MAMLIB_MEMCPY(&tail->mem[tail->size], mem, size_left);
			tail->size += size_left;
			builder->size += size_left;
			mem_size -= size_left;
			mem = mam_ptr_add(void, mem, size_left);

			MamBuilderBuffer* new_tail = (MamBuilderBuffer*)builder->allocator(MAM_MODE_ALLOC, sizeof(MamBuilderBuffer), 0, 0, builder->allocator_data);
			if(!new_tail) {builder->failed = 1; return;}
			new_tail->size = 0;
			new_tail->next = 0;
			tail->next = new_tail;
			tail = new_tail;
		} else {
			MAMLIB_MEMCPY(&tail->mem[tail->size], mem, mem_size);
			tail->size += mem_size;
			return;
		}
	}
}

MAMLIB__DECLR void mam_builder_push_uint(MamBuilder* builder, mam_int i) {
	if(i > 10) mam_builder_push_uint(builder, i/10);
	mam_builder_push_char(builder, i%10 + '0');
}
MAMLIB__DECLR void mam_builder_push_int(MamBuilder* builder, mam_int i) {
	if(i < 0) {
		mam_builder_push_char(builder, '-');
		mam_builder_push_uint(builder, -i);
	} else {
		mam_builder_push_uint(builder, i);
	}
}
MAMLIB__DECLR void mam_builder_push_hex_(MamBuilder* builder, mam_int i) {
	if(i > 16) mam_builder_push_uint(builder, i/16);
	mam_builder_push_char(builder, i%16 + '0');
}
MAMLIB__DECLR void mam_builder_push_hex(MamBuilder* builder, mam_int i) {
	mam_builder_push_char(builder, '0');
	mam_builder_push_char(builder, 'x');
	mam_builder_push_hex_(builder, i);
}

MAMLIB__DECLR void mam_builder_push_charf(MamBuilder* builder, const char* origin, char source) {
	for(; *origin; origin += 1) {
		if(*origin == '%') {
			mam_builder_push_char(builder, source);
		} else {
			mam_builder_push_char(builder, *origin);
		}
	}
}
MAMLIB__DECLR void mam_builder_push_strf(MamBuilder* builder, const char* origin, MamString source) {
	for(; *origin; origin += 1) {
		if(*origin == '%') {
			mam_builder_push_str(builder, source);
		} else {
			mam_builder_push_char(builder, *origin);
		}
	}
}
MAMLIB__DECLR void mam_builder_push_intf(MamBuilder* builder, const char* origin, mam_int source) {
	for(; *origin; origin += 1) {
		if(*origin == '%') {
			mam_builder_push_int(builder, source);
		} else {
			mam_builder_push_char(builder, *origin);
		}
	}
}
MAMLIB__DECLR void mam_builder_push_hexf(MamBuilder* builder, const char* origin, mam_int source) {
	for(; *origin; origin += 1) {
		if(*origin == '%') {
			mam_builder_push_hex(builder, source);
		} else {
			mam_builder_push_char(builder, *origin);
		}
	}
}


MAMLIB__DECLR void mam_builder_print(MamBuilder* builder, MamLogger* logger, void* logger_data) {
	MamBuilderBuffer* head = &builder->head;
	while(head) {
		MamString str = {(char*)head->mem, head->size};
		logger(MAM_MODE_DEFAULT, str, logger_data);
		head = head->next;
	}
}


MAMLIB__DECLR MamString mam_builder_to_string(MamBuilder* builder, MamAllocator* allocator, void* allocator_data) {
	MamString str = {(char*)allocator(MAM_MODE_ALLOC, builder->size, 0, 0, allocator_data), builder->size};
	if(!str.ptr) return str;
	MamBuilderBuffer* head = &builder->head;
	while(head) {
		MAMLIB_MEMCPY(str.ptr, head->mem, head->size);
		head = head->next;
	}
	return str;
}

#endif
