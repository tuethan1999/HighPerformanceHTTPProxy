#include "CacheObject.h"
#include <assert.h>

CacheObj_T new_cache_object()
{
        CacheObj_T cache_obj = malloc(sizeof(*cache_obj));
        assert(cache_obj != NULL);
        cache_obj->url = NULL;
        cache_obj->request_buffer = NULL;
        cache_obj->request_length = -1; 
        cache_obj->last_requested = -1;
        cache_obj->response_buffer = NULL;
        cache_obj->response_length = -1;
        cache_obj->last_updated = -1;
        utarray_new(cache_obj->client_fds, &ut_int_icd);
        return cache_obj;
}

void delete_from_clientfds(CacheObj_T cache_obj, int sockfd)
{
        unsigned i = 0;
        UT_array *darr = cache_obj->client_fds;
        for(int *p=(int*)utarray_front(darr); p!=NULL; p=(int*)utarray_next(darr, p)){
                if(*p == sockfd){
                        utarray_erase(darr, i, 1);
                        return;
                }
                i++;
        }
}

void free_cache_object(CacheObj_T cache_obj)
{
        utarray_free(cache_obj->client_fds);
        free(cache_obj->url);
        free(cache_obj->request_buffer);
        free(cache_obj->response_buffer);
        free(cache_obj);
}
