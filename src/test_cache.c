#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "cache.h"

// Helper to create dummy data
char* create_dummy_data(size_t size) {
    char* data = malloc(size);
    memset(data, 'A', size); 
    data[size-1] = '\0';    
    return data;
}

int main() {
    printf("=== Starting Cache Tests ===\n");

    file_cache_t cache;
    cache_init(&cache);

    // --- TEST 1: Basic Put and Get ---
    printf("[Test 1] Basic Put and Get... ");
    char* content1 = "<html>Hello World</html>";
    cache_put(&cache, "/index.html", content1, strlen(content1) + 1);

    size_t size_out;
    char* result = cache_get(&cache, "/index.html", &size_out);
    
    if (result && strcmp(result, content1) == 0) {
        printf("PASSED\n");
    } else {
        printf("FAILED (Got: %s)\n", result ? result : "NULL");
    }
    if (result) free(result);

    // --- TEST 2: LRU Eviction Logic ---
    printf("[Test 2] Testing LRU Eviction (Max 10MB)...\n");
    
    // We will add 3 files, each 4MB. 
    // Total = 12MB. The limit is 10MB.
    // Result: The first file should be evicted.

    size_t large_size = 4 * 1024 * 1024; // 4MB
    char* large_data = create_dummy_data(large_size);

    printf("   -> Inserting File A (4MB)...\n");
    cache_put(&cache, "/fileA", large_data, large_size);
    
    printf("   -> Inserting File B (4MB)...\n");
    cache_put(&cache, "/fileB", large_data, large_size);

    printf("   -> Inserting File C (4MB)...\n");
    cache_put(&cache, "/fileC", large_data, large_size);

    // At this point, Cache has B and C (8MB). A should be gone.
    
    printf("   -> Checking if File A is gone... ");
    char* resA = cache_get(&cache, "/fileA", &size_out);
    if (resA == NULL) {
        printf("PASSED (File A evicted)\n");
    } else {
        printf("FAILED (File A still exists!)\n");
        free(resA);
    }

    printf("   -> Checking if File C exists... ");
    char* resC = cache_get(&cache, "/fileC", &size_out);
    if (resC != NULL) {
        printf("PASSED (File C found)\n");
        free(resC);
    } else {
        printf("FAILED (File C missing!)\n");
    }

    free(large_data);

    // --- TEST 3: LRU Update on Access ---
    printf("[Test 3] Testing LRU Update on Access...\n");
    
    // Current State: [Most Recent] C -> B [Least Recent]
    // If we access B, it should move to the front.
    // State becomes: B -> C
    // Then if we add D (4MB), C should be evicted (not B).

    printf("   -> Accessing File B (Making it MRU)...\n");
    char* resB = cache_get(&cache, "/fileB", &size_out);
    if (resB) free(resB);

    printf("   -> Inserting File D (4MB)... (Should evict C, keep B)\n");
    char* large_data2 = create_dummy_data(large_size);
    cache_put(&cache, "/fileD", large_data2, large_size);
    free(large_data2);

    printf("   -> Checking if File B is still there... ");
    resB = cache_get(&cache, "/fileB", &size_out);
    if (resB) {
        printf("PASSED (B was saved because we accessed it)\n");
        free(resB);
    } else {
        printf("FAILED (B was evicted incorrectly)\n");
    }

    // Cleanup
    cache_destroy(&cache);
    printf("=== All Tests Completed ===\n");
    return 0;
}