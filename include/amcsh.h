#ifndef AMCSH_H
#define AMCSH_H

#include <stdbool.h>
#include <pthread.h>
#include <sys/types.h>
#include <histedit.h>

#define AMCSH_VERSION "0.1.0"
#define AMCSH_MAX_ARGS 256
#define AMCSH_MAX_CMD_LENGTH 4096
#define AMCSH_HISTORY_SIZE 1000
#define AMCSH_MAX_THREADS 4
#define AMCSH_CMD_CACHE_SIZE 128

// Command cache entry
typedef struct {
    char *cmd;              // Command name
    char *path;            // Full path to executable
    time_t last_used;      // Last time this command was used
    unsigned int uses;     // Number of times this command was used
} amcsh_cmd_cache_entry_t;

// Thread pool worker
typedef struct amcsh_worker {
    pthread_t thread;
    bool active;
    void *(*task)(void *);
    void *args;
    struct amcsh_thread_pool *thread_pool;
} amcsh_worker_t;

// Thread pool for parallel execution
typedef struct amcsh_thread_pool {
    amcsh_worker_t workers[AMCSH_MAX_THREADS];
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_cond;
    bool shutdown;
} amcsh_thread_pool_t;

// Command structure
typedef struct amcsh_command {
    char *raw_cmd;          // Raw command string
    char **argv;           // Command arguments
    int argc;              // Number of arguments
    int redirect_in;       // Input redirection fd
    int redirect_out;      // Output redirection fd
    int pipe_read;        // Read end of pipe
    int pipe_write;       // Write end of pipe
    bool background;      // Run in background?
    struct amcsh_command *next; // Next command in sequence
} amcsh_command_t;

// Job status
typedef enum {
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_DONE
} amcsh_job_status_t;

// Job control structure
typedef struct amcsh_job {
    pid_t pgid;             // Process group ID
    char *command;          // Command string
    amcsh_job_status_t status;  // Job status
    int exit_status;        // Status of the job
    struct amcsh_job *next; // Next job in list
} amcsh_job_t;

// Shell state
typedef struct {
    bool interactive;       // Running interactively?
    int exit_status;       // Exit status of last command
    amcsh_job_t *jobs;     // List of active jobs
    pthread_mutex_t job_mutex;  // Mutex for job list
    amcsh_cmd_cache_entry_t *cmd_cache;  // Command cache
    int cmd_cache_size;    // Size of command cache
    pthread_rwlock_t cache_lock;  // RW lock for cache
    amcsh_thread_pool_t *thread_pool;  // Thread pool
} amcsh_state_t;

// Function declarations
void amcsh_init(void);
void amcsh_cleanup(void);
void amcsh_parse_command(amcsh_command_t *cmd);
int amcsh_execute(amcsh_command_t *cmd);
int amcsh_execute_builtin(amcsh_command_t *cmd);
void amcsh_history_add(const char *line);
void amcsh_history_load(void);
void amcsh_history_save(void);
void amcsh_completion_init(void);
char **amcsh_complete(const char *line, int *num_matches);
void amcsh_free_completions(char **completions, int num_matches);
void amcsh_setup_signals(void);
void amcsh_handle_signal(int signo);
void amcsh_update_jobs(void);
void amcsh_thread_pool_init(amcsh_thread_pool_t *pool);
void amcsh_thread_pool_shutdown(amcsh_thread_pool_t *pool);
char *amcsh_cache_lookup(const char *cmd);
void amcsh_cache_update(const char *cmd, const char *path);
void amcsh_cache_cleanup(void);

// Thread pool operations
void amcsh_thread_pool_submit(amcsh_thread_pool_t *pool, void *(*task)(void *), void *args);

// Built-in commands
int amcsh_builtin_cd(char **args);
int amcsh_builtin_exit(char **args);
int amcsh_builtin_jobs(char **args);
int amcsh_builtin_fg(char **args);
int amcsh_builtin_bg(char **args);
int amcsh_builtin_pwd(char **args);
int amcsh_builtin_echo(char **args);
int amcsh_builtin_help(char **args);
int amcsh_builtin_clear(char **args);

// History management
char *amcsh_history_get(int index);
char **amcsh_history_search(const char *pattern, int *num_results);

// Completion system with trie-based suggestions
typedef struct amcsh_trie_node {
    char c;
    bool is_end;
    char *suggestion;
    struct amcsh_trie_node *children[128];
} amcsh_trie_node_t;

void amcsh_trie_insert(amcsh_trie_node_t *root, const char *word);
char **amcsh_trie_search(amcsh_trie_node_t *root, const char *prefix, int *num_matches);

#endif // AMCSH_H
