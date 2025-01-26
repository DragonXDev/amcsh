#include "amcsh.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

extern amcsh_state_t shell_state;

static int find_cache_slot(const char *cmd) {
    // Simple hash function
    unsigned int hash = 0;
    while (*cmd) {
        hash = (hash * 31) + *cmd++;
    }
    return hash % shell_state.cmd_cache_size;
}

static int find_lru_slot(void) {
    time_t oldest_time = time(NULL);
    unsigned int least_uses = UINT_MAX;
    int slot = 0;
    
    // Find the least recently used slot with fewest uses
    for (int i = 0; i < shell_state.cmd_cache_size; i++) {
        if (!shell_state.cmd_cache[i].cmd) {
            return i;  // Empty slot
        }
        
        if (shell_state.cmd_cache[i].last_used < oldest_time ||
            (shell_state.cmd_cache[i].last_used == oldest_time &&
             shell_state.cmd_cache[i].uses < least_uses)) {
            oldest_time = shell_state.cmd_cache[i].last_used;
            least_uses = shell_state.cmd_cache[i].uses;
            slot = i;
        }
    }
    
    return slot;
}

static void init_cache_entry(amcsh_cmd_cache_entry_t *entry) {
    entry->path = NULL;
    entry->last_used = 0;
    entry->uses = 0;
    pthread_rwlock_init(&entry->lock, NULL);
}

static void free_cache_entry(amcsh_cmd_cache_entry_t *entry) {
    if (entry->path) {
        free(entry->path);
    }
    pthread_rwlock_destroy(&entry->lock);
}

char *amcsh_cache_lookup(const char *cmd) {
    pthread_rwlock_rdlock(&shell_state.cache_lock);
    
    int slot = find_cache_slot(cmd);
    
    if (shell_state.cmd_cache[slot].cmd &&
        strcmp(shell_state.cmd_cache[slot].cmd, cmd) == 0) {
        // Update access time and count
        shell_state.cmd_cache[slot].last_used = time(NULL);
        shell_state.cmd_cache[slot].uses++;
        char *path = strdup(shell_state.cmd_cache[slot].path);
        pthread_rwlock_unlock(&shell_state.cache_lock);
        return path;
    }
    
    pthread_rwlock_unlock(&shell_state.cache_lock);
    return NULL;
}

void amcsh_cache_update(const char *cmd, const char *path) {
    pthread_rwlock_wrlock(&shell_state.cache_lock);
    
    int slot = find_cache_slot(cmd);
    
    // If slot is occupied by a different command, find LRU slot
    if (shell_state.cmd_cache[slot].cmd &&
        strcmp(shell_state.cmd_cache[slot].cmd, cmd) != 0) {
        slot = find_lru_slot();
    }
    
    // Free old entry if it exists
    if (shell_state.cmd_cache[slot].cmd) {
        free(shell_state.cmd_cache[slot].cmd);
        free(shell_state.cmd_cache[slot].path);
    }
    
    // Add new entry
    shell_state.cmd_cache[slot].cmd = strdup(cmd);
    shell_state.cmd_cache[slot].path = strdup(path);
    shell_state.cmd_cache[slot].last_used = time(NULL);
    shell_state.cmd_cache[slot].uses = 1;
    
    pthread_rwlock_unlock(&shell_state.cache_lock);
}

void amcsh_cache_cleanup(void) {
    if (!shell_state.cmd_cache) {
        return;
    }
    
    for (int i = 0; i < shell_state.cmd_cache_size; i++) {
        free_cache_entry(&shell_state.cmd_cache[i]);
    }
    
    free(shell_state.cmd_cache);
    shell_state.cmd_cache = NULL;
    shell_state.cmd_cache_size = 0;
    
    pthread_rwlock_destroy(&shell_state.cache_lock);
}
