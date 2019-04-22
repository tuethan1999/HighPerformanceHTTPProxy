/* 
 * Ethan Tu
 * High Performance HTTP Proxy
 *
 * HttpResParser.c file
 *
 * Purpose: this module provides functionality to parse a
 * HTTP responses by taking in a buffer and storing it in
 * struct which has values for every possible optional argument
 */

#include "HttpResParser.h"
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>

#ifndef FIELD_NAME_LENGTH
        #define FIELD_NAME_LENGTH 20
#endif
#ifndef CONTENT_LENGTH
        #define CONTENT_LENGTH 20
#endif
#ifndef DEFAULT_MAX_AGE
        #define DEFAULT_MAX_AGE 60
#endif
#ifndef MAX_HEADER_SIZE
        #define MAX_HEADER_SIZE 16000
#endif
#ifndef MIN
        #define MIN( a, b ) ( ((a) < (b)) ? (a) : (b) )
#endif



HttpResHead_T new_res_head()
{
        HttpResHead_T response_header = malloc(sizeof(*response_header));
        assert(response_header != NULL);

        response_header->max_age = DEFAULT_MAX_AGE;
        response_header->header_length = -1;
        response_header->content_length = -1;
        return response_header;
}

int parse_http_res(HttpResHead_T header, char* buffer, int buffer_size)
{
        buffer_size = MIN(buffer_size, MAX_HEADER_SIZE);
        char buf_string[buffer_size + 1];
        memset(buf_string, 0, buffer_size + 1);
        strncpy(buf_string, buffer, buffer_size);

        /* find where the end of the header*/
        char* header_end_indicator = "\r\n\r\n";
        char* header_end = strstr(buf_string, header_end_indicator);
        if(header_end == NULL) return 0;

        /*extract header*/
        header->header_length = strlen(buf_string) - strlen(header_end) + 4;
        char response_header_only[header->header_length + 1];
        sprintf(response_header_only, "%.*s", header->header_length, buf_string);

        /*parse header*/
        char *token = NULL;
        token = strtok(response_header_only, "\r\n: ");
        /*header->protocol = strdup(token);*/
        token = strtok(NULL, "\r\n: ");
        /*header->status_code = atoi(token);*/
        token = strtok(NULL, "\r\n: ");
        /*header->status_message = strdup(token);*/
        token = strtok(NULL, "\r\n: ");

        while (token) {
                if(strncmp("Content-Length", token, strlen("Content-Length")) == 0){
                        token = strtok(NULL, "\r\n: ");
                        header->content_length = atoi(token);
                }
                else if(strncmp("Cache-Control", token, strlen("Cache-Control")) == 0){
                        token = strtok(NULL, "\r\n:= ");
                        printf("CACHE CONTROL\n\n%s\n", token);
                        if(strncmp("max-age", token, strlen("max-age")) == 0){
                                token = strtok(NULL, "\r\n:= ");
                                header->max_age = atoi(token);
                        }        
                }
                token = strtok(NULL, "\r\n: ");
        }
        return 1;
}

void print_http_res_head(HttpResHead_T header)
{
        printf("--HTTP RESPONSE HEADER--\n");
        printf("%*s: %*d\n", FIELD_NAME_LENGTH, "CACHE CONTROL: max-age", CONTENT_LENGTH, header->max_age);
        printf("%*s: %*d\n", FIELD_NAME_LENGTH, "Header-Length", CONTENT_LENGTH, header->header_length);
        printf("%*s: %*d\n", FIELD_NAME_LENGTH, "Content-Length", CONTENT_LENGTH, header->content_length);
        printf("\n");
}

void free_res_head(HttpResHead_T header)
{
        free(header);
}
