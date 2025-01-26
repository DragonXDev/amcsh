#include "amcsh.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>

static amcsh_trie_node_t *root = NULL;
static char **completion_results = NULL;
static int completion_count = 0;
static int completion_capacity = 0;

static amcsh_trie_node_t *create_node(char c) {
    amcsh_trie_node_t *node = calloc(1, sizeof(amcsh_trie_node_t));
    node->c = c;
    return node;
}

void amcsh_completion_init(void) {
    root = create_node('\0');
    
    // Add built-in commands to trie
    const char *builtins[] = {
        "cd", "exit", "jobs", "fg", "bg", "help",
        "history", "alias", "unalias", "export",
        "echo", "pwd", "source", NULL
    };
    
    for (const char **cmd = builtins; *cmd; cmd++) {
        amcsh_trie_insert(root, *cmd);
    }
    
    // Add executables from PATH
    char *path = getenv("PATH");
    if (path) {
        char *path_copy = strdup(path);
        char *dir = strtok(path_copy, ":");
        
        while (dir) {
            DIR *d = opendir(dir);
            if (d) {
                struct dirent *entry;
                while ((entry = readdir(d))) {
                    if (entry->d_type == DT_REG || entry->d_type == DT_LNK) {
                        amcsh_trie_insert(root, entry->d_name);
                    }
                }
                closedir(d);
            }
            dir = strtok(NULL, ":");
        }
        
        free(path_copy);
    }
}

void amcsh_trie_insert(amcsh_trie_node_t *root, const char *word) {
    amcsh_trie_node_t *node = root;
    
    while (*word) {
        int idx = (unsigned char)*word;
        if (!node->children[idx]) {
            node->children[idx] = create_node(*word);
        }
        node = node->children[idx];
        word++;
    }
    
    node->is_end = true;
    if (node->suggestion) {
        free(node->suggestion);
    }
    node->suggestion = strdup(word - strlen(word));
}

static void add_completion(const char *word) {
    if (completion_count >= completion_capacity) {
        completion_capacity = completion_capacity ? completion_capacity * 2 : 16;
        completion_results = realloc(completion_results, 
                                   completion_capacity * sizeof(char *));
    }
    completion_results[completion_count++] = strdup(word);
}

static void collect_suggestions(amcsh_trie_node_t *node, char *prefix, int depth) {
    if (node->is_end) {
        add_completion(prefix);
    }
    
    for (int i = 0; i < 128; i++) {
        if (node->children[i]) {
            prefix[depth] = (char)i;
            prefix[depth + 1] = '\0';
            collect_suggestions(node->children[i], prefix, depth + 1);
        }
    }
}

char **amcsh_trie_search(amcsh_trie_node_t *root, const char *prefix, int *num_matches) {
    amcsh_trie_node_t *node = root;
    
    // Navigate to prefix node
    while (*prefix) {
        int idx = (unsigned char)*prefix;
        if (!node->children[idx]) {
            *num_matches = 0;
            return NULL;
        }
        node = node->children[idx];
        prefix++;
    }
    
    // Reset completion results
    for (int i = 0; i < completion_count; i++) {
        free(completion_results[i]);
    }
    completion_count = 0;
    
    // Collect all suggestions
    char buffer[AMCSH_MAX_CMD_LENGTH];
    strcpy(buffer, prefix - strlen(prefix));
    collect_suggestions(node, buffer, strlen(buffer));
    
    *num_matches = completion_count;
    return completion_results;
}

char **amcsh_complete(const char *line, int *num_matches) {
    // Find the command word being completed
    const char *word_start = line;
    const char *word_end = line + strlen(line);
    
    // Move backward to find start of current word
    while (word_end > line && isspace(*(word_end - 1))) {
        word_end--;
    }
    word_start = word_end;
    while (word_start > line && !isspace(*(word_start - 1))) {
        word_start--;
    }
    
    // Extract the word to complete
    char word[AMCSH_MAX_CMD_LENGTH];
    size_t len = word_end - word_start;
    strncpy(word, word_start, len);
    word[len] = '\0';
    
    return amcsh_trie_search(root, word, num_matches);
}

void amcsh_free_completions(char **completions, int num_matches) {
    // Completions are managed internally
}
