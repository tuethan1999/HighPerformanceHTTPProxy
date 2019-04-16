/* 
 * Ethan Tu
 * High Performance HTTP Proxy
 *
 * HttpReqParser.c file
 *
 * Purpose: this module provides functionality to parse a
 * HTTP requests by taking in a buffer and storing it in
 * struct which has values for every possible optional argument
 */

#include "HttpReqParser.h"
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>


#ifndef FIELD_NAME_LENGTH
        #define FIELD_NAME_LENGTH 10
#endif
#ifndef CONTENT_LENGTH
        #define CONTENT_LENGTH 20
#endif

int string_is_number(char* s){
        int length = strlen(s);
        for(int i = 0; i < length; i ++){
                if(!isdigit(s[i]))
                        return 0;
        }
        return 1;
}

HttpReqHead_T new_req_head()
{
        HttpReqHead_T request_header = malloc(sizeof(*request_header));
        assert(request_header != NULL);

        /*set default values*/
        request_header->method = NULL;
        request_header->url = NULL;
        request_header->host = NULL;
        request_header->protocol = NULL;
        request_header->port = 80;
        return request_header;
}

int parse_http_req(HttpReqHead_T header, char* buffer, int buffer_size)
{
        /* copy buffer and make it null terminated*/
        char buf_string[buffer_size + 1];
        memset(buf_string, 0, buffer_size + 1);
        strncpy(buf_string, buffer, buffer_size);

        /* find where the end of the header*/
        char* header_end_indicator = "\r\n\r\n";
        char* header_end = strstr(buf_string, header_end_indicator);
        if(header_end == NULL) return 0;

        /*parse first line of request*/
        char *token = NULL;
        token = strtok(buf_string, "\r\n ");
        header->method = strdup(token);
        token = strtok(NULL, "\r\n ");
        header->url = strdup(token);
        token = strtok(NULL, "\r\n ");
        header->protocol = strdup(token);
        token = strtok(NULL, "\r\n ");
        /*find parameters*/
        while (token) {
                if(strncmp("Host", token, strlen("Host")) == 0){
                        token = strtok(NULL, "\r\n: ");
                        header->host = strdup(token);
                }
                if(string_is_number(token)){
                        header->port = atoi(token);
                }
                /*else if(strncmp("User-Agent", token, strlen("User-Agent")) == 0){
                        token = strtok(NULL, "\r\n: ");
                        header->user_agent = strdup(token);
                }
                else if(strncmp("Accept", token, strlen("Accept")) == 0){
                        token = strtok(NULL, "\r\n: ");
                        header->accept = strdup(token);
                }
                else if(strncmp("Proxy-Connection", token,
                 strlen("Proxy-Connection")) == 0){
                        token = strtok(NULL, "\r\n: ");
                        header->proxy_connection = strdup(token);
                }*/
                token = strtok(NULL, "\r\n: ");
        }
        return 1;
}

void print_http_req_head(HttpReqHead_T header)
{
        printf("--HTTP REQUEST HEADER--\n");
        printf("%*s: %*s\n", FIELD_NAME_LENGTH,  "Method", CONTENT_LENGTH, header->method);
        printf("%*s: %*s\n", FIELD_NAME_LENGTH, "Url", CONTENT_LENGTH, header->url);
        printf("%*s: %*s\n", FIELD_NAME_LENGTH, "Host", CONTENT_LENGTH, header->host);
        printf("%*s: %*s\n", FIELD_NAME_LENGTH, "Protocol", CONTENT_LENGTH, header->protocol);
        printf("%*s: %*d\n", FIELD_NAME_LENGTH, "Port", CONTENT_LENGTH, header->port);
        printf("\n");
}

void free_req_head(HttpReqHead_T header)
{
        free(header->method);
        free(header->url);
        free(header->host);
        free(header->protocol);
        free(header);
}