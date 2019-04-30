/* 
 * Nate Krinsky and Ethan Tu
 * High Performance HTTP Proxy
 *
 * Buffer.h file
 *
 * Purpose: This module stores partial messages
 * in a buffer so complete packets can be handled
 */

#ifndef BUFFER_H_INCLUDED
#define BUFFER_H_INCLUDED

#include "Bucket.h"

#define BUFSIZE 2*4096
#define BUFLISTSIZE 1
#define BUCKET_SIZE 2*BUFSIZE
#define TOKEN_RATE 50000

typedef struct partialBuffer {
        char *buffer;
        int length;
        int size;
        Bucket_ptr bucket;
}*partialBuffer_ptr;

typedef struct bufferListStruct{
        partialBuffer_ptr *buffers;
        int size;
}*bufferList;

void server_error(char *msg);

partialBuffer_ptr newPartialBuffer();
void insertPartialBuffer(partialBuffer_ptr partial_buffer, char* msg, int length);
void printPartialBuffer(partialBuffer_ptr partial_buffer);
void deletePartialBuffer(partialBuffer_ptr partial_buffer);

bufferList newBufferList();
void insertBufferList(bufferList buffer_list, partialBuffer_ptr partial_buffer, int index);
void deleteFromBufferList(bufferList buffer_list, int index);
void clearFromBufferList(bufferList buffer_list, int index, int length);
void printBufferList(bufferList buffer_list);

#endif
