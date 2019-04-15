/* 
 * Ethan Tu
 * High Performance HTTP Proxy
 *
 * HttpCache.h file
 *
 * Purpose: this module provides functionality 
 * to store and query and delete
 * cache objects
 */

#ifndef HTTP_CACHE_H_INCLUDED
#define HTTP_CACHE_H_INCLUDED
#include <stdlib.h>
#include "utarray.h"
#include <time.h>

typedef struct CacheObj_T{
        char* url;                     /*must be null_terminated*/
        
        char *request_buffer;
        int request_length; 
        time_t last_requested;

                                        /*response variables*/
        char *response_buffer;
        int response_length;
        time_t last_accessed;

        UT_array *client_fds;           /*dynamic array of sockfds who want this URL*/
}*CacheObj_T;

typedef struct Cache_T *Cache_T;


/*
 * Function:  new_res_head 
 * --------------------
 * Allocates space for a Cache_T and returns it
 * 
 *  returns: Cache_T
 */
Cache_T new_cache();

/*
 * Function:  insert_into_cache 
 * --------------------
 * Inserts a CacheObj_T into the cache
 *
 *  cache: Cache_T to put the CacheObj_T into
 *
 *  returns: None
 */
void insert_into_cache(Cache_T cache, CacheObj_T obj);

/*
 * Function:  find_by_url
 * --------------------
 * Queries Cache_T by url string and returns CacheObj_T
 *
 *  cache: Cache_T to query
 *  url: string to query for
 *
 *  returns: CacheObj_T if url is found, else NULL
 */
CacheObj_T find_by_url(Cache_T cache, char* url);

/*
 * Function:  delete_expired
 * --------------------
 * Iterates through the Cache_T and checks if the time passed since
 * last_acessed is greater than max_age and deletes it if so. 
 *
 *  cache: Cache_T to iterate through
 *  max_age: if max age > current time - last_accessed then the CacheObj_T
 *  is expired
 *
 *  returns: None
 */
void delete_expired(Cache_T cache, int max_age);

/*
 * Function:  delete_by_sockfd
 * --------------------
 * Queries Cache_T by sockfd and deletes sockfd from each Cache 
 * Object's client_fds list
 *
 *  cache: Cache_T to query
 *  sockfd: file descriptor to query for
 *
 *  returns: None
 */
void delete_by_sockfd(Cache_T cache, int sockfd);

/*
 * Function:  free_cache
 * --------------------
 * Frees memory allocated for Cache_T
 *
 *  cache: Cache_T to free
 *
 *  returns: None
 */
void free_cache(Cache_T cache);

#endif