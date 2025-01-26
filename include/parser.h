#ifndef AMCSH_PARSER_H
#define AMCSH_PARSER_H

#include "amcsh.h"

// Parse a command string into the command structure
void amcsh_parse_command(amcsh_command_t *cmd);

// Helper functions for token manipulation
char *amcsh_unquote_token(char *token);
char *amcsh_escape_token(const char *token);

#endif /* AMCSH_PARSER_H */
