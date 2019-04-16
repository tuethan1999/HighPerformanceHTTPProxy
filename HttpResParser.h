/* 
 * Ethan Tu
 * High Performance HTTP Proxy
 *
 * HttpResParser.h file
 *
 * Purpose: this module provides functionality to parse a
 * HTTP response by taking in a buffer and storing it in
 * struct which has values for every possible optional argument
 */
#include <stdlib.h>

#ifndef HTTP_RESPONSE_PARSER_H_INCLUDED
#define HTTP_RESPONSE_PARSER_H_INCLUDED

typedef struct HttpResHead_T{
        int max_age;
        int header_length;
        int content_length;
}*HttpResHead_T;

/*
 * Function:  new_res_head 
 * --------------------
 * Allocates space for a HttpResHead_T and returns it
 * 
 *  returns: HttpResHead_T
 */
HttpResHead_T new_res_head();

/*
 * Function:  parse_http_res 
 * --------------------
 * Determines whether a buffer contains a valid http header and parses it if so.
 *
 *  header: HttpResHead_T to put the parsed header into
 *  buffer: buffer with data, probably from a TCP read, doesn't have to be null terminated
 *  buffer_size: number of bytes in the buffer (make sure unused bytes aren't zero though)
 *
 *  returns: 1 if header successfully was parsed
 *           0 if there was no complete header
 *           -1 if there was an error
 */
int parse_http_res(HttpResHead_T header, char* buffer, int buffer_size);

/*
 * Function:  print_http_res_head 
 * --------------------
 * Prints the arguments in the http response header
 *
 *  header: HttpResHead_T to print
 *
 *  returns: None
 */
void print_http_res_head(HttpResHead_T header);

/*
 * Function:  free_res_head
 * --------------------
 * Frees memory allocated for HttpResHead_T
 *
 *  header: HttpResHead_T to free
 *
 *  returns: None
 */
void free_res_head(HttpResHead_T header);

#endif