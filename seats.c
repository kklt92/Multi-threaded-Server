#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "seats.h"
#include "thread_pool.h"


seat_t* seat_header = NULL;

/* unuesd seat list. */
static struct stdlist unused_list;

/* request buffer. */
static struct stdlist *req_buffer = NULL;

char seat_state_to_char(seat_state_t);

static int standby_size = 0;

static pthread_mutex_t stdlist_lock;

static void list_append(struct stdlist *item, struct stdlist *header) {
  item->prev = header->prev;
  item->next = header;
  header->prev = item;
  item->prev->next = item;
}

static void list_remove(struct stdlist *item) {
  item->prev->next = item->next;
  item->next->prev = item->prev;
}

static struct stdlist *get_unused_item() {
  struct stdlist *item = unused_list.next;
  if(item != &unused_list) {
    list_remove(item);
  }else {
    item = NULL;
  }
  return item;
}

static void put_unused_item(struct stdlist *item) {
  list_append(item, &unused_list);
}

/**
 * list all seat 
 */
void list_seats(char* buf, int bufsize)
{
  seat_t* curr = seat_header;
  int index = 0;
  
  /* lock seat linked list */
  pthread_mutex_lock(&seat_lock);
  while(curr != NULL && index < bufsize+ strlen("%d %c,"))
  {
    int length = snprintf(buf+index, bufsize-index, 
        "%d %c,", curr->id, seat_state_to_char(curr->state));
    if (length > 0)
      index = index + length;
    curr = curr->next;
  }
  /* unlock seat linked list */
  pthread_mutex_unlock(&seat_lock);
  if (index > 0)
    snprintf(buf+index-1, bufsize-index-1, "\n");
  else
    snprintf(buf, bufsize, "No seats not found\n\n");
}

void view_seat(char* buf, int bufsize,  int seat_id, int customer_id, int customer_priority)
{
  seat_t* curr = seat_header;
  while(curr != NULL)
  {
    if(curr->id == seat_id)
    {
      /* lock seat linked list */
      pthread_mutex_lock(&seat_lock);
      if(curr->state == AVAILABLE || (curr->state == PENDING && curr->customer_id == customer_id))
      {
        snprintf(buf, bufsize, "Confirm seat: %d %c ?\n\n",
            curr->id, seat_state_to_char(curr->state));
        curr->state = PENDING;
        curr->customer_id = customer_id;
      }
      else
      {
        /* lock standby list */
        pthread_mutex_lock(&stdlist_lock);
        if(standby_size && curr->state == PENDING) {
          struct stdlist *item = get_unused_item();
          item->user_id = customer_id;
          list_append(item, &curr->stdlist_pending);
          standby_size--;

          snprintf(buf, bufsize, "Seat unavailable\n\n");
        }else {

          snprintf(buf, bufsize, "Seat unavailable\n\n");
        }
        /* unlock standby list */
        pthread_mutex_unlock(&stdlist_lock);
      }
      /*unlock seat linked list */
      pthread_mutex_unlock(&seat_lock);

      return;
    }
    curr = curr->next;
  }
  snprintf(buf, bufsize, "Requested seat not found\n\n");
  return;
}

/**
 * confirm seat.
 */
void confirm_seat(char* buf, int bufsize, int seat_id, int customer_id, int customer_priority)
{
  seat_t* curr = seat_header;
  while(curr != NULL)
  {
    if(curr->id == seat_id)
    {
      /* lock seat linked list */
      pthread_mutex_lock(&seat_lock);
      if(curr->state == PENDING && curr->customer_id == customer_id )
      {
        snprintf(buf, bufsize, "Seat confirmed: %d %c\n\n",
            curr->id, seat_state_to_char(curr->state));
        curr->state = OCCUPIED;

        /* lock standby list */
        pthread_mutex_lock(&stdlist_lock);
        struct stdlist *item = curr->stdlist_pending.next;
        struct stdlist *next;
        while(item != &curr->stdlist_pending) {
          next = item->next;
          list_remove(item);
          put_unused_item(item);
          standby_size++;
          item = next;
        }
        /* unlock standby list */
        pthread_mutex_unlock(&stdlist_lock);
      }
      else if(curr->customer_id != customer_id )
      {
        snprintf(buf, bufsize, "Permission denied - seat held by another user\n\n");
      }
      else if(curr->state != PENDING)
      {
        snprintf(buf, bufsize, "No pending request\n\n");
      }
      /* unlock seat linked list */
      pthread_mutex_unlock(&seat_lock);

      return;
    }
    curr = curr->next;
  }
  snprintf(buf, bufsize, "Requested seat not found\n\n");

  return;
}

/**
 * cancel seat
 */
void cancel(char* buf, int bufsize, int seat_id, int customer_id, int customer_priority)
{
  printf("Cancelling seat %d for user %d\n", seat_id, customer_id);

  seat_t* curr = seat_header;
  while(curr != NULL)
  {
    if(curr->id == seat_id)
    {
      /* lock seat linked list */
      pthread_mutex_lock(&seat_lock);
      if(curr->state == PENDING && curr->customer_id == customer_id )
      {
        snprintf(buf, bufsize, "Seat request cancelled: %d %c\n\n",
            curr->id, seat_state_to_char(curr->state));
        
        /* lock standby list */
        pthread_mutex_lock(&stdlist_lock);
        struct stdlist *item = curr->stdlist_pending.next;
        if(item != &curr->stdlist_pending) {
          curr->customer_id = item->user_id;
          list_remove(item);
          put_unused_item(item);
          standby_size++;
        }else {
          curr->state = AVAILABLE;
        }
        /* unlock standby list */
        pthread_mutex_unlock(&stdlist_lock);

      }
      else if(curr->customer_id != customer_id )
      {
        snprintf(buf, bufsize, "Permission denied - seat held by another user\n\n");
      }
      else if(curr->state != PENDING)
      {
        snprintf(buf, bufsize, "No pending request\n\n");
      }
      /* unlock seat linked list */
      pthread_mutex_unlock(&seat_lock);

      return;
    }
    curr = curr->next;
  }
  snprintf(buf, bufsize, "Seat not found\n\n");

  return;
}

void load_seats(int number_of_seats, int sb_size)
{
  seat_t* curr = NULL;
  int i;
  for(i = 0; i < number_of_seats; i++)
  {   
    seat_t* temp = (seat_t*) malloc(sizeof(seat_t));
    temp->id = i;
    temp->customer_id = -1;
    temp->state = AVAILABLE;
    temp->next = NULL;

    temp->stdlist_pending.prev = &temp->stdlist_pending;
    temp->stdlist_pending.next = &temp->stdlist_pending;

    if (seat_header == NULL)
    {
      seat_header = temp;
    }
    else
    {
      curr-> next = temp;
    }
    curr = temp;
  }

  standby_size = sb_size;

  /* initial mutex */
  pthread_mutex_init(&stdlist_lock, NULL);

  unused_list.next = unused_list.prev = &unused_list;
  req_buffer = (struct stdlist*)malloc(sizeof(struct stdlist) *sb_size);
  for(i=0; i<sb_size; i++) {
    list_append(&req_buffer[i], &unused_list);
  }
}

void unload_seats()
{
  seat_t* curr = seat_header;
  while(curr != NULL)
  {
    seat_t* temp = curr;
    curr = curr->next;
    free(temp);
  }
  free(req_buffer);
}

char seat_state_to_char(seat_state_t state)
{
  switch(state)
  {
    case AVAILABLE:
      return 'A';
    case PENDING:
      return 'P';
    case OCCUPIED:
      return 'O';
  }

  return '?';
}
