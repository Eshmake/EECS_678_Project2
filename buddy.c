/**
 * Buddy Allocator
 *
 * For the list library usage, see http://www.mcs.anl.gov/~kazutomo/list/
 */

/**************************************************************************
 * Conditional Compilation Options
 **************************************************************************/
#define USE_DEBUG 0

/**************************************************************************
 * Included Files
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "buddy.h"
#include "list.h"

/**************************************************************************
 * Public Definitions
 **************************************************************************/
#define MIN_ORDER 12
#define MAX_ORDER 20

#define PAGE_SIZE (1<<MIN_ORDER)
/* page index to address */
#define PAGE_TO_ADDR(page_idx) (void *)((page_idx*PAGE_SIZE) + g_memory)

/* address to page index */
#define ADDR_TO_PAGE(addr) ((unsigned long)((void *)addr - (void *)g_memory) / PAGE_SIZE)

/* find buddy address */
#define BUDDY_ADDR(addr, o) (void *)((((unsigned long)addr - (unsigned long)g_memory) ^ (1<<o)) \
									 + (unsigned long)g_memory)

#if USE_DEBUG == 1
#  define PDEBUG(fmt, ...) \
	fprintf(stderr, "%s(), %s:%d: " fmt,			\
		__func__, __FILE__, __LINE__, ##__VA_ARGS__)
#  define IFDEBUG(x) x
#else
#  define PDEBUG(fmt, ...)
#  define IFDEBUG(x)
#endif

/**************************************************************************
 * Public Types
 **************************************************************************/
typedef struct {
	struct list_head list;
	/* TODO: DECLARE NECESSARY MEMBER VARIABLES */
	int order;
	int is_free;
} page_t;

/**************************************************************************
 * Global Variables
 **************************************************************************/
/* free lists*/
struct list_head free_area[MAX_ORDER+1];

/* memory area */
char g_memory[1<<MAX_ORDER];

/* page structures */
page_t g_pages[(1<<MAX_ORDER)/PAGE_SIZE];

/**************************************************************************
 * Public Function Prototypes
 **************************************************************************/

/**************************************************************************
 * Local Functions
 **************************************************************************/

// Helper to get the order of the smallest block that can satisfy the request

static int get_order(int size){
	int o = MIN_ORDER;
	int block_size = 1 << o;

	while(o <= MAX_ORDER && block_size < size){
		o++;
		block_size <<= 1;
	}

	if(o > MAX_ORDER) return -1;

	return o;
}


/**
 * Initialize the buddy system
 */
void buddy_init()
{
	int i;
	int n_pages = (1<<MAX_ORDER) / PAGE_SIZE;
	for (i = 0; i < n_pages; i++) {
		/* TODO: INITIALIZE PAGE STRUCTURES */
		INIT_LIST_HEAD(&g_pages[i].list);
		g_pages[i].order = MIN_ORDER;
		g_pages[i].is_free = 0;
	}

	g_pages[0].order = MAX_ORDER;
	g_pages[0].is_free = 1;
	
	/* initialize freelist */
	for (i = MIN_ORDER; i <= MAX_ORDER; i++) {
		INIT_LIST_HEAD(&free_area[i]);
	}

	/* add the entire memory as a freeblock */
	list_add(&g_pages[0].list, &free_area[MAX_ORDER]);

}

/**
 * Allocate a memory block.
 *
 * On a memory request, the allocator returns the head of a free-list of the
 * matching size (i.e., smallest block that satisfies the request). If the
 * free-list of the matching block size is empty, then a larger block size will
 * be selected. The selected (large) block is then splitted into two smaller
 * blocks. Among the two blocks, left block will be used for allocation or be
 * further splitted while the right block will be added to the appropriate
 * free-list.
 *
 * @param size size in bytes
 * @return memory block address
 */
void *buddy_alloc(int size)
{
	/* TODO: IMPLEMENT THIS FUNCTION */

	int req_order = get_order(size);
	if(req_order == -1) return NULL;

	if(list_empty(&free_area[req_order])){
		for(int o = req_order+1; o <= MAX_ORDER; o++){
			if(!list_empty(&free_area[o])){
				page_t *block = list_entry(free_area[o].next, page_t, list);
				list_del(&block->list);

				unsigned long block_idx = block - g_pages;

				while(o > req_order){
					o--;

					unsigned long buddy_idx = block_idx + ((1 << o) / PAGE_SIZE);
					page_t *buddy = &g_pages[buddy_idx];
					
					buddy->order = o;
					buddy->is_free = 1;

					INIT_LIST_HEAD(&buddy->list);
					list_add(&buddy->list, &free_area[o]);

					block->order = o;
				}
				
				block->order = req_order;
				block->is_free = 0;
				return PAGE_TO_ADDR(block_idx); 
			}
		}

		return NULL;
		
	}
	else{
		page_t *block = list_entry(free_area[req_order].next, page_t, list);
		list_del(&block->list);

		block->order = req_order;
		block->is_free = 0;

		unsigned long block_idx = block - g_pages;
		return PAGE_TO_ADDR(block_idx); 
	}
}

/**
 * Free an allocated memory block.
 *
 * Whenever a block is freed, the allocator checks its buddy. If the buddy is
 * free as well, then the two buddies are combined to form a bigger block. This
 * process continues until one of the buddies is not free.
 *
 * @param addr memory block address to be freed
 */
void buddy_free(void *addr)
{
	/* TODO: IMPLEMENT THIS FUNCTION */
	unsigned long page_idx = ADDR_TO_PAGE(addr);
	page_t *page = &g_pages[page_idx];
	page->is_free = 1;

	int o = page->order;

	while(o < MAX_ORDER){
		unsigned long buddy_idx = page_idx ^ ((1 << o) / PAGE_SIZE);
		page_t *buddy = &g_pages[buddy_idx];

		if(!buddy->is_free || buddy->order != o) break;

		list_del(&buddy->list);
		buddy->is_free = 0;

		if(buddy_idx < page_idx){
			page_idx = buddy_idx;
		}

		page = &g_pages[page_idx];
		page->is_free = 1;

		o++;
		page->order = o;

	}

	INIT_LIST_HEAD(&page->list);
	list_add(&page->list, &free_area[o]);

}

/**
 * Print the buddy system status---order oriented
 *
 * print free pages in each order.
 */
void buddy_dump()
{
	int o;
	for (o = MIN_ORDER; o <= MAX_ORDER; o++) {
		struct list_head *pos;
		int cnt = 0;
		list_for_each(pos, &free_area[o]) {
			cnt++;
		}
		printf("%d:%dK ", cnt, (1<<o)/1024);
	}
	printf("\n");
}
