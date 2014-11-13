#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>


typedef struct m_sem_t {
  pthread_mutex_t lock;
  pthread_cond_t notify;
  int value;
  struct process *list;
} m_sem_t;

int sem_wait(m_sem_t *s);
int sem_post(m_sem_t *s);
void sem_init(m_sem_t *s, int expected, int new_value);

void sem_init(m_sem_t *s, int expected, int new_value) {
  s->value = new_value;

}

int sem_wait(m_sem_t *s)
{
 
  return 0;
}

int sem_post(m_sem_t *s)
{
  //TODO
  return 0;
}
