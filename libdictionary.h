/*******************************************************************************
 * @author		: Rohan Jyoti
 * @filename	: libdictionary.h
 * @purpose		: dictionary library header
 ******************************************************************************/

#ifndef _LIBDICTIONRY_H_
#define _LIBDICTIONRY_H_
#include <pthread.h>
#define KEY_EXISTS 1
#define NO_KEY_EXISTS 2

#define _GNU_SOURCE

typedef struct _dictionary_entry_t
{
	const char *key;
	const void *value;
} dictionary_entry_t;

typedef struct _dictionary_t
{
	void *root;
	pthread_mutex_t mutex;
} dictionary_t;


void dictionary_init(dictionary_t *d);
int dictionary_add(dictionary_t *d, const char *key, const void *value);
const void *dictionary_get(dictionary_t *d, const char *key);
int dictionary_remove(dictionary_t *d, const char *key);
int dictionary_remove_free(dictionary_t *d, const char *key);
void dictionary_destroy(dictionary_t *d);
void dictionary_destroy_free(dictionary_t *d);


#endif

