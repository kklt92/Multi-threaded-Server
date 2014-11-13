#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "thread_pool.h"
#include "semaphore.c"

/**
 *  @struct threadpool_task
 *  @brief the work struct
 *
 *  Feel free to make any modifications you want to the function prototypes and structs
 *
 *  @var function Pointer to the function that will perform the task.
 *  @var argument Argument to be passed to the function.
 */

#define MAX_THREADS 20
#define STANDBY_SIZE 8

typedef struct pool_task{
    void (*function)(void *);
    void *argument;
    struct pool_task *next;
    struct pool_task *prev;
} pool_task_t;


struct pool_t {
  pthread_mutex_t lock;
  pthread_cond_t notify;
  m_sem_t s;
  pthread_t *threads;
  pool_task_t *queue;
  int thread_count;
  int task_queue_size_limit;
};

static void *thread_do_work(void *pool);


void insert_list(pool_task_t *newNode, pool_task_t *head) {
  newNode->next = head->next;
  newNode->prev = head;
  head->next = newNode;

}

void append_list(pool_task_t *newNode, pool_task_t *head) {
  head->prev->next = newNode;
  newNode->prev = head->prev;
  newNode->next = NULL;
  head->prev = newNode;

}

void remove_list_node(pool_task_t *node, pool_task_t *head) {
}


/*
 * Create a threadpool, initialize variables, etc
 *
 */
pool_t *pool_create(int queue_size, int num_threads)
{
  pthread_t tid;
  pthread_attr_t attr;

  int i;

  struct pool_t *pool = (struct pool_t*)malloc(sizeof(struct pool_t));
  pool->threads = (pthread_t*)malloc(MAX_THREADS * sizeof(pthread_t));
  pool->queue = (pool_task_t*)malloc(sizeof(pool_task_t));
  pool->queue->function = NULL;
  pool->queue->argument = NULL;
  pool->queue->next = NULL;
  pool->queue->prev = NULL;

  pool->task_queue_size_limit = queue_size;
  pool->thread_count = num_threads;

  pthread_attr_init(&attr);
  
  for(i=0; i<MAX_THREADS; i++) {
    pthread_create(pool->threads+i, &attr, thread_do_work, NULL);
  }


  return NULL;
}


/*
 * Add a task to the threadpool
 *
 */
int pool_add_task(pool_t *pool, void (*function)(void *), void *argument)
{
    int err = 0;
    pool_task_t *curr;

    curr = pool->queue;
    while(curr != NULL) {
      pool->queue->next;
        
    return err;
}



/*
 * Destroy the threadpool, free all memory, destroy treads, etc
 *
 */
int pool_destroy(pool_t *pool)
{
    int err = 0;
 
    return err;
}



/*
 * Work loop for threads. Should be passed into the pthread_create() method.
 *
 */
static void *thread_do_work(void *pool)
{ 

    while(1) {
        
    }

    pthread_exit(NULL);
    return(NULL);
}
