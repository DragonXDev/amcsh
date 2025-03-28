#include "amcsh.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <spawn.h>
#include <errno.h>

extern char **environ;
extern amcsh_state_t shell_state;

// Built-in command function type
typedef int (*builtin_func)(char **);

// Built-in command structure
typedef struct {
    const char *name;
    builtin_func func;
} builtin_cmd_t;

// List of built-in commands
static builtin_cmd_t builtins[] = {
    {"cd", amcsh_builtin_cd},
    {"exit", amcsh_builtin_exit},
    {"jobs", amcsh_builtin_jobs},
    {"fg", amcsh_builtin_fg},
    {"bg", amcsh_builtin_bg},
    {"pwd", amcsh_builtin_pwd},
    {NULL, NULL}
};

// Find built-in command
static builtin_func get_builtin(const char *cmd) {
    for (builtin_cmd_t *builtin = builtins; builtin->name; builtin++) {
        if (strcmp(cmd, builtin->name) == 0) {
            return builtin->func;
        }
    }
    return NULL;
}

// Fast path for built-in commands
int amcsh_execute_builtin(amcsh_command_t *cmd) {
    if (strcmp(cmd->argv[0], "cd") == 0) {
        return amcsh_builtin_cd(cmd->argv);
    } else if (strcmp(cmd->argv[0], "exit") == 0) {
        return amcsh_builtin_exit(cmd->argv);
    }
    return -1; // Not a builtin
}

int amcsh_execute(amcsh_command_t *cmd)
{
    if (!cmd || !cmd->argv[0]) {
        return -1;
    }

    // Check for built-in commands
    builtin_func builtin = get_builtin(cmd->argv[0]);
    if (builtin) {
        return builtin(cmd->argv);
    }

    // Setup file actions for redirection
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);

    if (cmd->redirect_in >= 0) {
        posix_spawn_file_actions_adddup2(&actions, cmd->redirect_in, STDIN_FILENO);
        posix_spawn_file_actions_addclose(&actions, cmd->redirect_in);
    }
    if (cmd->redirect_out >= 0) {
        posix_spawn_file_actions_adddup2(&actions, cmd->redirect_out, STDOUT_FILENO);
        posix_spawn_file_actions_addclose(&actions, cmd->redirect_out);
    }
    if (cmd->pipe_read >= 0) {
        posix_spawn_file_actions_adddup2(&actions, cmd->pipe_read, STDIN_FILENO);
        posix_spawn_file_actions_addclose(&actions, cmd->pipe_read);
    }
    if (cmd->pipe_write >= 0) {
        posix_spawn_file_actions_adddup2(&actions, cmd->pipe_write, STDOUT_FILENO);
        posix_spawn_file_actions_addclose(&actions, cmd->pipe_write);
    }

    // Setup spawn attributes
    posix_spawnattr_t attr;
    posix_spawnattr_init(&attr);

    // Set the process group
    if (shell_state.interactive) {
        posix_spawnattr_setpgroup(&attr, 0);
        posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETPGROUP);
    }

    // Spawn the process
    pid_t pid;
    int status = posix_spawnp(&pid, cmd->argv[0], &actions, &attr, cmd->argv, environ);

    // Cleanup
    posix_spawn_file_actions_destroy(&actions);
    posix_spawnattr_destroy(&attr);

    if (status != 0) {
        fprintf(stderr, "amcsh: command not found: %s\n", cmd->argv[0]);
        shell_state.exit_status = 127;
        return -1;
    }

    // Close pipe/redirect file descriptors in parent
    if (cmd->redirect_in >= 0) close(cmd->redirect_in);
    if (cmd->redirect_out >= 0) close(cmd->redirect_out);
    if (cmd->pipe_read >= 0) close(cmd->pipe_read);
    if (cmd->pipe_write >= 0) close(cmd->pipe_write);

    // Wait for the command to finish if not background
    if (!cmd->background) {
        int wstatus;
        waitpid(pid, &wstatus, 0);
        shell_state.exit_status = WIFEXITED(wstatus) ? WEXITSTATUS(wstatus) : 1;
    } else {
        // Add to job list
        amcsh_job_t *job = malloc(sizeof(amcsh_job_t));
        job->pgid = pid;
        job->command = strdup(cmd->raw_cmd);
        job->status = JOB_RUNNING;
        job->next = NULL;

        pthread_mutex_lock(&shell_state.job_mutex);
        if (!shell_state.jobs) {
            shell_state.jobs = job;
        } else {
            job->next = shell_state.jobs;
            shell_state.jobs = job;
        }
        pthread_mutex_unlock(&shell_state.job_mutex);
    }

    return 0;
}
