#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "semaphore.h"


//int sem_wait(m_sem_t *s);
//int sem_post(m_sem_t *s);
//void sem_init(m_sem_t *s, int expected, int new_value);

/**
 * initial semaphore. get the semaphore->value 
 */
void sem_init(m_sem_t *s, int expected, int new_value) {
  s->value = new_value;
  pthread_mutex_init(&(s->lock), NULL);
  pthread_cond_init(&(s->notify), NULL);

}

/**
 * semaphore wait. stop and wait for semaphore->notify.
 */
int sem_wait(m_sem_t *s)
{
  pthread_mutex_lock(&(s->lock));

  while(s->value <= 0) {
    pthread_cond_wait(&(s->notify), &(s->lock));
  }

  
  s->value--;
  
  pthread_mutex_unlock(&(s->lock));

  return 0;
}

/**
 * semaphore post. continue and unlock semaphore->notify.
 */
int sem_post(m_sem_t *s)
{
  pthread_mutex_lock(&(s->lock));

  s->value++;

  pthread_cond_signal(&(s->notify));
  printf("hehe => %d\n", s->value);

  pthread_mutex_unlock(&(s->lock));
  return 0;
}
