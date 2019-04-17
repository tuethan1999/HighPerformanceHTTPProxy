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

int inititate_server_connection(HttpReqHead_T header, ServerListNode *server_list) {
		printf("in inititate_server_connection\n");
		int port = header->port;
		struct hostent *server;
		server = gethostbyname(header->host);
		struct sockaddr_in orig_addr;
		bzero((char *) &orig_addr, sizeof(orig_addr));
	    orig_addr.sin_family = AF_INET;
	    bcopy((char *)server->h_addr,
        (char *)&orig_addr.sin_addr.s_addr,
        server->h_length);
    	orig_addr.sin_port = htons(port);

    	int serv_fd = socket(AF_INET, SOCK_STREAM, 0);
    	if (serv_fd < 0)
        		error("ERROR opening new socket");

    	if (connect(serv_fd,(struct sockaddr *) &orig_addr,sizeof(orig_addr)) < 0)
        		error("ERROR connecting to origin");

        add_node(server_list, header, serv_fd);
        print_server_list(server_list);

    	return serv_fd;
}

void add_node(ServerListNode *server_list, HttpReqHead_T header, int sockfd) {
		ServerListNode *newNode = malloc(sizeof(ServerListNode));
		newNode->next = NULL;
		strcpy(newNode->url, header->url);
		newNode->sockfd = sockfd;

		if (server_list == NULL) {
				server_list = newNode;
		}
		else {
				ServerListNode *node = server_list;
				while (node->next != NULL) {
						node = node->next; 
				}
				node->next = newNode;
		}
}

void remove_list_node(ServerListNode *server_list) {
		// TODO
}

char *is_server(int sockfd, ServerListNode *server_list) {
		char *url = NULL;
		ServerListNode *node = server_list;
		while (node != NULL) {
				if (node->sockfd == sockfd) {
						strcpy(url, node->url);
				}
				else {
						node = node->next;
				}
		}
		return url;
}

void print_server_list(ServerListNode *server_list) {
		ServerListNode *node = server_list;
		while (node != NULL) {
				printf("url: %s sockfd: %d\n", node->url, node->sockfd);
				node = node->next;
		}
}

void forward_request_to_server(int sockfd, char *msg, int length) {

}
