/* 
 * Nate Krinsky
 * High Performance HTTP Proxy
 *
 * Bucket.h file
 *
 * Purpose: This module implements the Leaky/Token
 * Bucket algorithm to limit and regulate network
 * traffic
 */

#include "Bucket.h"
#include "Buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

Bucket_ptr new_bucket(int bucket_size, int token_rate) {
		Bucket_ptr bucket = malloc(sizeof(*bucket));
		bucket->bucket_size = bucket_size;
		//bucket->tokens = bucket_size; // bucket starts out as full
		bucket->tokens = 0; // TODO delete
		bucket->token_rate = token_rate;
		gettimeofday(&bucket->last_updated, NULL);
		//bucket->last_updated = time(0);
		return bucket;
}
