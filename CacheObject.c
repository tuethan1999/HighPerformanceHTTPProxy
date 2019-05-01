#include "CacheObject.h"
#include <assert.h>
#include <stdio.h>

#ifndef FIELD_NAME_LENGTH
        #define FIELD_NAME_LENGTH 20
#endif
#ifndef CONTENT_LENGTH
        #define CONTENT_LENGTH 20
#endif

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
        cache_obj->req_header = NULL;
        cache_obj->res_header = NULL;
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

int is_expired(CacheObj_T cache_obj){
        print_cache_object(cache_obj);
        fprintf(stderr, "age: %d, max_age: %d\n", (int)(time(NULL) - cache_obj->last_updated), (int)cache_obj->res_header->max_age);
        if ((time(NULL) - cache_obj->last_updated) >= cache_obj->res_header->max_age)
                return 1;
        return 0;
}

void print_cache_object(CacheObj_T cache_obj)
{
        printf("--------------------------------\n");
        printf("%*s: %*s\n", FIELD_NAME_LENGTH, "Url", CONTENT_LENGTH, cache_obj->url);
        printf("%*s: %*d\n", FIELD_NAME_LENGTH, "Last Requested", CONTENT_LENGTH, (int)cache_obj->last_requested);
        printf("%*s: %*d\n", FIELD_NAME_LENGTH, "Last Updated", CONTENT_LENGTH, (int)cache_obj->last_updated);
        printf("%*s: %*d\n", FIELD_NAME_LENGTH, "Request Length", CONTENT_LENGTH, cache_obj->request_length);
        printf("%*s: %*d\n", FIELD_NAME_LENGTH, "Response Length", CONTENT_LENGTH, cache_obj->response_length);
        if(cache_obj->res_header) print_http_res_head(cache_obj->res_header);
        if(cache_obj->req_header) print_http_req_head(cache_obj->req_header);
        int* p;
        printf("clientfds: ");
        for(p=(int*)utarray_front(cache_obj->client_fds);
                p!=NULL;
                p=(int*)utarray_next(cache_obj->client_fds, p)) {
                printf("|%d|",* p);
        }
        printf("\n");

        printf("--------------------------------\n");
}

void free_cache_object(CacheObj_T cache_obj)
{
        utarray_free(cache_obj->client_fds);
        free(cache_obj->url);
        free(cache_obj->request_buffer);
        free(cache_obj->response_buffer);
        free_req_head(cache_obj->req_header);
        free_res_head(cache_obj->res_header);
        free(cache_obj);

}
