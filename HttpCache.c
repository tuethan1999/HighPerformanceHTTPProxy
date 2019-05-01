#include "HttpCache.h"
#include <assert.h>
#include <stdio.h>

#define INITIAL_CACHE_SIZE 1
/*struct Cache_T{
        CacheObj_T *arr;
        int num_obj;
        int capacity;
};*/

Cache_T new_cache()
{
        Cache_T cache = malloc(sizeof(*cache));
        assert(cache != NULL);
        cache->arr = malloc(INITIAL_CACHE_SIZE * sizeof(CacheObj_T));
        memset(cache->arr, 0, INITIAL_CACHE_SIZE * sizeof(CacheObj_T));
        assert(cache->arr != NULL);
        cache->num_obj = 0;
        cache->capacity = INITIAL_CACHE_SIZE;
        return cache;
}

void insert_into_cache(Cache_T cache, CacheObj_T obj)
{
        if(cache->num_obj >= cache->capacity){
                int new_size = 2*cache->capacity;
                int new_size_bytes = new_size * sizeof(CacheObj_T);
                int old_size_bytes = cache->capacity * sizeof(CacheObj_T);
                fprintf(stderr, "Cache full, expanding buffer list size from %d -> %d\n", cache->capacity, new_size);
                CacheObj_T *new_list = malloc(new_size_bytes);
                memset(new_list, 0, new_size_bytes);
                memcpy(new_list, cache->arr, old_size_bytes);
                free(cache->arr);
                cache->arr = new_list;
                cache->capacity = new_size;
        }
        for(int i = 0; i < cache->capacity; i++){
                if (cache->arr[i] == NULL){
                        cache->arr[i] = obj;
                        cache->num_obj+=1;
                        break;
                }
        }
}

CacheObj_T find_by_url(Cache_T cache, char* url)
{
        fprintf(stderr, "looking for %s in cache\n", url);
        for(int i = 0; i < cache->capacity; i++){
                if(cache->arr[i] == NULL)
                        continue;
                if(strcmp(cache->arr[i]->url, url) == 0){
                        printf("Found in cache\n");
                        return cache->arr[i];
                }
        }
        printf("Could not be found in cache\n");
        return NULL;
}

void print_cache(Cache_T cache)
{
        fprintf(stderr, "%s\n", "******************************PRINTING CACHE******************************");
        for(int i = 0; i < cache->capacity; i++){
                if(cache->arr[i] == NULL)
                        continue;
                print_cache_object(cache->arr[i]);
        }
        fprintf(stderr, "%s\n", "***************************************************************************");
}


void delete_expired(Cache_T cache)
{
        //fprintf(stderr, "%s\n", "checking expired");
        for(int i = 0; i < cache->capacity; i++){
                CacheObj_T cache_obj = cache->arr[i];
                if(cache_obj == NULL || cache_obj->res_header == NULL || cache_obj->last_updated == -1)
                        continue;
                if(is_expired(cache_obj)){
                        fprintf(stderr, "deleting %s from cache\n", cache_obj->url);
                        free_cache_object(cache_obj);
                        cache->arr[i] = NULL;
                }
        }
}

void delete_by_sockfd(Cache_T cache, int sockfd)
{
        for(int i = 0; i < cache->capacity; i++){
                CacheObj_T cache_obj = cache->arr[i];
                if(cache_obj == NULL)
                        continue;
                delete_from_clientfds(cache_obj, sockfd);
        }
}       

void free_cache(Cache_T cache)
{
        for(int i = 0; i < cache->capacity; i++){
                if(cache->arr[i] == NULL)
                        continue;
                free_cache_object(cache->arr[i]);
        }
        free(cache);
}
