#include "assert.h"
#include "HttpReqParser.h"
#include "HttpResParser.h"
#include "HttpCache.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <time.h>
#include <sys/select.h>
#include <sys/time.h>

#define BUFSIZE 2*4096
#define BUFLISTSIZE 1
#define MAX( a, b ) ( ((a) > (b)) ? (a) : (b) )

void server_error(char *msg) {
        perror(msg);
        exit(EXIT_FAILURE);
}

/*************************************************************************************************************************************/
typedef struct secureNode {
        int client_fd;
        int server_fd;
}*secureNode_ptr;
secureNode_ptr newSecureNode(int client, int server);
/*************************************************************************************************************************************/
typedef struct secureNodeList {
        secureNode_ptr *nodes;
        int size;
        int length;
}*secureNodeList;
secureNodeList newSecureNodeList();
void insertSecureNode(secureNodeList node_list, secureNode_ptr node);
void deleteSecureNode(secureNodeList node_list, int sockfd);
int findNodeBySockfd(secureNodeList node_list, int sockfd);
void printSecureNodeList(secureNodeList node_list);
/*************************************************************************************************************************************/
typedef struct serverNode {
        int fd;
        char *url;
        struct serverNode *next;
}*serverNode_ptr;
serverNode_ptr newServerNode();
void pushFrontServerNode(serverNode_ptr *server_list, serverNode_ptr new_node);
void printServerList(serverNode_ptr server_list);
char* isServer(serverNode_ptr server_list, int fd);
/*************************************************************************************************************************************/
typedef struct partialBuffer {
        char *buffer;
        int length;
        int size;
}*partialBuffer_ptr;
partialBuffer_ptr newPartialBuffer();
void insertPartialBuffer(partialBuffer_ptr partial_buffer, char* msg, int length);
void printPartialBuffer(partialBuffer_ptr partial_buffer);
void deletePartialBuffer(partialBuffer_ptr partial_buffer);
/*************************************************************************************************************************************/
typedef struct bufferListStruct{
        partialBuffer_ptr *buffers;
        int size;
}*bufferList;
bufferList newBufferList();
void insertBufferList(bufferList buffer_list, partialBuffer_ptr partial_buffer, int index);
void deleteFromBufferList(bufferList buffer_list, int index);
void clearFromBufferList(bufferList buffer_list, int index);
void printBufferList(bufferList buffer_list);
/*************************************************************************************************************************************/
int setupListenSocket(int port_number);
int setupServerSocket(HttpReqHead_T header);
void handleNewConnection(int fd, fd_set *master_fd_set, bufferList buffer_list, int *max_sock_ptr);
int handleExistingConnection(int fd, fd_set *master_fd_set, int *max_sock_ptr, bufferList buffer_list, Cache_T cache, serverNode_ptr *server_list, secureNodeList secure_list);
void handleDisconnect(int fd, fd_set *master_fd_set, bufferList buffer_list, int *max_sock_ptr, secureNodeList node_list);
void handleClient(int fd, fd_set *master_fd_set, int *max_sock_ptr, bufferList buffer_list, Cache_T cache, serverNode_ptr *server_list, secureNodeList secure_list);
void handleServer(int fd, bufferList buffer_list, Cache_T cache, char* url, secureNodeList secure_list);
/*************************************************************************************************************************************/
char* headerWithAge(char* msg, int* msg_size, int age);
void sendConnectionEstablishedHeader(int serv_fd);
/*************************************************************************************************************************************/


int main(int argc, char *argv[])
{
        if(argc != 2){
                fprintf(stderr, "usage %s <port>\n", argv[0]);
                exit(1);
        }
        int c_portnum = atoi(argv[1]);
        int listen_fd = setupListenSocket(c_portnum);
        int rv;
        struct timeval timeout;
        fd_set master_fd_set, copy_fd_set;
        FD_ZERO(&master_fd_set);
        FD_ZERO(&copy_fd_set);
        FD_SET(listen_fd, &master_fd_set);

        Cache_T cache = new_cache();
        serverNode_ptr server_list = NULL;
        bufferList buffer_list = newBufferList();
        secureNodeList secure_list = newSecureNodeList();
        int max_sock = listen_fd;

        while(1){    
                //sleep(2);    
                FD_ZERO(&copy_fd_set);
                memcpy(&copy_fd_set, &master_fd_set, sizeof(master_fd_set));
                timeout.tv_sec = 1;
                timeout.tv_usec = 1000;
                rv = select(max_sock+1, &copy_fd_set, NULL, NULL, &timeout);

                if (rv < 0) {
                        // TODO implement error handling
                }
                else if (rv == 0) {
                        // TODO implement timeout
                        printf("timeout\n");
                }
                else {
                        for (int fd = 0; fd < max_sock+1; fd++) {
                                if (!FD_ISSET(fd, &copy_fd_set))
                                        continue;
                                if (fd == listen_fd){
                                        handleNewConnection(fd, &master_fd_set, buffer_list, &max_sock);
                                }
                                else if(!handleExistingConnection(fd, &master_fd_set, &max_sock, buffer_list, cache, &server_list, secure_list)){
                                        handleDisconnect(fd, &master_fd_set, buffer_list, &max_sock, secure_list);
                                }
                        }
                        //print_cache(cache);
                }
                delete_expired(cache);
        }  
        return 0;
}
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/

int setupListenSocket(int port_number)
{
        int parent_fd, opt_val;
        struct sockaddr_in server_addr;

        /*socket: create parent socket*/
        parent_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(parent_fd < 0) server_error("ERROR opening socket!");

        /*Debugging trick to avoid "ERROR on binding: Address already in use"*/
        opt_val =1;
        setsockopt(parent_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt_val, sizeof(int));

        /*build server's internal address*/
        bzero((char *) &server_addr, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        server_addr.sin_port = htons((unsigned short)port_number);
        
        /*binding: associating the server with a port*/
        if(bind(parent_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) server_error("ERROR on binding!");
        
        /*listening: make this socket ready to accept connection requests*/
        if(listen(parent_fd, 5) < 0 ) server_error("ERROR on listen!");

        return parent_fd;
}

int setupServerSocket(HttpReqHead_T header){
        int port = header->port;
        struct hostent *server;
        server = gethostbyname(header->host);
        if (server == NULL)server_error("ERROR, no such host");

        struct sockaddr_in serv_addr;
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);
        bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

        int serv_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (serv_fd < 0) server_error("ERROR opening new socket");

        if (connect(serv_fd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) server_error("ERROR connecting to origin");

        return serv_fd;
}

void handleNewConnection(int fd, fd_set* master_fd_set, bufferList buffer_list, int *max_sock_ptr)
{
        /* Connection request on original socket. */
        int new_fd;

        /*accept the connection and put it in the master set*/
        new_fd = accept (fd, NULL, NULL);
        if (new_fd < 0)
                server_error("accept");
        fprintf(stderr, "New connection on fd %d\n", new_fd);
        FD_SET(new_fd, master_fd_set);
        *max_sock_ptr = MAX(*max_sock_ptr, new_fd);

        /*allocate space in the Partial Buffer for the new connection*/
        partialBuffer_ptr new_partial_buffer =  newPartialBuffer();
        insertBufferList(buffer_list, new_partial_buffer, new_fd);
}

int handleExistingConnection(int fd, fd_set *master_fd_set, int *max_sock_ptr, bufferList buffer_list, Cache_T cache, serverNode_ptr *server_list, secureNodeList secure_list)
{
        fprintf(stderr, "Message incoming on existing fd %d\n", fd);
        char *buffer = malloc(sizeof(char)* BUFSIZE);
        bzero(buffer, BUFSIZE);
        int n = read(fd, buffer, BUFSIZE);
        if (n < 0)
                //server_error("ERROR reading from socket");
                printf("ERROR reading from socket\n");
        else if (n == 0) {
                free(buffer);
                return 0;
        }
        else {
                fprintf(stderr, "Read %d bytes from fd: %d\n", n, fd);
                insertPartialBuffer(buffer_list->buffers[fd], buffer, n);
                printf("successfully added to partial message buffer\n");
                char *url = isServer(*server_list, fd);
                if(url){
                        handleServer(fd, buffer_list, cache, url, secure_list);
                }
                else{
                        handleClient(fd, master_fd_set, max_sock_ptr, buffer_list, cache, server_list, secure_list);
                }
        }
        free(buffer);
        return 1;
}

void handleDisconnect(int fd, fd_set* master_fd_set, bufferList buffer_list, int *max_sock_ptr, secureNodeList node_list)
{
        fprintf(stderr, "Handle disconnection of fd %d\n", fd);
        deleteFromBufferList(buffer_list, fd);
        close(fd);
        if(*max_sock_ptr == fd) 
                *max_sock_ptr -=1;
        deleteSecureNode(node_list, fd);
        FD_CLR(fd, master_fd_set);
}

void handleClient(int fd, fd_set *master_fd_set, int *max_sock_ptr, bufferList buffer_list, Cache_T cache, serverNode_ptr *server_list, secureNodeList secure_list)
{
        HttpReqHead_T req_header = new_req_head();
        partialBuffer_ptr partial_buffer = buffer_list->buffers[fd];
        printf("attemping to parse buffer, might break things\n");
        if(parse_http_req(req_header, partial_buffer->buffer, partial_buffer->length))
        {
                fprintf(stderr, "fd %d has a complete header\n", fd);
                print_http_req_head(req_header);
                CacheObj_T cache_obj = find_by_url(cache, req_header->url);
                if(cache_obj == NULL){
                        /*make new cache_object*/
                        cache_obj = new_cache_object();
                        cache_obj->req_header = req_header;

                        cache_obj->url = malloc(strlen(req_header->url) + 1);
                        strncpy(cache_obj->url, req_header->url, strlen(req_header->url));
                        cache_obj->request_buffer = malloc(partial_buffer->length);
                        memcpy(cache_obj->request_buffer, partial_buffer->buffer, partial_buffer->length);

                        cache_obj->request_length = partial_buffer->length;
                        cache_obj->last_requested = time(NULL);
                        utarray_push_back(cache_obj->client_fds, &fd);
                        insert_into_cache(cache, cache_obj);

                        /*create connection to server and add node to server list*/
                        int serv_fd = setupServerSocket(req_header);
                        FD_SET(serv_fd, master_fd_set);
                        *max_sock_ptr = MAX(serv_fd, *max_sock_ptr);
                        serverNode_ptr new_node = newServerNode();
                        new_node->url = malloc(strlen(req_header->url) + 1);
                        strcpy(new_node->url, req_header->url);
                        new_node->fd = serv_fd;
                        pushFrontServerNode(server_list, new_node);
                        printServerList(*server_list);

                        /*set up space for partial buffer*/
                        partialBuffer_ptr new_partial_buffer =  newPartialBuffer();
                        insertBufferList(buffer_list, new_partial_buffer, serv_fd);

                        printf("req_header->method = %s\n", req_header->method);

                        /*write to server*/
                        if (strcmp(req_header->method, "GET") == 0) {
                                printf("writing this to the server:\n");
                                for (int i = 0; i < cache_obj->request_length; i++) {
                                        printf("%c", cache_obj->request_buffer[i]);
                                }
                                int n = write(serv_fd, cache_obj->request_buffer, cache_obj->request_length);
                                if (n < 0)
                                        server_error("ERROR writing to server");
                                fprintf(stderr, "wrote %d bytes to server\n", n);
                        }
                        else if (strcmp(req_header->method, "CONNECT") == 0) {
                                printf("CONNECT request received\nsending confirmation back to client\n");
                                sendConnectionEstablishedHeader(fd);
                                printf("Sent header\n");
                                secureNode_ptr node = newSecureNode(fd, serv_fd);
                                insertSecureNode(secure_list, node);
                                printSecureNodeList(secure_list);
                        }
                }
                else if(cache_obj->last_updated != -1 && !is_expired(cache_obj)){
                        fprintf(stderr, "%s\n", "Valid response in cache");
                        fprintf(stderr, "age: %d, max age: %d\n", (time(NULL) - cache_obj->last_updated), cache_obj->res_header->max_age);
                        int final_msg_size = cache_obj->response_length;
                        cache_obj->last_requested = time(NULL);
                        char *final_msg = headerWithAge(cache_obj->response_buffer, &final_msg_size, (time(NULL) - cache_obj->last_updated));

                        int n = write(fd, final_msg, final_msg_size);
                        if (n < 0)
                                server_error("ERROR writing to client");
                        fprintf(stderr, "wrote %d bytes to client\n", n);

                        free(final_msg);
                        delete_from_clientfds(cache_obj, fd);
                }
                else{
                        fprintf(stderr, "%s\n", "Invalid response in cache");
                }
                clearFromBufferList(buffer_list, fd);
        }
        else if (findNodeBySockfd(secure_list, fd) > 0) {
                int dest_fd = findNodeBySockfd(secure_list, fd);
                printf("Tunneling data from fd %d to fd %d\n", fd, dest_fd);
                //sleep(5);
                int n = write(dest_fd, partial_buffer->buffer, partial_buffer->length);
                if (n < 0)
                        server_error("ERROR tunneling data");
                printf("Tunneled %d bytes from fd %d to fd %d\n", n, fd, dest_fd);
                //sleep(5);
                clearFromBufferList(buffer_list, fd);
        }
}

void handleServer(int fd, bufferList buffer_list, Cache_T cache, char* url, secureNodeList secure_list){
        HttpResHead_T res_header = new_res_head();
        partialBuffer_ptr partial_buffer = buffer_list->buffers[fd];
        int complete_header = parse_http_res(res_header, partial_buffer->buffer, partial_buffer->length);
        if(complete_header && res_header->header_length + res_header->content_length == partial_buffer->length)
        {
                print_http_res_head(res_header);
                CacheObj_T cache_obj = find_by_url(cache, url);
                assert(cache_obj != NULL);
                cache_obj->response_buffer = realloc(cache_obj->response_buffer, partial_buffer->length);
                memcpy(cache_obj->response_buffer, partial_buffer->buffer, partial_buffer->length);
                cache_obj->res_header = res_header;
                cache_obj->response_length = partial_buffer->length;
                cache_obj->last_updated = time(NULL);

                int final_msg_size = partial_buffer->length;
                char *final_msg = headerWithAge(partial_buffer->buffer, &final_msg_size, 0);
                fprintf(stderr, "final_msg_size: %d\n", final_msg_size);
                for(int *p=(int*)utarray_front(cache_obj->client_fds); p!=NULL; p=(int*)utarray_next(cache_obj->client_fds,p)) {
                        printf("in handleServer for loop\n");
                        int n = write(*p, final_msg, final_msg_size);
                        if (n < 0){
                                fprintf(stderr, "socket: %d\n", *p);
                                server_error("ERROR writing to socket");
                        }
                        printf("wrote %d bytes to fd %d\n", n, *p);
                }
                free(final_msg);
        }
        else if (findNodeBySockfd(secure_list, fd) > 0) {
                int dest_fd = findNodeBySockfd(secure_list, fd);
                printf("Tunneling data from fd %d to fd %d\n", fd, dest_fd);
                //sleep(5);
                int n = write(dest_fd, partial_buffer->buffer, partial_buffer->length);
                if (n < 0)
                        server_error("ERROR tunneling data");
                printf("Tunneled %d bytes from fd %d to fd %d\n", n, fd, dest_fd);
                //sleep(5);
                clearFromBufferList(buffer_list, fd);
        }
}
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
serverNode_ptr newServerNode()
{
        serverNode_ptr server = malloc(sizeof(*server));
        if (server == NULL) server_error("create serverNode");
        server->fd = -1;
        server->url = NULL;
        server->next  = NULL;
        return server;
}

void pushFrontServerNode(serverNode_ptr *server_list, serverNode_ptr new_node)
{
        if (*server_list == NULL) {
                fprintf(stderr, "Server list is empty\n");
                *server_list = new_node;
        }
        else {
                serverNode_ptr temp = *server_list;
                *server_list = new_node;
                new_node->next = temp;
        }
}
void printServerList(serverNode_ptr server_list)
{
        if(server_list == NULL){
                fprintf(stderr, "%s\n", "empty server list");
                return;
        }
        while(server_list != NULL){
                fprintf(stderr, "fd: %d, url: %s\n", server_list->fd, server_list->url);
                server_list = server_list->next;
        }
}

char* isServer(serverNode_ptr server_list, int fd)
{
        char *url = NULL;
        while (server_list != NULL) {
                if (server_list->fd == fd) {
                        url = malloc(strlen(server_list->url) + 1);
                        strcpy(url, server_list->url);
                        return url;
                }
                else {
                        server_list = server_list->next;
                }
        }
        return url;
}
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
partialBuffer_ptr newPartialBuffer()
{
        partialBuffer_ptr buf = malloc(sizeof(*buf));
        if (buf == NULL) server_error("create partialBuffer_ptr");
        buf->buffer = malloc(BUFSIZE);
        memset(buf->buffer, 0, BUFSIZE);
        if (buf->buffer == NULL) server_error("create partialBuffer_ptr buffer");
        buf->size = BUFSIZE;
        buf->length = 0;
        return buf;
}

void insertPartialBuffer(partialBuffer_ptr partial_buffer, char* msg, int length)
{
        printf("in insertPartialBuffer\n");
        if (length >= partial_buffer->size - partial_buffer->length) {
                int new_size = 2*partial_buffer->size;
                fprintf(stderr, "length of msg is %d, expanding partial buffer size from %d -> %d\n", length, partial_buffer->size, new_size);
                partial_buffer->buffer = realloc(partial_buffer->buffer, new_size);
                partial_buffer->size = new_size;
        }
        int position = partial_buffer->length;
        memcpy(partial_buffer->buffer + position, msg, length);
        partial_buffer->length += length;
}

void printPartialBuffer(partialBuffer_ptr partial_buffer)
{
        if(partial_buffer == NULL || partial_buffer->buffer == NULL) return;
        if(partial_buffer->length == 0){
                fprintf(stderr, "--empty partial buffer--\n");
                return;
        }
        fprintf(stderr, "--------------------------------------\n");
        for(int i = 0; i < partial_buffer->length; i++){
                fprintf(stderr, "%c", partial_buffer->buffer[i]);
        }
        fprintf(stderr, "--------------------------------------\n");
}

void deletePartialBuffer(partialBuffer_ptr partial_buffer)
{
        if(partial_buffer == NULL) return;
        free(partial_buffer->buffer);
        free(partial_buffer);
}

/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
bufferList newBufferList()
{
        bufferList buffer_list = malloc(sizeof(*buffer_list));
        if (buffer_list == NULL) server_error("create bufferList");
        buffer_list->buffers = malloc(BUFLISTSIZE * sizeof(partialBuffer_ptr));
        memset(buffer_list->buffers, 0, BUFLISTSIZE * sizeof(partialBuffer_ptr));
        if (buffer_list->buffers == NULL) server_error("create bufferList buffers");
        buffer_list->size = BUFLISTSIZE;
        return buffer_list; 
}

void insertBufferList(bufferList buffer_list, partialBuffer_ptr partial_buffer, int index)
{
        while(index >= buffer_list->size){
                int new_size = 2*buffer_list->size;
                int new_size_bytes = new_size * sizeof(partialBuffer_ptr);
                int old_size_bytes = buffer_list->size * sizeof(partialBuffer_ptr);
                fprintf(stderr, "index is %d, expanding buffer list size from %d -> %d\n", index, buffer_list->size, new_size);
                partialBuffer_ptr *new_list = malloc(new_size_bytes);
                memset(new_list, 0, new_size_bytes);
                memcpy(new_list, buffer_list->buffers, old_size_bytes);
                free(buffer_list->buffers);
                buffer_list->buffers = new_list;
                buffer_list->size = new_size;
        }
        buffer_list->buffers[index] = partial_buffer;
}

void deleteFromBufferList(bufferList buffer_list, int index)
{
        fprintf(stderr, "deleting fd: %d from buffer list\n", index);
        partialBuffer_ptr partial_buffer =  buffer_list->buffers[index];
        deletePartialBuffer(partial_buffer);
        buffer_list->buffers[index] = NULL;
}

void clearFromBufferList(bufferList buffer_list, int index) {
        fprintf(stderr, "clearing buffer of fd: %d\n", index);
        partialBuffer_ptr partial_buffer =  buffer_list->buffers[index];
        deletePartialBuffer(partial_buffer);
        partialBuffer_ptr new_partial_buffer =  newPartialBuffer();
        insertBufferList(buffer_list, new_partial_buffer, index);
}

void printBufferList(bufferList buffer_list)
{       
        fprintf(stderr, "%s\n", "printing buffer list");
        for(int i = 0; i < buffer_list->size; i++){
                fprintf(stderr, "fd: %d\n", i);
                partialBuffer_ptr current_buffer = buffer_list->buffers[i];
                printPartialBuffer(current_buffer);
        }
}
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/

char* headerWithAge(char* msg, int* msg_size, int age){
        int bytes_copied =0;
        char input[20];
        sprintf(input, "\r\nAge: %d", age);
        *msg_size += strlen(input);
        char* response = malloc(*msg_size);

        char* header_end = "\r\n";
        char* end_of_header = strstr(msg, header_end);
        int len_first_line = end_of_header-msg;
        
        strncpy(response, msg, len_first_line);/*copy first line*/
        bytes_copied += len_first_line;
        
        strncpy(response + bytes_copied, input, strlen(input));/*copy my line*/
        bytes_copied += strlen(input);
        
        strncpy(response + bytes_copied, msg + len_first_line, *msg_size-bytes_copied); /*copy rest of msg*/
        return response;
}

void sendConnectionEstablishedHeader(int cli_fd) {
        char buffer[100];
        strcpy(buffer, "HTTP/1.1 200 Connection Established\r\n\r\n");
        int n = write(cli_fd, buffer, strlen(buffer));
        if (n < 0)
                server_error("ERROR writing to client");
}
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/

secureNode_ptr newSecureNode(int client, int server) {
        secureNode_ptr node = malloc(sizeof(*node));
        if (node == NULL) server_error("create secureNode_ptr");
        node->client_fd = client;
        node->server_fd = server;
        return node;
}
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/

secureNodeList newSecureNodeList() {
        secureNodeList list = malloc(sizeof(*list));
        if (list == NULL) server_error("create secureNodeList");
        list->nodes = malloc(sizeof(secureNode_ptr));
        memset(list->nodes, 0, sizeof(secureNode_ptr));
        if (list->nodes == NULL) server_error("create secureNodeList nodes");
        list->size = 1;
        list->length = 0;
        return list;
}

void insertSecureNode(secureNodeList node_list, secureNode_ptr node) {
        printf("in insertPartialBuffer\n");
        while ((node_list->size - node_list->length) <= 0) {
                int new_size = 2*node_list->size;
                int new_size_bytes = new_size * sizeof(secureNode_ptr);
                int old_size_bytes = node_list->size * sizeof(secureNode_ptr);
                fprintf(stderr, "expanding secure node list size from %d -> %d\n", node_list->size, new_size);
                secureNode_ptr *new_list = malloc(new_size_bytes);
                memset(new_list, 0, new_size_bytes);
                memcpy(new_list, node_list->nodes, old_size_bytes);
                free(node_list->nodes);
                node_list->nodes = new_list;
                node_list->size = new_size;
        }
        node_list->nodes[node_list->length] = node;
        node_list->length++;
}

void deleteSecureNode(secureNodeList node_list, int sockfd) {
        /*printf("in deleteSecureNode. before deleting:\n");
        printSecureNodeList(node_list);*/
        int delete_index = -1;
        for (int i = 0; i < node_list->length; i++) {
                if (sockfd == node_list->nodes[i]->server_fd || sockfd == node_list->nodes[i]->client_fd) {
                        delete_index = i;
                }
        }
        if (delete_index >= 0) {
                for (int i = delete_index; i < node_list->length-1; i++) {
                        node_list->nodes[i+1]->server_fd = node_list->nodes[i]->server_fd;
                        node_list->nodes[i+1]->client_fd = node_list->nodes[i]->client_fd;
                }
                free(node_list->nodes[node_list->length]);
                node_list->length--;
        }
        /*printf("after deleting:\n");
        printSecureNodeList(node_list);)*/
}

int findNodeBySockfd(secureNodeList node_list, int sockfd) {
        for (int i = 0; i < node_list->length; i++) {
                if (sockfd == node_list->nodes[i]->server_fd)
                        return node_list->nodes[i]->client_fd;
                else if (sockfd == node_list->nodes[i]->client_fd)
                        return node_list->nodes[i]->server_fd;
        }
        return 0;
}

void printSecureNodeList(secureNodeList node_list) {
        printf("Printing secureNodeList\n");
        for (int i = 0; i < node_list->length; i++) {
                printf("Index %d client: %d server: %d\n", i, node_list->nodes[i]->client_fd, node_list->nodes[i]->server_fd);
        }
}
