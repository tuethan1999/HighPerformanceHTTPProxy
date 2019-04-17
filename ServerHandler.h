/* 
 * Nate Krinsky
 * High Performance HTTP Proxy
 *
 * ServerHandler.h file
 *
 * Purpose: this module communicates with servers and
 * keeps track of which servers we're talking to
 */
#include "HttpReqParser.h"

#ifndef SERVERHANDLER_H_INCLUDED
#define SERVERHANDLER_H_INCLUDED

typedef struct ServerListNode {
		int sockfd;
		char *url;
		struct ServerListNode *next;
}ServerListNode;

/*
 * Function: create_server_list
 * --------------------
 * Starts a linked list of ServerListNodes
 * 
 *  returns: a pointer to the head of the list
 */
ServerListNode* create_server_list();

/*
 * Function: initiate_server_connection
 * --------------------
 * Connect to a server so we can forward a request to it
 *
 * header: struct containing info from a request header
 * server_list: a linked list containing info on current servers
 * 
 *  returns: the file descriptor of the new connection
 */
 int initiate_server_connection(HttpReqHead_T header, ServerListNode *server_list);

/*
 * Function: add_node
 * --------------------
 * 
 *
 * server_list:
 * header:
 * sockfd:
 * 
 *  returns: 
 */
 void add_node(ServerListNode *server_list, HttpReqHead_T header, int sockfd);

/*
 * Function: remove_list_node
 * --------------------
 * Removes a node from the server list
 *
 * server_list: a linked list containing info on current servers
 * 
 *  returns: None
 */
 void remove_list_node(ServerListNode *server_list);

/*
 * Function: is_server
 * --------------------
 * Given a sockfd, lets you know whether it's a client or server
 *	
 * sockfd: fd of the incoming message
 * server_list: a linked list containing info on current servers
 * 
 *  returns: associated url if the sockfd refers to a server
 			 NULL if the sockfd refers to a client
 */
char *is_server(int sockfd, ServerListNode *server_list);

/*
 * Function: print_server_list
 * --------------------
 * For testing, print contents of the server list
 *	
 * sockfd: fd of the incoming message
 * server_list: a linked list containing info on current servers
 * 
 *  returns: None
 */
void print_server_list(ServerListNode *server_list);

/*
 * Function: forward_request_to_server
 * --------------------
 * Forwards the request header to the server
 *	
 * sockfd: fd of the server
 * msg: char buffer to send
 * length: number of bytes in msg
 * 
 *  returns: None
 */
void forward_request_to_server(int sockfd, char *msg, int length);

#endif
