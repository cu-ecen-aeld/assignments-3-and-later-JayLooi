#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    struct thread_data *thread_args = (struct thread_data *)thread_param;

    int wait_to_obtain_us = thread_args->wait_to_obtain_ms * 1000;
    int wait_to_release_us = thread_args->wait_to_release_ms * 1000;
    pthread_mutex_t *mutex = thread_args->mutex;

    usleep(wait_to_obtain_us);

    pthread_mutex_lock(mutex);

    usleep(wait_to_release_us);

    pthread_mutex_unlock(mutex);

    thread_args->thread_complete_success = true;

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    
    struct thread_data *thread_param = (struct thread_data *)malloc(sizeof(struct thread_data));

    if (thread_param == NULL)
    {
        return false;
    }

    thread_param->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_param->wait_to_release_ms = wait_to_release_ms;
    thread_param->mutex = mutex;

    if (pthread_create(thread, NULL, threadfunc, (void *)thread_param) != 0)
    {
        return false;
    }

    return true;
}

