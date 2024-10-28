#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#define MEMORY_SIZE 1024 * 1024 // 1 MB
#define CACHE_SIZE 1024 // 1 KB Cache
#define CACHE_BLOCK_SIZE 64

typedef struct Block {
    size_t size;
    bool is_free;
    struct Block *next;
    struct Block *prev;
} Block;

static char memory[MEMORY_SIZE];
static Block *free_list = NULL;

typedef struct {
    int tag;
    bool valid;
} CacheLine;

static CacheLine cache[CACHE_SIZE / CACHE_BLOCK_SIZE];
static int cache_hits = 0;
static int cache_misses = 0;
static int cache_evictions = 0;

FILE *memory_log;
FILE *cache_log;

void init_cache() {
    for (int i = 0; i < CACHE_SIZE / CACHE_BLOCK_SIZE; i++) {
        cache[i].valid = false;
    }
}

void log_cache_stats() {
    fprintf(cache_log, "%d,%d,%d\n", cache_hits, cache_misses, cache_evictions);
    fflush(cache_log);
}

void access_cache(int address) {
    int block_number = (address / CACHE_BLOCK_SIZE) % (CACHE_SIZE / CACHE_BLOCK_SIZE);
    int tag = address / CACHE_SIZE;

    if (cache[block_number].valid && cache[block_number].tag == tag) {
        cache_hits++;
    } else {
        if (cache[block_number].valid) {
            cache_evictions++;
        }
        cache[block_number].valid = true;
        cache[block_number].tag = tag;
        cache_misses++;
    }
    log_cache_stats();
}

void log_memory_state() {
    Block *current = (Block *)memory;
    while ((char *)current < memory + MEMORY_SIZE) {
        fprintf(memory_log, "%p,%zu,%d\n", (void *)current, current->size, current->is_free);
        current = (Block *)((char *)current + current->size + sizeof(Block));
    }
    fprintf(memory_log, "END\n");
    fflush(memory_log);
}

Block *find_best_fit(size_t size) {
    Block *best_fit = NULL;
    Block *current = free_list;

    while (current) {
        if (current->is_free && current->size >= size) {
            if (!best_fit || current->size < best_fit->size) {
                best_fit = current;
            }
        }
        current = current->next;
    }
    return best_fit;
}

void split_block(Block *block, size_t size) {
    Block *new_block = (Block *)((char *)block + sizeof(Block) + size);
    new_block->size = block->size - size - sizeof(Block);
    new_block->is_free = true;
    new_block->next = block->next;
    new_block->prev = block;

    if (new_block->next) {
        new_block->next->prev = new_block;
    }

    block->size = size;
    block->is_free = false;
    block->next = new_block;
}

void *custom_malloc(size_t size) {
    Block *block = find_best_fit(size);

    if (!block) {
        return NULL;
    }

    if (block->size > size + sizeof(Block)) {
        split_block(block, size);
    } else {
        block->is_free = false;
    }

    access_cache((int)((char *)block - memory));
    log_memory_state();

    return (char *)block + sizeof(Block);
}

void coalesce(Block *block) {
    if (block->next && block->next->is_free) {
        block->size += sizeof(Block) + block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }

    if (block->prev && block->prev->is_free) {
        block->prev->size += sizeof(Block) + block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}

void custom_free(void *ptr) {
    if (!ptr) return;

    Block *block = (Block *)((char *)ptr - sizeof(Block));
    block->is_free = true;

    access_cache((int)((char *)block - memory));
    log_memory_state();

    coalesce(block);
}

void init_allocator() {
    free_list = (Block *)memory;
    free_list->size = MEMORY_SIZE - sizeof(Block);
    free_list->is_free = true;
    free_list->next = NULL;
    free_list->prev = NULL;

    // Open log files
    memory_log = fopen("memory_log.csv", "w");
    cache_log = fopen("cache_log.csv", "w");

    // Write headers to log files
    fprintf(memory_log, "Address,Size,Is_Free\n");
    fprintf(cache_log, "Cache_Hits,Cache_Misses,Cache_Evictions\n");
}

void close_logs() {
    fclose(memory_log);
    fclose(cache_log);
}

int main() {
    init_allocator();
    init_cache();

    void *ptr1 = custom_malloc(256);
    void *ptr2 = custom_malloc(128);
    void *ptr3 = custom_malloc(512);

    custom_free(ptr2);

    void *ptr4 = custom_malloc(128);

    close_logs();
    return 0;
}
