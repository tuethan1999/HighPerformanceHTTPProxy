#include "HandleMessage.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h> 

void error(const char *msg) {
		perror(msg);
		exit(1);}

void initialize_partial_msg_buffer(msg_buffer *buf_array, int index) {
		//printf("in initialize_partial_msg_buffer\n");
		buf_array[index].buffer = malloc(BUFFSIZE);
		buf_array[index].size = BUFFSIZE;
		buf_array[index].length = 0;
}

void increase_buffer_size(msg_buffer *buf_array, int index) {
		//printf("in increase_buffer_size\n");
		char *new_buf = malloc(2*buf_array[index].size);
		strncpy(new_buf, buf_array[index].buffer, buf_array[index].length);
		free(buf_array[index].buffer);
		buf_array[index].buffer = new_buf;
		buf_array[index].size *= 2;
}

void add_to_partial_msg_buffer(char *msg, msg_buffer *buf_array, int index, int length) {
		//printf("in add_to_partial_msg_buffer\n");
		//printf("current buffer size = %d\ncurrent buffer length = %d\n", buf_array[index].size, buf_array[index].length);
		if (length >= buf_array[index].size - buf_array[index].length) {
				printf("must increase buffer size\n");
				increase_buffer_size(buf_array, index);
		}
		int position = buf_array[index].length;
		strncpy(&buf_array[index].buffer[position], msg, length);
		buf_array[index].length += length;
}

void print_partial_msg_buffer(msg_buffer *buf_array, int length) {
		//printf("print_partial_msg_buffer:\n");
		for (int i = 0; i < length; i++) {
				printf("sockfd: %d\n", i);
				for (int j = 0; j < buf_array[i].length; j++) {
						printf("%c", buf_array[i].buffer[j]);
				}
		}
}

void clear_buffer(msg_buffer *buf_array, int index) {
		if (buf_array[index].buffer != NULL)
				bzero(buf_array[index].buffer, MAX_LENGTH);
}

void clear_header_from_buffer(msg_buffer *buf_array, int index, int header_length) {
		assert(header_length <= MAX_LENGTH);
		for (int i = header_length; i < MAX_LENGTH; i++) {
				buf_array[index].buffer[i-header_length] = buf_array[index].buffer[header_length];
		}
}

void delete_buffer(msg_buffer *buf_array, int index) {
		if (buf_array[index].buffer != NULL)
				free(buf_array[index].buffer);
		buf_array[index].length = 0;
		buf_array[index].size = 0;
}

void handle_incoming_message(msg_buffer *buf_array, int sockfd, int listen_fd, fd_set *master_fd_set, int *max_sock) {
		if (sockfd == listen_fd) {
				printf("message incoming on listen port, opening new port\n");
				
				struct sockaddr_in new_addr;
				socklen_t addr_len = sizeof(new_addr);
				int new_sock = accept(listen_fd, (struct sockaddr *) &new_addr, &addr_len);
				if (new_sock < 0)
						error("ERROR on accept");
				FD_SET(new_sock, master_fd_set);
				initialize_partial_msg_buffer(buf_array, new_sock);
				printf("new sockfd: %d\n", new_sock);
				if (new_sock > *max_sock)
						*max_sock = new_sock;
		}
		else {
				printf("message incoming on existing port\n");
				char *buffer = malloc(sizeof(char)* BUFFSIZE);
				bzero(buffer, BUFFSIZE);
				int n = read(sockfd, buffer, BUFFSIZE);
				if (n < 0)
						error("ERROR writing to socket");
				if (n == 0) {
						printf("n = 0\n");
						clear_buffer(buf_array, sockfd);
						delete_buffer(buf_array, sockfd);
						close(sockfd);
						FD_CLR(sockfd, master_fd_set);
				}
				else {
						add_to_partial_msg_buffer(buffer, buf_array, sockfd, n);
				}
		}

}
