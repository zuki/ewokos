#include <mm/trunkmem.h>
#include <mm/mmu.h>
#include <kstring.h>
#include <stddef.h>

/*
malloc for memory trunk management
*/

static mem_block_t* gen_block(char* p, uint32_t size) {
	uint32_t block_size = sizeof(mem_block_t);
	mem_block_t* block = (mem_block_t*)p;
	block->next = block->prev = NULL;
	block->mem = p + block_size;
	block->size = size - block_size;
	return block;
}

mem_block_t* get_block(char* p) {
	if(p == NULL)
		return NULL;

	uint32_t block_size = sizeof(mem_block_t);
	if(((uint32_t)p) < block_size)
		return NULL;

	mem_block_t* block = (mem_block_t*)(p - block_size);
	return block;
}

/*if block size much bigger than the size required, break to two blocks*/
static void try_break(malloc_t* m, mem_block_t* block, uint32_t size) {
	uint32_t block_size = sizeof(mem_block_t);
	//required more than half size of block. no break.
	if((block_size+size) > (uint32_t)(block->size/2)) 
		return;
	
	//do break;
	char* p = block->mem + size;
	mem_block_t* newBlock = gen_block(p, block->size - size);
	newBlock->used = 0; //break a new free block.

	block->size = size;
	newBlock->next = block->next;
	if(newBlock->next != NULL)
		newBlock->next->prev = newBlock;

	newBlock->prev = block;
	block->next = newBlock;

	if(m->tail == block) 
		m->tail = newBlock;
}

char* trunk_malloc(malloc_t* m, uint32_t size) {
	size = ALIGN_UP(size, 8);
	mem_block_t* block = m->start == NULL ? m->head : m->start;
	while(block != NULL) {
		if(block->used || block->size < size) {
			block = block->next;
		}
		else {
			block->used = 1;
			try_break(m, block, size);
			break;
		}
	}
	if(block != NULL) {
		m->start = block->next;
		return block->mem;
	}

	/*Can't find any available block, expand pages*/
	uint32_t block_size = sizeof(mem_block_t);
	uint32_t expand_size = size + block_size;

	uint32_t pages = expand_size / PAGE_SIZE;	
	if((expand_size % PAGE_SIZE) > 0)
		pages++;

	char* p = (char*)m->get_mem_tail(m->arg);
	if(m->expand(m->arg, pages) != 0)
		return NULL;

	block = gen_block(p, pages*PAGE_SIZE);
	block->used = 1;

	if(m->head == NULL) {
		m->head = block;
	}

	if(m->tail == NULL) {
		m->tail = block;
	}
	else {
		m->tail->next = block;
		block->prev = m->tail;
		m->tail = block;
	}

	try_break(m, block, size);
	m->start = block->next;
	return block->mem;
}

uint32_t trunk_free_size(malloc_t* m) {
	uint32_t ret = m->get_mem_top(m->arg) - m->get_mem_tail(m->arg);
	//mem_block_t* block = m->start == NULL ? m->head : m->start;
	mem_block_t* block = m->head;
	while(block != NULL) {
		if(!block->used) {
			ret += block->size;
		}
		block = block->next;
	}
	return ret;
}

/*
try to merge around free blocks.
*/
static mem_block_t* try_merge(malloc_t* m, mem_block_t* block) {
	mem_block_t* ret = block;
	uint32_t block_size = sizeof(mem_block_t);
	//try next block	
	mem_block_t* b = block->next;
	if(b != NULL && b->used == 0) {
		block->size += (b->size + block_size);
		block->next = b->next;
		if(block->next != NULL) 
			block->next->prev = block;
		else
			m->tail = block;
	}

	//try left block	
	b = block->prev;
	if(b != NULL && b->used == 0) {
		b->size += (block->size + block_size);
		b->next = block->next;
		if(b->next != NULL) 
			b->next->prev = b;
		else
			m->tail = b;
		ret = b;
	}
	return ret;
}

/*
try to shrink the pages.
*/
static void try_shrink(malloc_t* m) {
	uint32_t block_size = sizeof(mem_block_t);
	uint32_t addr = (uint32_t)m->tail;
	//check if page aligned.	
	if(m->tail == NULL ||
			m->tail->used == 1 ||
			(addr % PAGE_SIZE) != 0)
		return;

	uint32_t pages = (m->tail->size+block_size) / PAGE_SIZE;
	m->tail = m->tail->prev;
	if(m->tail != NULL)
		m->tail->next = NULL;
	else
		m->head = NULL;
	m->shrink(m->arg, pages);
}

void trunk_free(malloc_t* m, char* p) {
	mem_block_t* block = get_block(p);
	if(block == NULL)
		return;

	block->used = 0; //mark as free.
	block = try_merge(m, block);
	if(m->start == 0 || m->start >= block)
		m->start = block->prev;
	if(m->shrink != NULL)
		try_shrink(m);
}

uint32_t trunk_msize(malloc_t* m, char* p) {
    (void)m;
	mem_block_t* block = get_block(p);
	if(block == NULL)
		return 0;
	return block->size;
}
