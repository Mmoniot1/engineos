//By Monica Moniot
#include "basic.h"


typedef struct {
	uint32 size;
	uint32 capacity;
	relptr root_block;
	relptr end_block;
} Slab;

void slab_init(Slab* slab, uint32 capacity);

relptr slab_alloc_rel(Slab* slab, uint32 size);
#define slab_alloc(type, slab, size) ptr_add(type, slab, slab_alloc_rel(slab, size))

void slab_free_rel(Slab* slab, relptr item);
void slab_free(Slab* slab, void* ptr);

#ifdef BASIC_IMPLEMENTATION
typedef struct {
	relptr pre;//points to the previous block in memory, is 0 for the first block
	uint32 size;//the current size of the memory here
	relptr free_parent;//When this equals 0, the block is in use, when equals 1, this block is the root
	relptr free_left;//points to next item in free list
	relptr free_right;//points to next item in free list
} __Block;

static inline void __block_set_next(__Block* block, relptr cur_i, relptr next_i) {
	block->size = next_i - cur_i;
}
static inline relptr __block_get_next(__Block* block, relptr cur_i) {
	return block->size + cur_i;
}
static inline uint32 __slab_correct_size(uint32 size) {
	return sizeof(__Block)*((size + sizeof(__Block) - 1)/sizeof(__Block) + 1);
}

static void __free_remove(Slab* slab, __Block* block, relptr i) {
	relptr parent_i = block->free_parent;
	relptr right_i = block->free_right;
	relptr left_i = block->free_left;
	relptr* parent_child;
	if(parent_i == 1) {
		parent_child = &slab->root_block;
	} else if(parent_block->free_right == i) {
		parent_child = &ptr_add(__Block, slab, parent_i)->free_right;
	} else {
		parent_child = &ptr_add(__Block, slab, parent_i)->free_left;
	}
	if(right_i) {
		*parent_child = right_i;
		ptr_add(__Block, slab, right_i)->free_parent = parent_i;
		if(left_i) {
			relptr left_parent_i;
			do {
				left_parent_i = right_i;
				right_i = ptr_add(__Block, slab, left_parent_i)->free_left;
			} while(right_i);
			ptr_add(__Block, slab, left_parent_i)->free_left = left_i;
			ptr_add(__Block, slab, left_i)->free_parent = left_parent_i;
		}
	} else if(left_i) {
		*parent_child = left_i;
		ptr_add(__Block, slab, left_i)->free_parent = parent_i;
	} else {
		*parent_child = 0;
	}
}
static void __free_insert(Slab* slab, __Block* block, relptr i) {
	uint32 size = block->size;
	relptr root_i = slab->root_block;
	if(root_i) {//add block to the free tree
		while(true) {
			__Block* root_block = ptr_add(__Block, slab, root_i);
			if(size >= root_block->size) {
				if(root_block->free_right) {
					root_i = root_block->free_right;
				} else {
					root_block->free_right = i;
					block->free_parent = root_i;
					block->free_right = 0;
					block->free_left = 0;
					return;
				}
			} else {
				if(root_block->free_left) {
					root_i = root_block->free_left;
				} else {
					root_block->free_left = i;
					block->free_parent = root_i;
					block->free_right = 0;
					block->free_left = 0;
					return;
				}
			}
		}
	} else {//tree is empty
		slab->root_block = cur_i;
		block->free_parent = 1;
		block->free_right = 0;
		block->free_left = 0;
	}
}
static relptr __free_find(Slab* slab, uint32 size) {
	relptr root_i = slab->root_block;
	if(root_i) {
		while(true) {
			__Block* root_block = ptr_add(__Block, slab, root_i);
			if(size < root_block->size) {
				if(root_block->free_right) {
					root_i = root_block->free_right;
				} else {
					return 0;
				}
			} else {
				__free_remove(slab, root_block, root_i);
				root_block->free_parent = 0;
				return root_i;
			}
		}
	} else {//tree is empty
		return 0;
	}
}

void slab_init(Slab* slab, uint32 capacity) {
	slab->size = sizeof(Slab);
	slab->capacity = capacity;
	slab->head_block = 0;
	slab->end_block = 0;
}

relptr slab_alloc_rel(Slab* slab, uint32 size) {
	size = __slab_correct_size(size);
	relptr original_i = slab->head_block;
	if(original_i) {
		relptr cur_i = original_i;
		do {
			__Block* cur_block = ptr_add(__Block, slab, cur_i);
			relptr next_i = cur_block->free_next;
			slab->head_block = next_i;
			if(cur_block->size == size) {//remove cur_block from free list
				relptr pre_i = cur_block->free_pre;
				if(next_i == cur_i) {//all items would be removed from free list
					slab->head_block = 0;
				} else {
					slab->head_block = next_i;
					ptr_add(__Block, slab, pre_i)->free_next = next_i;
					ptr_add(__Block, slab, next_i)->free_pre = pre_i;
				}
				cur_block->free_pre = 0;
				return cur_i + sizeof(__Block);
			} else if(cur_block->size > size) {
				relptr pre_i = cur_block->free_pre;

				relptr fragment_i = cur_i + size;
				__Block* fragment_block = ptr_add(__Block, slab, fragment_i);
				fragment_block->pre = cur_i;
				fragment_block->size = cur_block->size - size;
				cur_block->size = size;
				if(slab->end_block == cur_i) {
					slab->end_block = fragment_i;
				}

				if(next_i == cur_i) {//only one item in free list
					slab->head_block = fragment_i;
					fragment_block->free_pre = fragment_i;
					fragment_block->free_next = fragment_i;
				} else {
					fragment_block->free_pre = pre_i;
					fragment_block->free_next = next_i;
					ptr_add(__Block, slab, pre_i)->free_next = fragment_i;
					ptr_add(__Block, slab, next_i)->free_pre = fragment_i;
				}
				cur_block->free_pre = 0;
				return cur_i + sizeof(__Block);
			}
			cur_i = next_i;
		} while(original_i != cur_i);
	}
	//no free space, push the size
	relptr i = slab->size;
	__Block* block = ptr_add(__Block, slab, i);
	if(slab->end_block) {
		__block_set_next(ptr_add(__Block, slab, slab->end_block), slab->end_block, i);
		block->pre = slab->end_block;
	}
	block->size = size;
	block->free_pre = 0;
	slab->size += size;
	slab->end_block = i;
	return i + sizeof(__Block);
}

void slab_free_rel(Slab* slab, relptr item) {
	relptr cur_i = item - sizeof(__Block);
	__Block* cur_block = ptr_add(__Block, slab, cur_i);
	relptr pre_i = cur_block->pre;
	__Block* pre_block = ptr_add(__Block, slab, pre_i);
	relptr next_i = __block_get_next(cur_block, cur_i);
	__Block* next_block = ptr_add(__Block, slab, next_i);

	assert(!cur_block->free_parent);
	if(pre_i && pre_block->free_parent) {//is free, combine
		if(slab->end_block == cur_i) {//is last block in the whole slab, remove from slab
			slab->end_block = pre_block->pre;
			slab->size -= cur_block->size + pre_block->size;
			//remove pre_block
			__free_remove(slab, pre_block, pre_i);
		} else if(next_block->free_parent) {//remove next_block and cur_block
			__block_set_next(pre_block, pre_i, __block_get_next(next_block, next_i));
			ptr_add(__Block, slab, __block_get_next(next_block, next_i))->pre = pre_i;
			//update pre_block
			__free_remove(slab, pre_block, pre_i);
			__free_remove(slab, next_block, next_i);
			__free_insert(slab, pre_block, pre_i);
		} else {//remove cur_block
			__block_set_next(pre_block, pre_i, next_i);
			next_block->pre = pre_i;
			//update pre_block
			__free_remove(slab, pre_block, pre_i);
			__free_insert(slab, pre_block, pre_i);
		}
	} else if(slab->end_block == cur_i) {//cur_block is last block in slab, remove it
		slab->end_block = pre_i;
		slab->size -= cur_block->size;
	} else if(next_block->free_parent) {//remove next_block
		__block_set_next(cur_block, cur_i, __block_get_next(next_block, next_i));
		ptr_add(__Block, slab, __block_get_next(next_block, next_i))->pre = cur_i;
		//inherit next_block's free list info
		__free_remove(slab, next_block, next_i);
		__free_insert(slab, cur_block, cur_i);
	} else {//add to free tree
		__free_insert(slab, cur_block, cur_i);
	}
}
void slab_free(Slab* slab, void* ptr) {
	slab_free_rel(slab, ptr_dist(slab, ptr));
}

void slab_check_all(Slab* slab) {
	if(slab->end_block) {
		relptr cur_i = sizeof(Slab);
		while(cur_i < slab->end_block) {
			cur_i = __block_get_next(ptr_add(__Block, slab, cur_i), cur_i);
			assert(sizeof(Slab) < cur_i);
			assert(cur_i < slab->size);
		}
		assert(cur_i == slab->end_block);
		assert(__block_get_next(ptr_add(__Block, slab, cur_i), cur_i) == slab->size);
	}
}
#endif
