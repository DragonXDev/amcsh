#include "amcsh.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
