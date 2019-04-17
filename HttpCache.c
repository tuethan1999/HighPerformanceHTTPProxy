#include "HttpCache.h"
#include "uthash.h"
#include <assert.h>

struct Cache_T{
        CacheObj_T cache_obj;
        UT_hash_handle hh; /* makes this structure hashable */
};

void delete_cache_T(Cache_T cache, Cache_T c){
        HASH_DEL(cache, c);
        free_cache_object(c->cache_obj);
        free(c);
}

Cache_T new_cache()
{
        Cache_T cache = NULL;
        return cache;
}

void insert_into_cache(Cache_T cache, CacheObj_T obj)
{
        Cache_T c = malloc(sizeof(*c));
        c->cache_obj = obj;
        HASH_ADD_KEYPTR(hh, cache, c->cache_obj->url, strlen(c->cache_obj->url), c);
}

CacheObj_T find_by_url(Cache_T cache, char* url)
{
        Cache_T c = NULL;
        HASH_FIND_STR(cache, url, c);
        if(c) return c->cache_obj;
        return NULL;
}


void delete_expired(Cache_T cache, int max_age)
{
        Cache_T c = NULL;
        Cache_T tmp = NULL;
        int age;
        int now = time(NULL);
        HASH_ITER(hh, cache, c, tmp){
                age = now - c->cache_obj->last_updated;
                if(age > max_age)
                        delete_cache_T(cache, c);
        }
}

void delete_by_sockfd(Cache_T cache, int sockfd)
{
        Cache_T c = NULL;
        Cache_T tmp = NULL;
        HASH_ITER(hh, cache, c, tmp){
                delete_from_clientfds(c->cache_obj, sockfd);
        }
}

void free_cache(Cache_T cache)
{
        Cache_T c = NULL;
        Cache_T tmp = NULL;
        HASH_ITER(hh, cache, c, tmp){
                delete_cache_T(cache, c);
        }
}
