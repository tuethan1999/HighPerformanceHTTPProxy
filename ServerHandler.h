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
 int initiate_server_connection(HttpReqHead_T header, ServerListNode **server_list);

/*
 * Function: add_node
 * --------------------
 * Adds a node to the server list when a connection to a server is made
 *
 * server_list: a linked list containing info on current servers
 * header: struct containing info from a request header
 * sockfd: fd of the new connection
 * 
 *  returns: None
 */
 void add_node(ServerListNode **head_ptr, HttpReqHead_T header, int sockfd);

/*
 * Function: remove_list_node
 * --------------------
 * Removes a node from the server list
 *
 * server_list: a linked list containing info on current servers
 * sockfd: the sockfd of the server node to be removed
 * 
 *  returns: None
 */
 void remove_list_node(ServerListNode *server_list, int sockfd);

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

#endif
