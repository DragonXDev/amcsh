#include "amcsh.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

extern amcsh_state_t shell_state;

int amcsh_builtin_cd(char **args) {
    if (!args[1]) {
        // No argument - go to home directory
        char *home = getenv("HOME");
        if (!home) {
            fprintf(stderr, "amcsh: cd: HOME not set\n");
            return 1;
        }
        if (chdir(home) != 0) {
            perror("amcsh: cd");
            return 1;
        }
    } else {
        if (chdir(args[1]) != 0) {
            perror("amcsh: cd");
            return 1;
        }
    }
    return 0;
}

int amcsh_builtin_jobs(char **args) {
    pthread_mutex_lock(&shell_state.job_mutex);
    amcsh_job_t *job = shell_state.jobs;
    int job_num = 1;

    while (job) {
        const char *status_str;
        switch (job->status) {
            case JOB_RUNNING:
                status_str = "Running";
                break;
            case JOB_STOPPED:
                status_str = "Stopped";
                break;
            case JOB_DONE:
                status_str = "Done";
                break;
            default:
                status_str = "Unknown";
        }

        printf("[%d] %s\t%s\n", job_num++, status_str, job->command);
        job = job->next;
    }
    pthread_mutex_unlock(&shell_state.job_mutex);
    return 0;
}

int amcsh_builtin_fg(char **args) {
    // TODO: Implement foreground job control
    return 0;
}

int amcsh_builtin_bg(char **args) {
    // TODO: Implement background job control
    return 0;
}

int amcsh_builtin_exit(char **args) {
    // TODO: Clean up resources before exit
    exit(shell_state.exit_status);
    return 0;
}

int amcsh_builtin_pwd(char **args) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
        return 0;
    } else {
        perror("amcsh: pwd");
        return 1;
    }
}

int amcsh_builtin_echo(char **args) {
    int i = 1;
    bool newline = true;
    
    // Check for -n option (no newline)
    if (args[1] && strcmp(args[1], "-n") == 0) {
        newline = false;
        i++;
    }
    
    // Print all arguments with spaces between them
    while (args[i]) {
        printf("%s", args[i]);
        if (args[i+1]) {
            printf(" ");
        }
        i++;
    }
    
    if (newline) {
        printf("\n");
    }
    
    return 0;
}

// Help information for built-in commands
static const struct {
    const char *name;
    const char *desc;
} builtin_help[] = {
    {"cd", "Change the current directory"},
    {"clear", "Clear the terminal screen"},
    {"echo", "Display a line of text"},
    {"exit", "Exit the shell"},
    {"fg", "Move job to foreground"},
    {"bg", "Move job to background"},
    {"help", "Display information about built-in commands"},
    {"history", "Display command history"},
    {"jobs", "List active jobs"},
    {"pwd", "Print the current working directory"},
    {NULL, NULL}
};

int amcsh_builtin_help(char **args) {
    if (!args[1]) {
        // No argument - list all built-in commands
        printf("\033[1;36mamcsh %s\033[0m - High Performance Shell\n\n", AMCSH_VERSION);
        printf("These shell commands are defined internally.\n");
        printf("Type 'help name' to find out more about the function 'name'.\n\n");
        
        printf("Built-in commands:\n");
        for (int i = 0; builtin_help[i].name; i++) {
            printf("  \033[1;32m%-10s\033[0m %s\n", builtin_help[i].name, builtin_help[i].desc);
        }
    } else {
        // Find specific command help
        for (int i = 0; builtin_help[i].name; i++) {
            if (strcmp(args[1], builtin_help[i].name) == 0) {
                printf("%s: %s\n", builtin_help[i].name, builtin_help[i].desc);
                
                // Additional usage information for specific commands
                if (strcmp(args[1], "cd") == 0) {
                    printf("Usage: cd [directory]\n");
                    printf("  Changes the current directory to [directory].\n");
                    printf("  If no directory is specified, changes to the home directory.\n");
                } else if (strcmp(args[1], "echo") == 0) {
                    printf("Usage: echo [-n] [string...]\n");
                    printf("  Display the STRING(s) on standard output.\n");
                    printf("  -n    do not output the trailing newline\n");
                } else if (strcmp(args[1], "jobs") == 0) {
                    printf("Usage: jobs\n");
                    printf("  Lists all jobs that are running in the background.\n");
                } else if (strcmp(args[1], "fg") == 0) {
                    printf("Usage: fg [job_id]\n");
                    printf("  Brings the specified job to the foreground.\n");
                } else if (strcmp(args[1], "bg") == 0) {
                    printf("Usage: bg [job_id]\n");
                    printf("  Continues the specified job in the background.\n");
                } else if (strcmp(args[1], "history") == 0) {
                    printf("Usage: history [n]\n");
                    printf("  Display the command history list with line numbers.\n");
                    printf("  An optional argument 'n' limits the number of entries shown.\n");
                }
                
                return 0;
            }
        }
        
        fprintf(stderr, "amcsh: help: no help topics match '%s'\n", args[1]);
        return 1;
    }
    
    return 0;
}

int amcsh_builtin_clear(char **args) {
    // ANSI escape sequence to clear screen and move cursor to home position
    printf("\033[2J\033[H");
    return 0;
}

int amcsh_builtin_history(char **args) {
    int limit = AMCSH_HISTORY_SIZE;
    
    // Check if a limit is specified
    if (args[1]) {
        char *endptr;
        int num = strtol(args[1], &endptr, 10);
        if (*endptr == '\0' && num > 0) {
            limit = num;
        }
    }
    
    // Get history items
    for (int i = 0; i < limit; i++) {
        char *cmd = amcsh_history_get(i);
        if (cmd) {
            printf("%5d  %s\n", i + 1, cmd);
        } else {
            break;
        }
    }
    
    return 0;
}
