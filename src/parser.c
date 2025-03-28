#include "amcsh.h"
#include "parser.h"
#include <string.h>
#include <ctype.h>

// Fast string tokenization without copying
static char *skip_whitespace(char *str) {
    while (isspace(*str)) str++;
    return str;
}

static char *find_token_end(char *str) {
    while (*str && !isspace(*str)) {
        if (*str == '"' || *str == '\'') {
            char quote = *str++;
            while (*str && *str != quote) str++;
            if (*str) str++;
        } else {
            str++;
        }
    }
    return str;
}

void amcsh_parse_command(amcsh_command_t *cmd) {
    char *current = cmd->raw_cmd;
    cmd->argc = 0;
    
    // Allocate memory for argv array
    cmd->argv = (char **)malloc(AMCSH_MAX_ARGS * sizeof(char *));
    
    while (*current && cmd->argc < AMCSH_MAX_ARGS - 1) {
        current = skip_whitespace(current);
        if (!*current) break;
        
        // Handle special characters
        if (*current == '|' || *current == '>' || *current == '<' || *current == '&') {
            switch (*current) {
                case '|':
                    // TODO: Setup pipe
                    break;
                case '>':
                    if (*(current + 1) == '>') {
                        cmd->append_out = true;
                        current++;
                    }
                    // TODO: Setup output redirection
                    break;
                case '<':
                    // TODO: Setup input redirection
                    break;
                case '&':
                    cmd->background = true;
                    break;
            }
            current++;
            continue;
        }
        
        // Handle quoted strings
        if (*current == '"' || *current == '\'') {
            char quote = *current;
            cmd->argv[cmd->argc++] = ++current;
            while (*current && *current != quote) current++;
            if (*current) *current++ = '\0';
        } else {
            cmd->argv[cmd->argc++] = current;
            char *end = find_token_end(current);
            if (*end) *end++ = '\0';
            current = end;
        }
    }
    
    cmd->argv[cmd->argc] = NULL;
}
