/* 
 * Nate Krinsky
 * High Performance HTTP Proxy
 *
 * HandleMessage.h file
 *
 * Purpose: this module handles messages from clients
 * and servers and stores partial messages in a buffer array
 */
#include <sys/select.h>

#ifndef HANDLEMESSAGE_H_INCLUDED
#define HANDLEMESSAGE_H_INCLUDED

#define MAX_LENGTH 40960
#define BUFFSIZE 4096

typedef struct msg_buffer {
		char *buffer;
		int size;
		int length;
} msg_buffer;

/*
 * Function: initialize_partial_msg_buffer
 * --------------------
 * Intitialzes the partial message buffer array for a client or server
 *
 * buf_array: the full array of partial message buffers
 * index: index of the client or server in the partial buffer array, equal to its sockfd
 * 
 *  returns: None
 */
void initialize_partial_msg_buffer(msg_buffer *buf_array, int index);

/*
 * Function: increase_buffer_size
 * --------------------
 * Dynamically doubles the size of a buffer in the array
 *
 * buf_array: the full array of partial message buffers
 * index: index of the client or server in the partial buffer array, equal to its sockfd
 *
 * Returns: None
 */
void increase_buffer_size(msg_buffer *buf_array, int index);

/*
 * Function: add_to_partial_msg_buffer
 * --------------------
 * Adds a char array to a server or client's existing partial message array
 *
 * msg: a char array of the (partial) message received
 * buf_array: the full array of partial message buffers
 * index: index of the client or server in the partial buffer array, equal to its sockfd
 *
 *  returns: None
 */
void add_to_partial_msg_buffer(char *msg, msg_buffer *buf_array, int index, int length);

/*
 * Function: print_partial_msg_buffer
 * --------------------
 * Prints the full array of partial buffers for testing pruposes
 *
 * buf_array: the full array of partial message buffers
 * length: the number of entries in buf_array
 * 
 *  returns: None
 */
void print_partial_msg_buffer(msg_buffer *buf_array, int length);

/*
 * Function: clear_buffer
 * --------------------
 * Fully clears the buffer of a particular server or client
 *
 * buf_array: the full array of partial message buffers
 * index: index of the client or server in the partial buffer array, equal to its sockfd
 *
 *  returns: None
 */
void clear_buffer(msg_buffer *buf_array, int index);

/*
 * Function: clear_header_from_buffer 
 * --------------------
 * For a server's buffer, clear out the header portion and move the body so that it
 * starts at index 0. Should only be called once the header info has been parsed and
 * stored elsewhere
 *
 * buf_array: the full array of partial message buffers
 * index: index of the client or server in the partial buffer array, equal to its sockfd
 * hader_length: length of the portion of the buffer that contains the header
 *
 *  returns: None
 */
void clear_header_from_buffer(msg_buffer *buf_array, int index, int header_length);

/*
 * Function: delete_buffer
 * --------------------
 * Frees the memory of a buffer for a server or client that is no longer talking to the proxy
 *
 * buf_array: the full array of partial message buffers
 * index: index of the client or server in the partial buffer array, equal to its sockfd
 *
 *  returns: None
 */
void delete_buffer(msg_buffer *buf_array, int index);

/*
 * Function: handle_incoming_message
 * --------------------
 * Handles an incoming message from a server or client
 *
 * buf_array: the full array of partial message buffers
 * index: index of the client or server in the partial buffer array, equal to its sockfd
 *
 *  returns: None
 */
//void handle_incoming_message(msg_buffer *buf_array, int sockfd, int listen_fd);
void handle_incoming_message(msg_buffer *buf_array, int sockfd, int listen_fd, fd_set *master_fd_set, int *max_sock);

void error(const char *msg);

#endif
