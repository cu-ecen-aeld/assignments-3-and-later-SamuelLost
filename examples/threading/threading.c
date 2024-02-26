#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
// #define DEBUG_LOG(msg,...) 
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    int mutex_result = 0;
    pthread_mutex_t *mutex = thread_func_args->mutex;

    DEBUG_LOG("[threadfunc] Data: wait_to_obtain_ms: %d, wait_to_release_ms: %d, mutex: %p", thread_func_args->wait_to_obtain_ms, thread_func_args->wait_to_release_ms, thread_func_args->mutex);
    
    int sleep_result = usleep(thread_func_args->wait_to_obtain_ms * 1000);
    if(sleep_result != 0) {
        ERROR_LOG("[threadfunc] Failed to sleep for %d ms", thread_func_args->wait_to_obtain_ms);
        thread_func_args->thread_complete_success = false;
        return thread_param;
    }

    DEBUG_LOG("[threadfunc] Attempting to obtain mutex");
    mutex_result = pthread_mutex_lock(mutex);
    if(mutex_result != 0) {
        ERROR_LOG("[threadfunc] Failed to obtain mutex");
        thread_func_args->thread_complete_success = false;
        return thread_param;
    }

    DEBUG_LOG("[threadfunc] Obtained mutex, waiting for %d ms", thread_func_args->wait_to_release_ms);
    sleep_result = usleep(thread_func_args->wait_to_release_ms * 1000);
    if(sleep_result != 0) {
        ERROR_LOG("[threadfunc] Failed to second sleep for %d ms", thread_func_args->wait_to_release_ms);
        thread_func_args->thread_complete_success = false;
        return thread_param;
    }

    DEBUG_LOG("[threadfunc] Releasing mutex");
    mutex_result = pthread_mutex_unlock(mutex);
    if(mutex_result != 0) {
        ERROR_LOG("[threadfunc] Failed to release mutex");
        thread_func_args->thread_complete_success = false;
        return thread_param;
    }

    DEBUG_LOG("[threadfunc] Thread complete");
    thread_func_args->thread_complete_success = true;

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

    DEBUG_LOG("[start_thread_obtaining_mutex] Creating thread with wait_to_obtain_ms: %d, wait_to_release_ms: %d, mutex: %p", wait_to_obtain_ms, wait_to_release_ms, mutex);
    struct thread_data *thread_data = malloc(sizeof(struct thread_data));
    if(thread_data == NULL) {
        ERROR_LOG("[start_thread_obtaining_mutex] Failed to allocate memory for thread_data");
        return false;
    }

    thread_data->mutex = mutex;
    thread_data->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_data->wait_to_release_ms = wait_to_release_ms;
    thread_data->thread_complete_success = false;

    DEBUG_LOG("[start_thread_obtaining_mutex] Creating thread");
    DEBUG_LOG("[start_thread_obtaining_mutex] Thread data: wait_to_obtain_ms: %d, wait_to_release_ms: %d, mutex: %p", thread_data->wait_to_obtain_ms, thread_data->wait_to_release_ms, thread_data->mutex);

    int thread_result = pthread_create(thread, NULL, threadfunc, (void *) thread_data);
    if(thread_result != 0) {
        ERROR_LOG("[start_thread_obtaining_mutex] Failed to create thread");
        free(thread_data);
        return false;
    }

    DEBUG_LOG("[start_thread_obtaining_mutex] Thread created");

    return true;
}

