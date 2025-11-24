#define _POSIX_C_SOURCE 200809L 
#include "cache.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void cache_init(file_cache_t* cache) {
    cache->head = NULL;
    cache->tail = NULL;
    cache->current_size = 0;
    pthread_rwlock_init(&cache->rwlock, NULL);
}

void cache_destroy(file_cache_t* cache) {
    pthread_rwlock_wrlock(&cache->rwlock);
    cache_entry_t* current = cache->head;
    while (current) {
        cache_entry_t* next = current->next;
        free(current->filename);
        free(current->content);
        free(current);
        current = next;
    }
    pthread_rwlock_unlock(&cache->rwlock);
    pthread_rwlock_destroy(&cache->rwlock);
}

// Internal helper: Move node to front (Must hold Write Lock)
static void move_to_front(file_cache_t* cache, cache_entry_t* entry) {
    if (cache->head == entry) return; 

    // Unlink
    if (entry->prev) entry->prev->next = entry->next;
    if (entry->next) entry->next->prev = entry->prev;
    if (cache->tail == entry) cache->tail = entry->prev;

    // Relink at head
    entry->next = cache->head;
    entry->prev = NULL;
    if (cache->head) cache->head->prev = entry;
    cache->head = entry;
    if (!cache->tail) cache->tail = entry;
}

char* cache_get(file_cache_t* cache, const char* filename, size_t* size_out) {
    pthread_rwlock_rdlock(&cache->rwlock);

    cache_entry_t* current = cache->head;
    while (current) {
        if (strcmp(current->filename, filename) == 0) {
            // Found! Upgrade lock
            pthread_rwlock_unlock(&cache->rwlock);
            pthread_rwlock_wrlock(&cache->rwlock);
            
            // Re-check after lock swap
            current = cache->head; 
            while(current) {
                if (strcmp(current->filename, filename) == 0) {
                    move_to_front(cache, current);
                    *size_out = current->size;
                    
                    char* ret = malloc(current->size);
                    if (ret) memcpy(ret, current->content, current->size);
                    
                    pthread_rwlock_unlock(&cache->rwlock);
                    return ret;
                }
                current = current->next;
            }
            pthread_rwlock_unlock(&cache->rwlock);
            return NULL; 
        }
        current = current->next;
    }
    pthread_rwlock_unlock(&cache->rwlock);
    return NULL;
}

void cache_put(file_cache_t* cache, const char* filename, const char* content, size_t size) {
    if (size > MAX_FILE_SIZE) return;

    pthread_rwlock_wrlock(&cache->rwlock);

    // 1. Check if update existing
    cache_entry_t* current = cache->head;
    while (current) {
        if (strcmp(current->filename, filename) == 0) {
            move_to_front(cache, current);
            pthread_rwlock_unlock(&cache->rwlock);
            return;
        }
        current = current->next;
    }

    // 2. Evict if needed
    while (cache->current_size + size > MAX_CACHE_SIZE && cache->tail) {
        cache_entry_t* old_tail = cache->tail;
        cache->current_size -= old_tail->size;
        
        if (old_tail->prev) old_tail->prev->next = NULL;
        cache->tail = old_tail->prev;
        if (cache->head == old_tail) cache->head = NULL;

        free(old_tail->filename);
        free(old_tail->content);
        free(old_tail);
    }

    // 3. Insert new
    cache_entry_t* new_entry = malloc(sizeof(cache_entry_t));
    if (new_entry) {
        new_entry->filename = strdup(filename);
        new_entry->content = malloc(size);
        if (new_entry->content) memcpy(new_entry->content, content, size);
        new_entry->size = size;

        new_entry->prev = NULL;
        new_entry->next = cache->head;
        if (cache->head) cache->head->prev = new_entry;
        cache->head = new_entry;
        if (!cache->tail) cache->tail = new_entry;

        cache->current_size += size;
    }
    
    pthread_rwlock_unlock(&cache->rwlock);
}