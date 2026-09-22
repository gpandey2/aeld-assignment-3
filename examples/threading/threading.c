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

    int ret = 0;
    struct timespec ts;
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    thread_func_args->thread_complete_success = true;
  
    printf("sleeping for %d ms to obtain the lock\n", thread_func_args->wait_to_obtain_ms);
    ts.tv_sec = thread_func_args->wait_to_obtain_ms / 1000;
    ts.tv_nsec = (thread_func_args->wait_to_obtain_ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
    ret = pthread_mutex_lock(thread_func_args->mutex);
    if (ret != 0) {
        printf("Failed to obtain the mutex lock post sleep. rc : %d\n", ret);
        thread_func_args->thread_complete_success = false;
        return thread_param;
    }

    printf("sleeping for %d ms to release the lock\n", thread_func_args->wait_to_release_ms);
    ts.tv_sec = thread_func_args->wait_to_release_ms / 1000;
    ts.tv_nsec = (thread_func_args->wait_to_release_ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
    ret = pthread_mutex_unlock(thread_func_args->mutex);
    if (ret != 0) {
        printf("Failed to release the mutex lock post sleep. rc : %d\n", ret);
        thread_func_args->thread_complete_success = false;
        return thread_param;
    }

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    int ret = 0;
    struct thread_data *thread_param = (struct thread_data *)malloc(sizeof(struct thread_data));

    thread_param->mutex = mutex;
    thread_param->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_param->wait_to_release_ms = wait_to_release_ms;

    ret = pthread_create(thread, NULL, threadfunc, (void *)thread_param);
    if (ret != 0) {
        printf("pthread_create failed with rc:%d\n", ret);
        return false;
    }
    printf("pthread_create successful with thread_id:%lu\n", *thread);

    return true;
}

