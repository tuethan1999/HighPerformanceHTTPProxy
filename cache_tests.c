#include <stdio.h>
#include <stdlib.h>
#include "HttpCache.h"
#include "uthash.h"

int main(int argc, char *argv[])
{
        printf("%s\n", argv[0]);
        Cache_T cache = new_cache();
        printf("%d\n", argc);
        Cache_T s = NULL;
        char *names[] = { "joe", "bob", "betty", NULL };
        printf("%s\n", "0");
        for (int i = 0; names[i]; ++i) {
                CacheObj_T cache_obj = new_cache_object();
                printf("%s\n", "1");
                cache_obj->url = names[i];
                printf("%s\n", "2");
                cache_obj->request_length = i;
                printf("%s\n", "3");
                insert_into_cache(cache, obj); 
                printf("%s\n", "4");
        }

        HASH_FIND_STR( cache, "betty", s);
        if (s) printf("betty's id is %d\n", s->cache_obj->request_length);

        /* free the hash table contents */
        // HASH_ITER(hh, users, s, tmp) {
        //         HASH_DEL(users, s);
        //         free(s);
        // }
        return 0;
}