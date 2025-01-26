#include "amcsh.h"
#include <stdlib.h>
#include <string.h>

static void *worker_thread(void *arg) {
    amcsh_worker_t *worker = (amcsh_worker_t *)arg;
    
    while (1) {
        // Wait for work
        pthread_mutex_lock(&worker->thread_pool->queue_mutex);
        while (!worker->active && !worker->thread_pool->shutdown) {
            pthread_cond_wait(&worker->thread_pool->queue_cond, 
                            &worker->thread_pool->queue_mutex);
        }
        
        if (worker->thread_pool->shutdown) {
            pthread_mutex_unlock(&worker->thread_pool->queue_mutex);
            break;
        }
        
        // Execute task
        void *(*task)(void *) = worker->task;
        void *task_arg = worker->args;
        worker->active = false;
        
        pthread_mutex_unlock(&worker->thread_pool->queue_mutex);
        
        if (task) {
            task(task_arg);
        }
    }
    
    return NULL;
}

void amcsh_thread_pool_init(amcsh_thread_pool_t *pool) {
    pthread_mutex_init(&pool->queue_mutex, NULL);
    pthread_cond_init(&pool->queue_cond, NULL);
    pool->shutdown = false;
    
    // Initialize workers
    for (int i = 0; i < AMCSH_MAX_THREADS; i++) {
        pool->workers[i].active = false;
        pool->workers[i].task = NULL;
        pool->workers[i].args = NULL;
        pool->workers[i].thread_pool = pool;
        pthread_create(&pool->workers[i].thread, NULL, worker_thread, &pool->workers[i]);
    }
}

void amcsh_thread_pool_submit(amcsh_thread_pool_t *pool, void *(*task)(void *), void *args) {
    pthread_mutex_lock(&pool->queue_mutex);
    
    // Find an inactive worker
    for (int i = 0; i < AMCSH_MAX_THREADS; i++) {
        if (!pool->workers[i].active) {
            pool->workers[i].task = task;
            pool->workers[i].args = args;
            pool->workers[i].active = true;
            pthread_cond_signal(&pool->queue_cond);
            pthread_mutex_unlock(&pool->queue_mutex);
            return;
        }
    }
    
    // No inactive worker found, execute in current thread
    pthread_mutex_unlock(&pool->queue_mutex);
    task(args);
}

void amcsh_thread_pool_shutdown(amcsh_thread_pool_t *pool) {
    pthread_mutex_lock(&pool->queue_mutex);
    pool->shutdown = true;
    pthread_cond_broadcast(&pool->queue_cond);
    pthread_mutex_unlock(&pool->queue_mutex);
    
    // Wait for all threads to finish
    for (int i = 0; i < AMCSH_MAX_THREADS; i++) {
        pthread_join(pool->workers[i].thread, NULL);
    }
    
    pthread_mutex_destroy(&pool->queue_mutex);
    pthread_cond_destroy(&pool->queue_cond);
}
