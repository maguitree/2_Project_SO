#ifndef CACHE_H
#define CACHE_H

#include <stddef.h>  
#include <pthread.h> 

#define MAX_CACHE_SIZE (10 * 1024 * 1024) 
#define MAX_FILE_SIZE (5 * 1024 * 1024)

typedef struct cache_entry {
    char* filename;             
    char* content;              
    size_t size;                
    struct cache_entry* prev;   
    struct cache_entry* next;   
} cache_entry_t;

typedef struct {
    cache_entry_t* head;        
    cache_entry_t* tail;        
    size_t current_size;
    pthread_rwlock_t rwlock;    
} file_cache_t;

void cache_init(file_cache_t* cache);
void cache_destroy(file_cache_t* cache);
char* cache_get(file_cache_t* cache, const char* filename, size_t* size_out);
void cache_put(file_cache_t* cache, const char* filename, const char* content, size_t size);

#endif