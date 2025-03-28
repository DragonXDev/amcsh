#include "amcsh.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <histedit.h>
#include <signal.h>
#include <sys/wait.h>

static EditLine *el = NULL;
static History *hist = NULL;
amcsh_state_t shell_state = {0};

// Prompt callback for libedit
char *prompt(EditLine *e)
{
    static char prompt_buf[AMCSH_MAX_CMD_LENGTH];
    char cwd[AMCSH_MAX_CMD_LENGTH];
    char *home = getenv("HOME");

    getcwd(cwd, sizeof(cwd));

    // Replace home directory with ~
    if (home && strncmp(cwd, home, strlen(home)) == 0)
    {
        snprintf(prompt_buf, sizeof(prompt_buf), "\033[1;36m~%s\033[0m \033[1;32m➜\033[0m ", cwd + strlen(home));
    }
    else
    {
        char *last_dir = strrchr(cwd, '/');
        if (last_dir && *(last_dir + 1))
        {
            snprintf(prompt_buf, sizeof(prompt_buf), "\033[1;36m%s\033[0m \033[1;32m➜\033[0m ", last_dir + 1);
        }
        else
        {
            snprintf(prompt_buf, sizeof(prompt_buf), "\033[1;36m/\033[0m \033[1;32m➜\033[0m ");
        }
    }

    return prompt_buf;
}

// Command completion callback
static unsigned char complete(EditLine *el, int ch)
{
    const LineInfo *li = el_line(el);
    int num_matches;
    
    // Get completions
    char **completions = amcsh_complete(li->buffer, &num_matches);
    
    if (num_matches == 0) {
        return CC_ERROR;  // No completions
    }
    
    if (num_matches == 1) {
        // Single match - complete it
        el_insertstr(el, completions[0] + strlen(li->buffer));
        return CC_REFRESH;
    }
    
    // Multiple matches - show them
    printf("\n");
    for (int i = 0; i < num_matches; i++) {
        printf("%s  ", completions[i]);
    }
    printf("\n");
    
    // Redisplay prompt and line
    el_set(el, EL_REFRESH);
    return CC_REDISPLAY;
}

void amcsh_init(void)
{
    // Initialize shell state
    shell_state.interactive = isatty(STDIN_FILENO);
    shell_state.jobs = NULL;
    
    // Initialize locks with default attributes
    pthread_mutex_init(&shell_state.job_mutex, NULL);
    pthread_rwlock_init(&shell_state.cache_lock, NULL);

    // Pre-allocate command cache with a power of 2 size for faster modulo
    shell_state.cmd_cache = calloc(AMCSH_CMD_CACHE_SIZE, sizeof(amcsh_cmd_cache_entry_t));
    shell_state.cmd_cache_size = AMCSH_CMD_CACHE_SIZE;

    // Initialize thread pool with optimized settings
    shell_state.thread_pool = malloc(sizeof(amcsh_thread_pool_t));
    amcsh_thread_pool_init(shell_state.thread_pool);

    // Setup signal handlers
    amcsh_setup_signals();

    // Initialize completion system (lazy load in background)
    if (shell_state.interactive) {
        pthread_t completion_thread;
        pthread_create(&completion_thread, NULL, (void *(*)(void *))amcsh_completion_init, NULL);
        pthread_detach(completion_thread);
    }

    // Initialize line editing
    if (shell_state.interactive)
    {
        HistEvent ev;
        hist = history_init();
        history(hist, &ev, H_SETSIZE, AMCSH_HISTORY_SIZE);

        el = el_init("amcsh", stdin, stdout, stderr);
        el_set(el, EL_PROMPT, &prompt);
        el_set(el, EL_EDITOR, "emacs");
        el_set(el, EL_HIST, history, hist);
        el_set(el, EL_ADDFN, "complete", "Complete command", complete);
        el_set(el, EL_BIND, "^I", "complete", NULL);
        
        // Load history asynchronously
        pthread_t history_thread;
        pthread_create(&history_thread, NULL, (void *(*)(void *))amcsh_history_load, NULL);
        pthread_detach(history_thread);
    }
}

void amcsh_cleanup(void)
{
    if (shell_state.interactive)
    {
        history_end(hist);
        el_end(el);
    }

    // Save history before exit
    amcsh_history_save();

    // Cleanup thread pool
    amcsh_thread_pool_shutdown(shell_state.thread_pool);
    free(shell_state.thread_pool);

    // Cleanup command cache
    amcsh_cache_cleanup();

    // Cleanup jobs
    pthread_mutex_destroy(&shell_state.job_mutex);
    pthread_rwlock_destroy(&shell_state.cache_lock);
}

int main(int argc, char *argv[])
{
    amcsh_init();

    if (shell_state.interactive)
    {
        printf("\033[1;35mamcsh %s\033[0m - High Performance Shell\n", AMCSH_VERSION);

        const char *line;
        int count;
        char *cmd_copy = NULL;

        while ((line = el_gets(el, &count)))
        {
            if (count <= 1)
                continue;

            // Parse and execute the command
            amcsh_command_t cmd = {0};
            cmd.redirect_in = -1;
            cmd.redirect_out = -1;
            cmd.pipe_read = -1;
            cmd.pipe_write = -1;

            // Reuse command buffer if possible
            if (cmd_copy) {
                free(cmd_copy);
            }
            cmd_copy = strdup(line);
            cmd.raw_cmd = cmd_copy;

            // Parse the command
            amcsh_parse_command(&cmd);
            if (cmd.argc > 0)
            {
                // Fast path for built-in commands
                if (amcsh_execute_builtin(&cmd) == 0) {
                    continue;
                }

                // Check command cache with read lock
                pthread_rwlock_rdlock(&shell_state.cache_lock);
                char *cached_path = amcsh_cache_lookup(cmd.argv[0]);
                if (cached_path) {
                    char *path_copy = strdup(cached_path);
                    pthread_rwlock_unlock(&shell_state.cache_lock);
                    free(cmd.argv[0]);
                    cmd.argv[0] = path_copy;
                } else {
                    pthread_rwlock_unlock(&shell_state.cache_lock);
                }

                // Execute command
                amcsh_execute(&cmd);
                
                // Update history in background
                if (shell_state.interactive) {
                    char *hist_line = strdup(line);
                    pthread_t hist_thread;
                    pthread_create(&hist_thread, NULL, (void *(*)(void *))amcsh_history_add, hist_line);
                    pthread_detach(hist_thread);
                }

                // Update command cache if successful
                if (shell_state.exit_status == 0 && !cached_path) {
                    amcsh_cache_update(cmd.argv[0], cmd.argv[0]);
                }
                
                // Free argv array
                if (cmd.argv) {
                    free(cmd.argv);
                    cmd.argv = NULL;
                }
            }
        }

        if (cmd_copy) {
            free(cmd_copy);
        }
    }
    else
    {
        // Non-interactive mode optimization
        char buffer[AMCSH_MAX_CMD_LENGTH];
        while (fgets(buffer, sizeof(buffer), stdin)) {
            amcsh_command_t cmd = {0};
            cmd.raw_cmd = buffer;
            amcsh_parse_command(&cmd);
            if (cmd.argc > 0) {
                amcsh_execute(&cmd);
            }
        }
    }

    amcsh_cleanup();
    return shell_state.exit_status;
}

void amcsh_setup_signals(void)
{
    signal(SIGINT, amcsh_handle_signal);
    signal(SIGTSTP, amcsh_handle_signal);
    signal(SIGCHLD, amcsh_handle_signal);
}

void amcsh_handle_signal(int signo)
{
    switch (signo)
    {
    case SIGINT:
        // Handle Ctrl+C
        if (shell_state.interactive)
        {
            printf("\n");
            el_reset(el);
        }
        break;

    case SIGTSTP:
        // Handle Ctrl+Z
        break;

    case SIGCHLD:
        // Handle child process status changes
        amcsh_update_jobs();
        break;
    }
}
