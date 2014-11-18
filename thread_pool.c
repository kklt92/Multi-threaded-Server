#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

#include "thread_pool.h"
#include "semaphore.h"

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

#define PRIORITY_LEVEL 3

pthread_mutex_t seat_lock;

/**
 * single task information. 
 * including `function`, and `argument`.
 */
typedef struct pool_task{
  void (*function)(void *);
  int argument;
  struct pool_task *next;
  struct pool_task *prev;
} pool_task_t;

/**
 * thread pool 
 */
struct pool_t {
  m_sem_t *s;
  pthread_mutex_t lock;
  pthread_t *threads;
  pool_task_t queue[PRIORITY_LEVEL];
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
  node->prev->next = node->next;
  if(head->prev == node) {
    head->prev = node->prev;
  }
  else {
    node->next->prev = node->prev;
  }

}




/*
 * Create a threadpool, initialize variables, etc
 *
 */
pool_t *pool_create(int queue_size, int num_threads)
{
  pthread_attr_t attr;

  int i;
  
  if(queue_size <= 0)
    queue_size = 50000;
  if(num_threads <= 0)
    num_threads = MAX_THREADS;

  struct pool_t *pool = (struct pool_t*)malloc(sizeof(struct pool_t));
  pool->threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
  for(i=0; i<PRIORITY_LEVEL; i++) {
    pool->queue[i].function = NULL;
    pool->queue[i].argument = 0;
    pool->queue[i].next = NULL;
    pool->queue[i].prev = &(pool->queue[i]);
  }

  /* initial semaphore and lock. */
  pool->s = (struct m_sem*)malloc(sizeof(struct m_sem));
  pthread_mutex_init(&pool->lock, NULL);

  sem_init(pool->s, 0, 1);

  pthread_mutex_init(&seat_lock, NULL);

  pool->task_queue_size_limit = queue_size;
  pool->thread_count = num_threads;

  pthread_attr_init(&attr);

  /* loop to create pthread */
  for(i=0; i<pool->thread_count; i++) {
    pthread_create(pool->threads+i, &attr, thread_do_work, (void*)pool);
  }


  return pool;
}


/*
 * Add a task to the threadpool
 *
 */
int pool_add_task(pool_t *pool, void (*function)(void *), void *argument, int priority)
{
  int err = 0;
  pool_task_t *curr;

  curr = (pool_task_t*)malloc(sizeof(pool_task_t));
//  curr->argument = malloc(sizeof(int));

  curr->function = function;

  curr->argument = *(int*)argument;


  /* lock pool->queue */
  pthread_mutex_lock(&pool->lock);
  append_list(curr, &pool->queue[priority]);

  /* unlock semaphore */
  sem_post(((struct pool_t*)pool)->s);
  /* unlock pool->queue */
  pthread_mutex_unlock(&pool->lock);


  return err;
}



/*
 * Destroy the threadpool, free all memory, destroy treads, etc
 *
 */
int pool_destroy(pool_t *pool)
{
  int err = 0;

  int i;
  
  printf("Running destroy\n");

  /* force cancel all thread. */
  for(i=0; i<pool->thread_count; i++) {
    pthread_cancel(*(pool->threads+i));
  }
  pthread_mutex_destroy(&pool->lock);

  free(pool->s);
  free(pool);

  pthread_mutex_destroy(&seat_lock);
  

  return err;
}



/*
 * Work loop for threads. Should be passed into the pthread_create() method.
 *
 */
static void *thread_do_work(void *pool)
{ 
  pool_task_t *currT;
  int i;

  currT = NULL;
  
  /* loop to read task from queue. */
  while(1) {
    sem_wait(((struct pool_t*)pool)->s);

    pthread_mutex_lock(&((struct pool_t*)pool)->lock);
    currT = NULL;
    /* read from high priority */
    for(i=0; i<PRIORITY_LEVEL; i++) {
      if(((struct pool_t*)pool)->queue[i].next != NULL) {
        currT = ((struct pool_t*)pool)->queue[i].next;
        remove_list_node(currT, &((struct pool_t*)pool)->queue[i]);
        break;
      }
    }
    pthread_mutex_unlock(&((struct pool_t*)pool)->lock);

    sem_post(((struct pool_t*)pool)->s);
    
    /* run function and free it immediately. */
    if(currT != NULL) {
      (currT->function)(&(currT->argument));
      free(currT);
    }

  }

  pthread_exit(NULL);
  return(NULL);
}
