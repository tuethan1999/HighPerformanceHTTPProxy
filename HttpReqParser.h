/* 
 * Ethan Tu
 * High Performance HTTP Proxy
 *
 * HttpReqParser.h file
 *
 * Purpose: this module provides functionality to parse a
 * HTTP requests by taking in a buffer and storing it in
 * struct which has values for every possible optional argument
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef HTTP_REQUEST_PARSER_H_INCLUDED
#define HTTP_REQUEST_PARSER_H_INCLUDED


typedef struct HttpReqHead_T {
        char *method;   /*ex: GET*/
        char *url;      /*ex: http://www.cs.cmu.edu/~prs/bio.html*/
        char *host;     /*ex: www.cs.cmu.edu*/
        char *protocol; /*ex: HTTP/1.1*/
        int port;       /*ex: 80*/
}*HttpReqHead_T;

/*
 * Function:  new_req_head 
 * --------------------
 * Allocates space for a HttpReqHead_T, and returns it
 * Sets default values for fields, NULL for strings, 80 for port
 * 
 *  returns: HttpReqHead_T
 */
HttpReqHead_T new_req_head();

/*
 * Function:  parse_http_req 
 * --------------------
 * Determines whether a buffer contains a valid http header and parses it if so.
 *
 *  header: HttpReqHead_T to put the parsed header into
 *  buffer: buffer with data, probably from a TCP read, doesn't have to be null terminated
 *  buffer_size: number of bytes in the buffer (make sure unused bytes aren't zero though)
 *
 *  returns: 1 if header successfully was parsed
 *           0 if there was no complete header
 *           -1 if there was an error
 */
int parse_http_req(HttpReqHead_T header, char* buffer, int buffer_size);

/*
 * Function:  print_http_req_head 
 * --------------------
 * Prints the arguments in the http request header
 *
 *  header: HttpReqHead_T to print
 *
 *  returns: None
 */
void print_http_req_head(HttpReqHead_T header);

/*
 * Function:  free_req_head
 * --------------------
 * Frees memory allocated for HttpReqHead_T
 *
 *  header: HttpReqHead_T to free
 *
 *  returns: None
 */
void free_req_head(HttpReqHead_T header);

#endif