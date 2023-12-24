/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "debug.h"
#include "sfmm.h"

static int MIN_BLOCK_SIZE = 32;
double total_payload = 0.0;
double total_allocated = 0.0;
double max_aggregate = 0.0;

size_t payload_size();
int initialize();
int free_list_index();
size_t block_size();
size_t prev_block_size();
void header_setting();
int check_wilderness();
void splitting();
void coalescing();

void *sf_malloc(size_t size) {
    // To be implemented.
    size_t renew_size;
    size_t actual_size = size;

    if (size <= 0)
        return NULL;

    if (sf_mem_start() == sf_mem_end()) {
        int init = initialize();
        if (init == 0)
            return NULL;
    }

    if (size + 16 < 32)
        renew_size = 32;

    else {
        renew_size = size + 16;
        while (renew_size % 16 != 0)
            renew_size++;
    }

    int situation = 0;
    int index = free_list_index(renew_size);

    sf_block* find_space;
    sf_block* find_next_space;
    sf_block* dummy;
    int no_null = 0;

    for (int i = index; i < NUM_FREE_LISTS; i++) {
        find_space = &sf_free_list_heads[i];
        find_next_space = find_space->body.links.next;

        while(find_space != find_next_space) {
            if (renew_size <= block_size(find_space->body.links.next)) {
                dummy = find_next_space;
                situation = 1;
                no_null = 1;
                break;
            }
            find_next_space = find_next_space->body.links.next;
        }
    }

    if (no_null == 0) {
        dummy = sf_free_list_heads[NUM_FREE_LISTS-1].body.links.next;
        situation = 2;
    }

    size_t wild_size;
    sf_block* dummy_epilogue;

    switch(situation) {
        case 1:
            splitting(dummy, actual_size, renew_size);

            total_payload += actual_size;
            total_allocated += renew_size;
            if (max_aggregate < total_payload)
                max_aggregate = total_payload;

            return dummy->body.payload;

        case 2:
            size_t original_size = renew_size;
            int first_increase_page = 0;
            wild_size = block_size(dummy);

            while (renew_size > PAGE_SZ) {
                if (sf_mem_grow() == NULL) {
                    sf_errno = ENOMEM;
                    return NULL;
                }

                dummy_epilogue = (sf_block*)((void *)sf_mem_end() - 16);
                dummy_epilogue->header = 0x00000008;

                if(first_increase_page == 0) {
                    dummy = (sf_block*)((void *)dummy_epilogue - PAGE_SZ - wild_size);
                    first_increase_page = 1;
                }

                wild_size += PAGE_SZ;

                int prev_alloc = dummy->prev_footer & 0x8;
                if (prev_alloc != 0)
                    header_setting(dummy, 0, wild_size, 1);
                else
                    header_setting(dummy, 0, wild_size, 0);

                dummy_epilogue->prev_footer = dummy->header;
                renew_size -= PAGE_SZ;
            }

            splitting(dummy, actual_size, original_size);

            total_payload += actual_size;
            total_allocated += original_size;
            if (max_aggregate < total_payload)
                max_aggregate = total_payload;

            return dummy->body.payload;
    }
    return NULL;
}

void sf_free(void *pp) {
    // To be implemented.
    // printf("\n get in free \n");
    if (pp == NULL)
        return abort();

    if ((long int)pp % 16 != 0)
        return abort();

    sf_block* block = (sf_block *)((void *)pp - 16);
    size_t free_size = block_size(block);

    if (free_size < MIN_BLOCK_SIZE)
        return abort();

    if (free_size % 16 != 0)
        return abort();

    if (pp < sf_mem_start() + 32 || pp > sf_mem_end() + 16)
        return abort();

    if ((block->header & 0x8) == 0)
        return abort();

    if ((block->header & 0x4) == 0)
        if ((block->prev_footer & 0x8) != 0)
            return abort();

    // printf("\n passed all abort conditions in free \n");

    size_t fblock_s = block_size(block);
    size_t fpayload_s = payload_size(block);
    int prev_alloc = block->prev_footer & 0x8;
    if (prev_alloc != 0)
        header_setting(block, 0, fblock_s, 1);
    else
        header_setting(block, 0, fblock_s, 0);

    sf_block* next_block_free = (sf_block *)((void *)block + free_size);
    next_block_free->prev_footer = block->header;

    size_t fnblock_s = block_size(next_block_free);
    size_t fnpayload_s = payload_size(next_block_free);

    header_setting(next_block_free, fnpayload_s, fnblock_s, 0);

    coalescing(block);

    total_payload -= fpayload_s;
    total_allocated -= fblock_s;

    // printf("\n free done \n");
}

void *sf_realloc(void *pp, size_t rsize) {
    // To be implemented.
    // printf("\n get in realloc \n");
    if (pp == NULL)
        return NULL;

    if ((long int)pp % 16 != 0)
        return NULL;

    sf_block* block = (sf_block *)((void *)pp - 16);
    size_t pre_alloc_size = block_size(block);

    if (pre_alloc_size < MIN_BLOCK_SIZE)
        return NULL;

    if (pre_alloc_size % 16 != 0)
        return NULL;

    if (pp < sf_mem_start() + 32 || pp > sf_mem_end() + 16)
        return NULL;

    if ((block->header & 0x8) == 0)
        return NULL;

    if ((block->header & 0x4) == 0)
        if ((block->prev_footer & 0x8) != 0)
            return NULL;

    if (rsize == 0) {
        sf_free(pp);
        return NULL;
    }

    // printf("\n passed all abort conditions in realloc \n");

    size_t renew_rsize;
    size_t realpayload_s = payload_size(block);

    if (rsize + 16 < 32)
        renew_rsize = 32;

    else {
        renew_rsize = rsize + 16;
        while (renew_rsize % 16 != 0)
            renew_rsize++;
    }

    // printf("\n rsize %d \n", (int)rsize);
    // printf("\n renew rsize %d \n", (int)renew_rsize);
    // printf("\n pre alloc size %d \n", (int)pre_alloc_size);

    // renew_rsize > pre_alloc_size
    if (renew_rsize > pre_alloc_size) {
        // printf("\n renew_rsize > pre_alloc_size \n");

        void* rpayload = sf_malloc(rsize);
        if (rpayload == NULL)
            return NULL;

        memcpy(rpayload, block->body.payload, rsize);
        sf_free(pp);
        return rpayload;
    }

    // renew_rsize <= pre_alloc_size
    else {
        // printf("\n renew_rsize <= pre_alloc_size \n");

        size_t remain_size = pre_alloc_size - renew_rsize;

        if (remain_size < 32) {
            // printf("\n remain size in realloc < 32 \n");

            int prev_alloc = block->prev_footer & 0x8;
            if (prev_alloc != 0)
                header_setting(block, rsize, pre_alloc_size, 1);
            else
                header_setting(block, rsize, pre_alloc_size, 0);

            sf_block* next_block_r = (sf_block *)((void *)block + pre_alloc_size);
            next_block_r->prev_footer = block->header;

            total_payload -= realpayload_s;
            total_payload += rsize;
            if (max_aggregate < total_payload)
                max_aggregate = total_payload;
        }

        else {
            // printf("\n remain size in realloc >= 32 \n");

            sf_block* lower_part = block;
            sf_block* upper_part = (sf_block *)((void *)block + renew_rsize);

            int prev_alloc = lower_part->prev_footer & 0x8;
            if (prev_alloc != 0)
                header_setting(lower_part, rsize, renew_rsize, 1);
            else
                header_setting(lower_part, rsize, renew_rsize, 0);

            header_setting(upper_part, 0, remain_size, 1);
            coalescing(upper_part);

            size_t up_block_s = block_size(upper_part);
            total_payload -= realpayload_s;
            total_allocated -= up_block_s;
            total_payload += rsize;

            if (max_aggregate < total_payload)
                max_aggregate = total_payload;
        }
        return block->body.payload;
    }
    return NULL;
}

double sf_fragmentation() {
    // To be implemented.

    // printf("\n total payload %f \n", total_payload);
    // printf("\n total allocated %f \n", total_allocated);

    if (total_allocated == 0)
        return 0.0;

    double fragment = total_payload / total_allocated;
    return fragment;
}

double sf_utilization() {
    // To be implemented.

    // printf("\n max aggregate %f \n", max_aggregate);

    if (sf_mem_start() == sf_mem_end())
        return 0.0;

    double util = max_aggregate / (double)((void *)sf_mem_end() - (void *)sf_mem_start());
    return util;
}

int initialize() {
    if (sf_mem_grow() != NULL) {
        sf_block* prologue = (sf_block*)((void *)sf_mem_start());
        sf_block* epilogue = (sf_block*)((void *)sf_mem_end() - 16);
        sf_block* wilderness = (sf_block*)((void *)sf_mem_start() + 32);

        prologue->header = MIN_BLOCK_SIZE | 0x00000008;
        epilogue->header = 0x00000008;
        wilderness->header = (PAGE_SZ - sizeof(epilogue) - MIN_BLOCK_SIZE - 8) | 0x00000004;

        wilderness->prev_footer = prologue->header;
        epilogue->prev_footer = wilderness->header;

        for (int i = 0; i < NUM_FREE_LISTS; i++) {
            sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
            sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
        }

        sf_free_list_heads[NUM_FREE_LISTS-1].body.links.prev = wilderness;
        sf_free_list_heads[NUM_FREE_LISTS-1].body.links.next = wilderness;
        wilderness->body.links.prev = &sf_free_list_heads[NUM_FREE_LISTS-1];
        wilderness->body.links.next = &sf_free_list_heads[NUM_FREE_LISTS-1];

        return 1;
    }

    return 0;
}

int free_list_index(size_t size) {

    int j = 1;
    int k = 1;
    int c;

    for (int i = 2; i < NUM_FREE_LISTS-2; i++) {
        c = j + k;
        j = k;
        k = c;
        if (size == MIN_BLOCK_SIZE)
            return 0;
        else if(size <= k * MIN_BLOCK_SIZE)
            return i-1;
    }
    return NUM_FREE_LISTS-2;
}

size_t block_size(sf_block* block) {
    size_t size = block->header & 0x00000000fffffff0;
    return size;
}

size_t prev_block_size(sf_block* block) {
    size_t size = block->prev_footer & 0x00000000fffffff0;
    return size;
}

void header_setting(sf_block* block, size_t p_size, size_t b_size, int prev_al) {
    block->header = p_size & 0xffffffff;
    block->header = block->header << 32;

    if (p_size == 0){
        if (prev_al == 0) {
            // printf("\n psize = 0 and prev block is not allocated \n");
            block->header = block->header + (b_size);
        }
        else {
            // printf("\n psize = 0 and prev block is allocated \n");
            block->header = block->header + (b_size | 0x00000004);
        }
    }

    else {
        if (prev_al == 0) {
            // printf("\n psize != 0 and prev block is not allocated \n");
            block->header = block->header + (b_size | 0x00000008);
        }
        else {
            // printf("\n psize != 0 and prev block is allocated \n");
            block->header = block->header + (b_size | 0x0000000c);
        }
    }

}

void coalescing(sf_block* block) {
    int prev = block->header & 0x4;
    size_t size = block_size(block);
    sf_block* next_block = (sf_block *)((void *)block + size);
    int next = next_block->header & 0x8;
    size_t next_size = block_size(next_block);
    size_t prev_size = block->prev_footer & 0x00000000fffffff0;
    sf_block* prev_block = (sf_block *)((void *)block - prev_size);
    sf_block* next_next_block;

    // printf("\n prev %d \n", prev);
    // printf("\n next %d \n", next);

    // coalescing case 1
    if (prev != 0 && next != 0) {
        // printf("\n coalescing 1 \n");

        next_block->prev_footer = block->header;
        size_t coalpayload_s = payload_size(next_block);
        header_setting(next_block, coalpayload_s, next_size, 0);

        int index_coal = free_list_index(size);

        block->body.links.next = sf_free_list_heads[index_coal].body.links.next;
        block->body.links.prev = &sf_free_list_heads[index_coal];

        sf_free_list_heads[index_coal].body.links.next = block;
        (block->body.links.next)->body.links.prev = block;
    }

    // coalescing case 2
    else if (prev != 0 && next == 0) {
        // printf("\n coalescing 2 \n");
        (next_block->body.links.prev)->body.links.next = next_block->body.links.next;
        (next_block->body.links.next)->body.links.prev = next_block->body.links.prev;

        size += next_size;

        int prev_alloc = block->header & 0x4;
        if (prev_alloc != 0)
            header_setting(block, 0, size, 1);
        else
            header_setting(block, 0, size, 0);

        next_next_block = (sf_block *)((void *)next_block + next_size);
        next_next_block->prev_footer = block->header;

        if (check_wilderness(block) == 1) {
            // printf("\n coalescing 2 and wilderness \n");
            sf_free_list_heads[NUM_FREE_LISTS-1].body.links.prev = block;
            sf_free_list_heads[NUM_FREE_LISTS-1].body.links.next = block;

            block->body.links.prev = &sf_free_list_heads[NUM_FREE_LISTS-1];
            block->body.links.next = &sf_free_list_heads[NUM_FREE_LISTS-1];
        }

        else {
            // printf("\n coalescing 2 and not wilderness \n");
            int index_coal = free_list_index(size);

            block->body.links.next = sf_free_list_heads[index_coal].body.links.next;
            block->body.links.prev = &sf_free_list_heads[index_coal];

            sf_free_list_heads[index_coal].body.links.next = block;
            (block->body.links.next)->body.links.prev = block;
        }
    }


    // coalescing case 3
    else if (prev == 0 && next != 0) {
        // printf("\n coalescing 3 \n");
        (prev_block->body.links.prev)->body.links.next = prev_block->body.links.next;
        (prev_block->body.links.next)->body.links.prev = prev_block->body.links.prev;

        size += prev_size;

        int prev_alloc = prev_block->header & 0x4;
        if (prev_alloc != 0)
            header_setting(prev_block, 0, size, 1);
        else
            header_setting(prev_block, 0, size, 0);

        next_block->prev_footer = prev_block->header;

        if (check_wilderness(prev_block) == 1) {
            // printf("\n coalescing 3 and wilderness \n");
            sf_free_list_heads[NUM_FREE_LISTS-1].body.links.prev = prev_block;
            sf_free_list_heads[NUM_FREE_LISTS-1].body.links.next = prev_block;

            prev_block->body.links.prev = &sf_free_list_heads[NUM_FREE_LISTS-1];
            prev_block->body.links.next = &sf_free_list_heads[NUM_FREE_LISTS-1];
        }

        else {
            // printf("\n coalescing 3 and not wilderness \n");
            int index_coal = free_list_index(size);

            prev_block->body.links.next = sf_free_list_heads[index_coal].body.links.next;
            prev_block->body.links.prev = &sf_free_list_heads[index_coal];

            sf_free_list_heads[index_coal].body.links.next = prev_block;
            (prev_block->body.links.next)->body.links.prev = prev_block;
        }
    }

    // coalescing case 4
    else {
        // printf("\n coalescing 4 \n");
        // sf_show_heap();
        (next_block->body.links.prev)->body.links.next = next_block->body.links.next;
        (next_block->body.links.next)->body.links.prev = next_block->body.links.prev;

        size += next_size;

        (prev_block->body.links.prev)->body.links.next = prev_block->body.links.next;
        (prev_block->body.links.next)->body.links.prev = prev_block->body.links.prev;
        prev_block->body.links.prev = NULL;
        prev_block->body.links.next = NULL;

        size += prev_size;

        int prev_alloc = prev_block->header & 0x4;
        if (prev_alloc != 0)
            header_setting(prev_block, 0, size, 1);
        else
            header_setting(prev_block, 0, size, 0);

        next_next_block = (sf_block *)((void *)prev_block + size);
        next_next_block->prev_footer = prev_block->header;

        if (check_wilderness(prev_block) == 1) {
            // printf("\n coalescing 4 and wilderness \n");
            sf_free_list_heads[NUM_FREE_LISTS-1].body.links.prev = prev_block;
            sf_free_list_heads[NUM_FREE_LISTS-1].body.links.next = prev_block;

            prev_block->body.links.prev = &sf_free_list_heads[NUM_FREE_LISTS-1];
            prev_block->body.links.next = &sf_free_list_heads[NUM_FREE_LISTS-1];
        }

        else {
            // printf("\n coalescing 4 and not wilderness \n");
            int index_coal = free_list_index(size);

            prev_block->body.links.next = sf_free_list_heads[index_coal].body.links.next;
            prev_block->body.links.prev = &sf_free_list_heads[index_coal];

            sf_free_list_heads[index_coal].body.links.next = prev_block;
            (prev_block->body.links.next)->body.links.prev = prev_block;
        }
    }
}

int check_wilderness(sf_block* block) {
    sf_block* epi = (sf_block *)((void *)sf_mem_end() - 16);
    size_t next_size = block_size(block);
    sf_block* next = (sf_block *)((void *)block + next_size);

    if (epi == next)
        return 1;
    else
        return 0;
}

void splitting(sf_block* free_block, size_t real_size, size_t alloc_size) {
    size_t remain_size = block_size(free_block) - alloc_size;

    if (remain_size < 32) {
        // printf("\n remain size < 32 \n");
        int prev_alloc = free_block->prev_footer & 0x8;
        if (prev_alloc != 0)
            header_setting(free_block, 0, alloc_size, 1);
        else
            header_setting(free_block, 0, alloc_size, 0);

        (free_block->body.links.prev)->body.links.next = free_block->body.links.next;
        (free_block->body.links.next)->body.links.prev = free_block->body.links.prev;
    }

    else {
        // printf("\n remain size >= 32 \n");
        sf_block* lower_part = free_block;
        sf_block* upper_part = (sf_block *)((void *)free_block + alloc_size);
        sf_block* upper_next = (sf_block *)((void *)upper_part + remain_size);

        (lower_part->body.links.prev)->body.links.next = lower_part->body.links.next;
        (lower_part->body.links.next)->body.links.prev = lower_part->body.links.prev;

        if (check_wilderness(free_block) == 1) {
            // printf("\n remain size >= 32 and wilderness \n");
            sf_free_list_heads[NUM_FREE_LISTS-1].body.links.prev = upper_part;
            sf_free_list_heads[NUM_FREE_LISTS-1].body.links.next = upper_part;

            upper_part->body.links.prev = &sf_free_list_heads[NUM_FREE_LISTS-1];
            upper_part->body.links.next = &sf_free_list_heads[NUM_FREE_LISTS-1];
        }

        else {
            // printf("\n remain size >= 32 and not wilderness \n");
            int index = free_list_index(remain_size);
            upper_part->body.links.next = sf_free_list_heads[index].body.links.next;
            upper_part->body.links.prev = &sf_free_list_heads[index];

            sf_free_list_heads[index].body.links.next = upper_part;
            (upper_part->body.links.next)->body.links.prev = upper_part;
        }

        int prev_alloc = lower_part->prev_footer & 0x8;
        if (prev_alloc != 0)
            header_setting(lower_part, real_size, alloc_size, 1);
        else
            header_setting(lower_part, real_size, alloc_size, 0);

        header_setting(upper_part, 0, remain_size, 1);
        upper_part->prev_footer = lower_part->header;
        upper_next->prev_footer = upper_part->header;
    }
}

size_t payload_size(sf_block* block) {
    size_t pl_size = (block->header & 0xffffffff00000000) >>32;
    return pl_size;
}
