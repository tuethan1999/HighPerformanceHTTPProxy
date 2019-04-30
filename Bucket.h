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

#include <time.h>
#include <sys/time.h>

#ifndef BUCKET_H_INCLUDED
#define BUCKET_H_INCLUDED

typedef struct Bucket {
		int tokens;
		int bucket_size;
		int token_rate;
		struct timeval last_updated;
}*Bucket_ptr;

/*
 * Function: newBucket
 * --------------------
 * Allocates space for a Bucket and returns it
 *
 * bucket_size: max number of tokens the bucket can hold
 * token_rate: the rate at which new tokens are added
 * 
 * returns: Bucket_ptr
 */
Bucket_ptr new_bucket(int bucket_size, int token_rate);

/*
 * Function: add_token
 * --------------------
 * Checks all Buckets to see if it's time to add a token
 * If it is, add token and update last_updated
 *
 * length: number of buckets (equal to length of partial buffer array)
 * buffer_list: array of partial message buffers
 * 
 * returns: None
 */
//void add_token(bufferList buffer_list, int length);

/*
 * Function:  
 * --------------------
 * 
 * 
 *  returns: 
 */


/*
 * Function:  
 * --------------------
 * 
 * 
 *  returns: 
 */

#endif
