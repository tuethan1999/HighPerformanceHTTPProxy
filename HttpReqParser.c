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
#include <stdlib.h>

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

}

void print_http_req_head(HttpReqHead_T header)
{
        printf("--HTTP REQUEST HEADER--\n");
        printf("%*s\n", "Method", header->method);
        printf("%*s\n", "Url", header->url);
        printf("%*s\n", "Host", header->host);
        printf("%*s\n", "Protocol", header->protocol);
        printf("%*s\n", "Port", header->port);
}

void free_req_head(HttpReqHead_T header)
{

}

#endif