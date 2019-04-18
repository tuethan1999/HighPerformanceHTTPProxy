#include "ServerHandler.h"
#include "HttpReqParser.h"
#include "HandleMessage.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>

ServerListNode* create_server_list() {
		ServerListNode *head = malloc(sizeof(ServerListNode));
		head->next = NULL;
		head->url = NULL;
		head->sockfd = -1;
		return head;
}

int initiate_server_connection(HttpReqHead_T header, ServerListNode **server_list) {
		//printf("in inititate_server_connection\n");
		int port = header->port;
		//printf("port = %d\n", port);
		struct hostent *server;
		server = gethostbyname(header->host);
		printf("%s\n", header->host);
		if (server == NULL) {
				error("ERROR, no such host");
		}
		struct sockaddr_in serv_addr;
		bzero((char *) &serv_addr, sizeof(serv_addr));
	    serv_addr.sin_family = AF_INET;
    	serv_addr.sin_port = htons(port);
    	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

    	//printf("setup socket\n");
    	int serv_fd = socket(AF_INET, SOCK_STREAM, 0);
    	if (serv_fd < 0)
        		error("ERROR opening new socket");

        //printf("about to connect\n");
    	if (connect(serv_fd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        		error("ERROR connecting to origin");
		//printf("connect successful\n");
        add_node(server_list, header, serv_fd);
        print_server_list(*server_list);

    	return serv_fd;
}

void add_node(ServerListNode **head_ptr, HttpReqHead_T header, int sockfd) {
		//printf("in add_node\n");
		
		ServerListNode *newNode = malloc(sizeof(ServerListNode));
		//printf("new node created\n");
		newNode->next = NULL;
		newNode->url = malloc(strlen(header->url) + 1);
		strcpy(newNode->url, header->url);
		newNode->sockfd = sockfd;

		//printf("going into if statement\n");
		if (*head_ptr == NULL) {
				printf("head node is NULL\n");
				*head_ptr = newNode;
		}
		else {
				ServerListNode *node = *head_ptr;
				while (node->next != NULL) {
						node = node->next; 
				}
				node->next = newNode;
		}
}

void remove_list_node(ServerListNode *server_list, int sockfd) {
		// TODO test
		printf("in remove_list_node, we're going to remove %d\n", sockfd);
		ServerListNode *node = server_list;
		ServerListNode *last;
		while (node != NULL) {
				printf("node's sockfd: %d\n", node->sockfd);
				if (node->sockfd == sockfd) {
						printf("match found!\n");
						if (node->next != NULL) {
								//ServerListNode *next = node->next;
								last->next = node->next;
								free(node);
						}
				}
				else {
						last = node;
						node = node->next;
				}
		}
		printf("sockfd not found\n");
}

char *is_server(int sockfd, ServerListNode *server_list) {
		printf("in is_server, we're looking for sockfd %d\n", sockfd);
		char *url = NULL;
		ServerListNode *node = server_list;
		while (node != NULL) {
				printf("node's sockfd: %d\n", node->sockfd);
				if (node->sockfd == sockfd) {
						printf("match found!\n");
						url = malloc(strlen(node->url) + 1);
						strcpy(url, node->url);
						return url;
				}
				else {
						node = node->next;
				}
		}
		return url;
}

void print_server_list(ServerListNode *server_list) {
		printf("in print_server_list\n");
		ServerListNode *node = server_list;
		while (node != NULL) {
				printf("url: %s sockfd: %d\n", node->url, node->sockfd);
				node = node->next;
		}
}

