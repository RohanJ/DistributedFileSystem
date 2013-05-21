/*******************************************************************************
 * @author		: Rohan Jyoti
 * @filename	: queue.h
 * @purpose		: Queue Library Header
 ******************************************************************************/
#ifndef __QUEUE_H__
#define __QUEUE_H__

#define _GNU_SOURCE

typedef struct {
	struct queue_node *head;
	struct queue_node *tail;
	unsigned int size; 
} queue_t;

void queue_init(queue_t *q);
void queue_destroy(queue_t *q);

void *queue_dequeue(queue_t *q);
void *queue_at(queue_t *q, int pos);
void *queue_remove_at(queue_t *q, int pos);
void queue_enqueue(queue_t *q, void *item);
unsigned int queue_size(queue_t *q);

void queue_iterate(queue_t *q, void (*iter_func)(void *, void *), void *arg);

#endif
