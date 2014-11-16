#include <pthread.h>

typedef struct m_sem {
  pthread_mutex_t lock;
  pthread_cond_t notify;
  int value;
  struct process *list; 
}m_sem_t;

void sem_init(m_sem_t*, int, int);

int sem_wait(m_sem_t*);

int sem_post(m_sem_t*);
