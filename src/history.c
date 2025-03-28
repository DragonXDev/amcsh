#include "amcsh.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define AMCSH_HISTORY_FILE "/.amcsh_history"

// History storage
static char **history_items = NULL;
static int history_count = 0;
static int history_capacity = 0;

// Add a command to history
void amcsh_history_add(const char *cmd) {
    // Skip empty commands
    if (!cmd || !*cmd || (*cmd == '\n' && *(cmd+1) == '\0')) {
        return;
    }
    
    // Remove trailing newline if present
    char *cmd_copy = strdup(cmd);
    size_t len = strlen(cmd_copy);
    if (len > 0 && cmd_copy[len-1] == '\n') {
        cmd_copy[len-1] = '\0';
    }
    
    // Skip if command is the same as the last one
    if (history_count > 0 && strcmp(history_items[history_count-1], cmd_copy) == 0) {
        free(cmd_copy);
        return;
    }
    
    // Initialize history if needed
    if (!history_items) {
        history_capacity = AMCSH_HISTORY_SIZE;
        history_items = calloc(history_capacity, sizeof(char*));
        if (!history_items) {
            free(cmd_copy);
            return;
        }
    }
    
    // Add command to history
    if (history_count >= history_capacity) {
        // History is full, remove oldest item
        free(history_items[0]);
        memmove(history_items, history_items + 1, (history_capacity - 1) * sizeof(char*));
        history_count = history_capacity - 1;
    }
    
    history_items[history_count++] = cmd_copy;
}

// Get a command from history by index
char *amcsh_history_get(int index) {
    if (index < 0 || index >= history_count) {
        return NULL;
    }
    return history_items[index];
}

// Save history to file
void amcsh_history_save(void) {
    if (!history_items || history_count == 0) {
        return;
    }
    
    char *home = getenv("HOME");
    if (!home) {
        return;
    }
    
    char history_path[AMCSH_MAX_CMD_LENGTH];
    snprintf(history_path, sizeof(history_path), "%s%s", home, AMCSH_HISTORY_FILE);
    
    FILE *fp = fopen(history_path, "w");
    if (!fp) {
        return;
    }
    
    for (int i = 0; i < history_count; i++) {
        fprintf(fp, "%s\n", history_items[i]);
    }
    
    fclose(fp);
}

// Load history from file
void amcsh_history_load(void) {
    char *home = getenv("HOME");
    if (!home) {
        return;
    }
    
    char history_path[AMCSH_MAX_CMD_LENGTH];
    snprintf(history_path, sizeof(history_path), "%s%s", home, AMCSH_HISTORY_FILE);
    
    // Check if file exists
    struct stat st;
    if (stat(history_path, &st) != 0) {
        return;
    }
    
    FILE *fp = fopen(history_path, "r");
    if (!fp) {
        return;
    }
    
    char line[AMCSH_MAX_CMD_LENGTH];
    while (fgets(line, sizeof(line), fp)) {
        amcsh_history_add(line);
    }
    
    fclose(fp);
}

// Search history for a pattern
char **amcsh_history_search(const char *pattern, int *num_results) {
    if (!pattern || !history_items || history_count == 0) {
        *num_results = 0;
        return NULL;
    }
    
    // Count matches
    int count = 0;
    for (int i = 0; i < history_count; i++) {
        if (strstr(history_items[i], pattern)) {
            count++;
        }
    }
    
    if (count == 0) {
        *num_results = 0;
        return NULL;
    }
    
    // Allocate result array
    char **results = calloc(count, sizeof(char*));
    if (!results) {
        *num_results = 0;
        return NULL;
    }
    
    // Fill results
    int j = 0;
    for (int i = 0; i < history_count; i++) {
        if (strstr(history_items[i], pattern)) {
            results[j++] = strdup(history_items[i]);
        }
    }
    
    *num_results = count;
    return results;
}
