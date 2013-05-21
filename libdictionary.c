/*******************************************************************************
 * @author		: Rohan Jyoti
 * @filename	: libdictionary.c
 * @purpose		: dictionary library
 ******************************************************************************/

#define _GNU_SOURCE //MUST HAVE THIS because it makes use of GNU only functions such as tdestroy which is not available on POSIX systems (including Mac OS X) in general
#include <search.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>

#include "libdictionary.h"

/*********************PRIVATE FUNCTIONS*******************/

static int compare(const void *a, const void *b)
{
	return strcmp(((const dictionary_entry_t *)a)->key, ((const dictionary_entry_t *)b)->key);
}

static dictionary_entry_t *malloc_entry_t(const char *key, const void *value)
{
	dictionary_entry_t *entry = malloc(sizeof(dictionary_entry_t));
	entry->key = key;
	entry->value = value;
	return entry;
}

static dictionary_entry_t *dictionary_tfind(dictionary_t *d, const char *key)
{
	dictionary_entry_t tentry = {key, NULL};
	void *tresult = tfind((void *)&tentry, &d->root, compare);
	
	if (tresult == NULL)
		return NULL;
	else
		return *((dictionary_entry_t **)tresult);
}

static void dictionary_tdelete(dictionary_t *d, const char *key)
{
	dictionary_entry_t tentry = {key, NULL};
	tdelete((void *)&tentry, &d->root, compare);
}

static int dictionary_remove_options(dictionary_t *d, const char *key, int free_memory)
{
	dictionary_entry_t *entry = dictionary_tfind(d, key);
	
	if (entry == NULL)
		return NO_KEY_EXISTS;
	else
	{
		dictionary_tdelete(d, key);
		
		if (free_memory)
		{
			free((void *)entry->key);
			free((void *)entry->value);
		}
		free(entry);
		
		return 0;
	}
}

static void destroy_no_element_free(void *ptr)
{
	free(ptr);
}

static void destroy_with_element_free(void *ptr)
{
	dictionary_entry_t *entry = (dictionary_entry_t *)ptr;
	
	free((void *)entry->key);
	free((void *)entry->value);
	free(entry);
}




/*********************PUBLIC FUNCTIONS*******************/

/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: dictionary_init
 * @param	: d is a Dictionary Data Structure
 * @return	: void
 * @purpose	: Initialize the dictionary data structure
 ******************************************************************************/
void dictionary_init(dictionary_t *d)
{
	d->root = NULL;
	pthread_mutex_init(&d->mutex, NULL);
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: dictionary_add
 * @param	: d is a Dictionary Data Structure; key is the key to be added;
 *				value is the value to be added
 * @return	: 0 on sucess, KEY_EXISTS if key exists in dictionary
 * @purpose	: Add a key-value pair to dictionary if key does not already exist
 *				in dictionary. This function is thread-safe.
 ******************************************************************************/
int dictionary_add(dictionary_t *d, const char *key, const void *value)
{
	pthread_mutex_lock(&d->mutex);
	
	if (dictionary_tfind(d, key) == NULL)
	{
		tsearch((void *)malloc_entry_t(key, value), &d->root, compare);
		
		pthread_mutex_unlock(&d->mutex);
		return 0;
	}
	else
	{
		pthread_mutex_unlock(&d->mutex);
		return KEY_EXISTS;
	}
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: dictionary_get
 * @param	: d is a Dictionary Data Structure; key is the key to retrieve
 * @return	: The value associated with key (if exists); else NULL
 * @purpose	: Retrieve value associated with key stored in dictionary.
 *				This function is thread-safe.
 ******************************************************************************/
const void *dictionary_get(dictionary_t *d, const char *key)
{
	pthread_mutex_lock(&d->mutex);
	dictionary_entry_t *entry = dictionary_tfind(d, key);
	
	if (entry == NULL)
	{
		pthread_mutex_unlock(&d->mutex);
		return NULL;
	}
	else
	{
		pthread_mutex_unlock(&d->mutex);
		return entry->value;
	}
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: dictionary_remove
 * @param	: d is a Dictionary Data Structure; key is the key to remove
 * @return	: 0 on sucess; NO_KEY_EXISTS if key is not in dictionary
 * @purpose	: Remove key-value pair stored in dictionary
 *				This function is thread-safe.
 ******************************************************************************/
int dictionary_remove(dictionary_t *d, const char *key)
{
	pthread_mutex_lock(&d->mutex);
	int val = dictionary_remove_options(d, key, 0);
	pthread_mutex_unlock(&d->mutex);
	
	return val;
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: dictionary_remove_free
 * @param	: d is a Dictionary Data Structure; key is the key to remove
 * @return	: 0 on sucess; NO_KEY_EXISTS if key is not in dictionary
 * @purpose	: Remove and frees assoc. memory key-value pair stored in dictionary
 *				This function is thread-safe.
 ******************************************************************************/
int dictionary_remove_free(dictionary_t *d, const char *key)
{
	pthread_mutex_lock(&d->mutex);
	int val = dictionary_remove_options(d, key, 1);
	pthread_mutex_unlock(&d->mutex);
	
	return val;
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: dictionary_destroy
 * @param	: d is a Dictionary Data Structure
 * @return	: void
 * @purpose	: Frees only memory associated with internal data structure
 ******************************************************************************/
void dictionary_destroy(dictionary_t *d)
{
	//must comment out when using on mac
	printf("WARNING MEMORY LEAK!!! UNCOMMENT IN DICTIONARYLIB\n");
	//tdestroy(d->root, destroy_no_element_free);
	d->root = NULL;
	
	pthread_mutex_destroy(&d->mutex);
}


/*******************************************************************************
 * @author	: Rohan Jyoti
 * @name	: dictionary_destroy_free
 * @param	: d is a Dictionary Data Structure
 * @return	: void
 * @purpose	: Frees both memory associated with internal data structure and
 *				memory associated with the content
 ******************************************************************************/
void dictionary_destroy_free(dictionary_t *d)
{
	//must comment out when using on mac
	//tdestroy(d->root, destroy_with_element_free);
	d->root = NULL;
	
	pthread_mutex_destroy(&d->mutex);
}

