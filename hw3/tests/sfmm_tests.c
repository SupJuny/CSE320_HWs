#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>
#include "debug.h"
#include "sfmm.h"
#define TEST_TIMEOUT 15

/*
 * Assert the total number of free blocks of a specified size.
 * If size == 0, then assert the total number of all free blocks.
 */
void assert_free_block_count(size_t size, int count) {
    int cnt = 0;
    for(int i = 0; i < NUM_FREE_LISTS; i++) {
	sf_block *bp = sf_free_list_heads[i].body.links.next;
	while(bp != &sf_free_list_heads[i]) {
	    if(size == 0 || size == (bp->header & ~0xf))
		cnt++;
	    bp = bp->body.links.next;
	}
    }
    if(size == 0) {
	cr_assert_eq(cnt, count, "Wrong number of free blocks (exp=%d, found=%d)",
		     count, cnt);
    } else {
	cr_assert_eq(cnt, count, "Wrong number of free blocks of size %ld (exp=%d, found=%d)",
		     size, count, cnt);
    }
}

/*
 * Assert that the free list with a specified index has the specified number of
 * blocks in it.
 */
void assert_free_list_size(int index, int size) {
    int cnt = 0;
    sf_block *bp = sf_free_list_heads[index].body.links.next;
    while(bp != &sf_free_list_heads[index]) {
	cnt++;
	bp = bp->body.links.next;
    }
    cr_assert_eq(cnt, size, "Free list %d has wrong number of free blocks (exp=%d, found=%d)",
		 index, size, cnt);
}

Test(sfmm_basecode_suite, malloc_an_int, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	int *x = sf_malloc(sizeof(int));

	cr_assert_not_null(x, "x is NULL!");

	*x = 4;

	cr_assert(*x == 4, "sf_malloc failed to give proper space for an int!");

	assert_free_block_count(0, 1);
	assert_free_block_count(4016, 1);
	assert_free_list_size(9, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
	cr_assert(sf_mem_start() + PAGE_SZ == sf_mem_end(), "Allocated more than necessary!");
}

Test(sfmm_basecode_suite, malloc_four_pages, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	// We want to allocate up to exactly four pages.
	void *x = sf_malloc(16384 - 48 - (sizeof(sf_header) + sizeof(sf_footer)));

	cr_assert_not_null(x, "x is NULL!");
	assert_free_block_count(0, 0);
	cr_assert(sf_errno == 0, "sf_errno is not 0!");
}

Test(sfmm_basecode_suite, malloc_too_large, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	void *x = sf_malloc(PAGE_SZ << 16);
	cr_assert_null(x, "x is not NULL!");
	assert_free_block_count(0, 1);
	assert_free_block_count(110544, 1);
	cr_assert(sf_errno == ENOMEM, "sf_errno is not ENOMEM!");
}

Test(sfmm_basecode_suite, free_no_coalesce, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	/* void *x = */ sf_malloc(8);
	void *y = sf_malloc(200);
	/* void *z = */ sf_malloc(1);

	sf_free(y);

	assert_free_block_count(0, 2);
	assert_free_block_count(224, 1);
	assert_free_block_count(3760, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sfmm_basecode_suite, free_coalesce, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	/* void *w = */ sf_malloc(8);
	void *x = sf_malloc(200);
	void *y = sf_malloc(300);
	/* void *z = */ sf_malloc(4);

	sf_free(y);
	sf_free(x);

	assert_free_block_count(0, 2);
	assert_free_block_count(544, 1);
	assert_free_block_count(3440, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sfmm_basecode_suite, freelist, .timeout = TEST_TIMEOUT) {
	void *u = sf_malloc(200);
	/* void *v = */ sf_malloc(300);
	void *w = sf_malloc(200);
	/* void *x = */ sf_malloc(500);
	void *y = sf_malloc(200);
	/* void *z = */ sf_malloc(700);

	sf_free(u);
	sf_free(w);
	sf_free(y);

	assert_free_block_count(0, 4);
	assert_free_block_count(224, 3);
	assert_free_block_count(1808, 1);
	assert_free_list_size(4, 3);
	assert_free_list_size(9, 1);

	// First block in list should be the most recently freed block.
	int i = 4;
	sf_block *bp = sf_free_list_heads[i].body.links.next;
	cr_assert_eq(bp, (char *)y - 2*sizeof(sf_header),
		     "Wrong first block in free list %d: (found=%p, exp=%p)!",
                     i, bp, (char *)y - 2*sizeof(sf_header));
}

Test(sfmm_basecode_suite, realloc_larger_block, .timeout = TEST_TIMEOUT) {
	void *x = sf_malloc(sizeof(int));
	/* void *y = */ sf_malloc(10);
	x = sf_realloc(x, sizeof(int) * 20);

	cr_assert_not_null(x, "x is NULL!");
	sf_block *bp = (sf_block *)((char *)x - 2*sizeof(sf_header));
	cr_assert(bp->header & 0x8, "Allocated bit is not set!");
	cr_assert((bp->header & ~0xf & 0xffff) == 96,
		  "Realloc'ed block size not what was expected (found=%lu, exp=%lu)!",
		  bp->header & ~0xf, 96);

	assert_free_block_count(0, 2);
	assert_free_block_count(32, 1);
	assert_free_block_count(3888, 1);
}

Test(sfmm_basecode_suite, realloc_smaller_block_splinter, .timeout = TEST_TIMEOUT) {
	void *x = sf_malloc(sizeof(int) * 20);
	void *y = sf_realloc(x, sizeof(int) * 16);

	cr_assert_not_null(y, "y is NULL!");
	cr_assert(x == y, "Payload addresses are different!");

	sf_block *bp = (sf_block *)((char*)y - 2*sizeof(sf_header));
	cr_assert(bp->header & 0x8, "Allocated bit is not set!");
	cr_assert((bp->header & ~0xf & 0xffff) == 96,
		  "Block size not what was expected (found=%lu, exp=%lu)!",
		  bp->header & ~0xf & 0xffff, 96);

	// There should be only one free block of size 3952.
	assert_free_block_count(0, 1);
	assert_free_block_count(3952, 1);
}

Test(sfmm_basecode_suite, realloc_smaller_block_free_block, .timeout = TEST_TIMEOUT) {
	void *x = sf_malloc(sizeof(double) * 8);
	void *y = sf_realloc(x, sizeof(int));

	cr_assert_not_null(y, "y is NULL!");

	sf_block *bp = (sf_block *)((char*)y - 2*sizeof(sf_header));
	cr_assert(bp->header & 0x8, "Allocated bit is not set!");
	cr_assert((bp->header & ~0xf & 0xffff) == 32,
		  "Realloc'ed block size not what was expected (found=%lu, exp=%lu)!",
		  bp->header & ~0xf & 0xffff, 32);

	// After realloc'ing x, we can return a block of size 32 to the freelist.
	// This block will go into the freelist and be coalesced.
	assert_free_block_count(0, 1);
	assert_free_block_count(4016, 1);
}

//############################################
//STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
//DO NOT DELETE THESE COMMENTS
//############################################

void assert_fragmetation_size(double fragment) {
	double tests = sf_fragmentation();
    cr_assert_eq(tests, fragment, "Fragmentation has wrong number (exp=%f, found=%f)",
		 fragment, tests);
}

void assert_utilization_size(double utilize) {
	double tests = sf_utilization();
    cr_assert_eq(tests, utilize, "Utilization has wrong number (exp=%f, found=%f)",
		 utilize, tests);
}

Test(sfmm_student_suite, student_test_1, .timeout = TEST_TIMEOUT) {
	void *x = sf_malloc(sizeof(double) * 8);
	void *y = sf_malloc(4*PAGE_SZ);
	void *z = sf_realloc(x, 4 * sizeof(int));

	sf_block *bp = (sf_block *)((void*)z - 2*sizeof(sf_header));
	cr_assert(bp->header & 0x8, "Allocated bit is not set!");
	cr_assert(bp->header & 0x4, "Prev allocated bit is not set!");
	cr_assert((bp->header & ~0xf & 0xffff) == 32,
		  "Realloc'ed block size not what was expected (found=%lu, exp=%lu)!",
		  bp->header & ~0xf & 0xffff, 32);

	cr_assert_not_null(z, "z is NULL!");

	sf_free(z);

	assert_free_block_count(80, 1);
	assert_free_block_count(3952, 1);

	sf_free(y);

	assert_free_block_count(20432, 1);
}

Test(sfmm_student_suite, student_test_2, .timeout = TEST_TIMEOUT) {
	void *x = sf_malloc(4);
	void *y = sf_realloc(x, 10);

	sf_block *bp = (sf_block *)((void*)y - 2*sizeof(sf_header));
	cr_assert(bp->header & 0x8, "Allocated bit is not set!");
	cr_assert(bp->header & 0x4, "Prev allocated bit is not set!");
	cr_assert((bp->header & ~0xf & 0xffff) == 32,
		  "Realloc'ed block size not what was expected (found=%lu, exp=%lu)!",
		  bp->header & ~0xf & 0xffff, 32);

	void *z = sf_realloc(y, 8);

	bp = (sf_block *)((void*)z - 2*sizeof(sf_header));
	cr_assert(bp->header & 0x8, "Allocated bit is not set!");
	cr_assert(bp->header & 0x4, "Prev allocated bit is not set!");
	cr_assert((bp->header & ~0xf & 0xffff) == 32,
		  "Realloc'ed block size not what was expected (found=%lu, exp=%lu)!",
		  bp->header & ~0xf & 0xffff, 32);

	sf_free(z);

	assert_free_block_count(4048, 1);
}

Test(sfmm_student_suite, student_test_3, .timeout = TEST_TIMEOUT) {
	void *x = sf_malloc(sizeof(long double) * 9);
	void *y = sf_malloc(100);
	void *z = sf_realloc(x, PAGE_SZ);
	double frag_answer = 4196.0 / 4240.0;

	assert_free_block_count(160, 1);
	assert_free_block_count(3744, 1);
	assert_fragmetation_size(frag_answer);

	sf_free(y);
	sf_free(z);

	assert_free_block_count(8144, 1);
}

Test(sfmm_student_suite, student_test_4, .timeout = TEST_TIMEOUT) {
	void *x = sf_malloc(sizeof(long double) * 9);
	void *y = sf_malloc(100);
	void *z = sf_realloc(x, PAGE_SZ);

	assert_free_block_count(160, 1);
	assert_free_block_count(3744, 1);

	sf_free(y);
	sf_free(z);

	assert_free_block_count(8144, 1);

	double current_heap_size = (double)((void *)sf_mem_end() - (void *)sf_mem_start());
	double util_answer = 4340.0 / current_heap_size;

	assert_utilization_size(util_answer);
}

Test(sfmm_student_suite, student_test_5, .timeout = TEST_TIMEOUT) {
	assert_fragmetation_size(0.0);
	assert_utilization_size(0.0);

	void *x = sf_malloc(sizeof(long double) * 9);
	void *y = sf_realloc(x, 0);

	cr_assert_null(y, "y is not NULL!");
}
