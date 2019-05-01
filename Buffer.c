/* 
 * Nate Krinsky and Ethan Tu
 * High Performance HTTP Proxy
 *
 * Buffer.h file
 *
 * Purpose: This module stores partial messages
 * in a buffer so complete packets can be handled
 */

#include "Buffer.h"
#include "Bucket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void server_error(char *msg) {
        perror(msg);
        exit(EXIT_FAILURE);
}

partialBuffer_ptr newPartialBuffer()
{
        partialBuffer_ptr buf = malloc(sizeof(*buf));
        if (buf == NULL) server_error("create partialBuffer_ptr");
        buf->buffer = malloc(BUFSIZE);
        memset(buf->buffer, 0, BUFSIZE);
        if (buf->buffer == NULL) server_error("create partialBuffer_ptr buffer");
        buf->size = BUFSIZE;
        buf->length = 0;
        buf->bucket = new_bucket(BUCKET_SIZE, TOKEN_RATE);
        return buf;
}

void insertPartialBuffer(partialBuffer_ptr partial_buffer, char* msg, int length)
{
        //printf("in insertPartialBuffer, current buffer length: %d\n", partial_buffer->length);
        if (length >= partial_buffer->size - partial_buffer->length) {
                int new_size = 2*partial_buffer->size;
                //fprintf(stderr, "length of msg is %d, expanding partial buffer size from %d -> %d\n", length, partial_buffer->size, new_size);
                partial_buffer->buffer = realloc(partial_buffer->buffer, new_size);
                partial_buffer->size = new_size;
        }
        int position = partial_buffer->length;
        memcpy(partial_buffer->buffer + position, msg, length);
        partial_buffer->length += length;
        //printf("added %d bytes, current length is %d\n", length, partial_buffer->length);
}

void printPartialBuffer(partialBuffer_ptr partial_buffer)
{
        if(partial_buffer == NULL || partial_buffer->buffer == NULL) return;
        if(partial_buffer->length == 0){
                fprintf(stderr, "--empty partial buffer--\n");
                return;
        }
        fprintf(stderr, "--------------------------------------\n");
        for(int i = 0; i < partial_buffer->length; i++){
                fprintf(stderr, "%c", partial_buffer->buffer[i]);
        }
        fprintf(stderr, "--------------------------------------\n");
}

void deletePartialBuffer(partialBuffer_ptr partial_buffer)
{
        printf("in deletePartialBuffer\n");
        if(partial_buffer == NULL) return;
        free(partial_buffer->buffer);
        printf("freed partial_buffer->buffer\n");
        free(partial_buffer->bucket);
        printf("freed partial_buffer->bucket\n");
        free(partial_buffer);
        printf("freed partial_buffer\n");
}

bufferList newBufferList()
{
        bufferList buffer_list = malloc(sizeof(*buffer_list));
        if (buffer_list == NULL) server_error("create bufferList");
        buffer_list->buffers = malloc(BUFLISTSIZE * sizeof(partialBuffer_ptr));
        memset(buffer_list->buffers, 0, BUFLISTSIZE * sizeof(partialBuffer_ptr));
        if (buffer_list->buffers == NULL) server_error("create bufferList buffers");
        buffer_list->size = BUFLISTSIZE;
        return buffer_list; 
}

void insertBufferList(bufferList buffer_list, partialBuffer_ptr partial_buffer, int index)
{
        while(index >= buffer_list->size){
                int new_size = 2*buffer_list->size;
                int new_size_bytes = new_size * sizeof(partialBuffer_ptr);
                int old_size_bytes = buffer_list->size * sizeof(partialBuffer_ptr);
                fprintf(stderr, "index is %d, expanding buffer list size from %d -> %d\n", index, buffer_list->size, new_size);
                partialBuffer_ptr *new_list = malloc(new_size_bytes);
                memset(new_list, 0, new_size_bytes);
                memcpy(new_list, buffer_list->buffers, old_size_bytes);
                free(buffer_list->buffers);
                buffer_list->buffers = new_list;
                buffer_list->size = new_size;
        }
        buffer_list->buffers[index] = partial_buffer;
}

void deleteFromBufferList(bufferList buffer_list, int index)
{
        fprintf(stderr, "deleting fd: %d from buffer list\n", index);
        partialBuffer_ptr partial_buffer =  buffer_list->buffers[index];
        deletePartialBuffer(partial_buffer);
        buffer_list->buffers[index] = NULL;
}

void clearFromBufferList(bufferList buffer_list, int index, int length) {
        fprintf(stderr, "clearing buffer of fd: %d\n", index);
        partialBuffer_ptr partial_buffer =  buffer_list->buffers[index];
        char *new_buffer = malloc(partial_buffer->size);
        bzero(new_buffer, partial_buffer->size);
        printf("size is %d, length is %d\n", partial_buffer->size, partial_buffer->length);
        int bytes_to_copy = partial_buffer->size - partial_buffer->length;
        memcpy(new_buffer, partial_buffer->buffer+length, bytes_to_copy);

        free(partial_buffer->buffer);
        partial_buffer->buffer = new_buffer;
        partial_buffer->length -= length;
        printf("finished clearing, current buffer length is %d\n", partial_buffer->length);
}

void printBufferList(bufferList buffer_list)
{       
        fprintf(stderr, "%s\n", "printing buffer list");
        for(int i = 0; i < buffer_list->size; i++){
                fprintf(stderr, "fd: %d\n", i);
                partialBuffer_ptr current_buffer = buffer_list->buffers[i];
                printPartialBuffer(current_buffer);
        }
}
