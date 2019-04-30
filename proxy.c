#include "assert.h"
#include "HttpReqParser.h"
#include "HttpResParser.h"
#include "HttpCache.h"
#include "Buffer.h"

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
void deleteFromServerList(serverNode_ptr *server_list, int fd);
char* isServer(serverNode_ptr server_list, int fd);
/*************************************************************************************************************************************/
int setupListenSocket(int port_number);
int setupServerSocket(HttpReqHead_T header);
void handleNewConnection(int fd, fd_set *master_fd_set, bufferList buffer_list, int *max_sock_ptr);
int handleExistingConnection(int fd, fd_set *master_fd_set, int *max_sock_ptr, bufferList buffer_list, Cache_T cache,
 serverNode_ptr *server_list, secureNodeList secure_list);
void handleDisconnect(int fd, fd_set *master_fd_set, bufferList buffer_list, int *max_sock_ptr,
 Cache_T cache, secureNodeList node_list);
void handleClient(int fd, fd_set *master_fd_set, int *max_sock_ptr, bufferList buffer_list, Cache_T cache,
 serverNode_ptr *server_list, secureNodeList secure_list);
void handleServer(int fd,  fd_set *master_fd_set, bufferList buffer_list, Cache_T cache, char* url,
 serverNode_ptr *server_list, secureNodeList secure_list);
/*************************************************************************************************************************************/
char* headerWithAge(char* msg, int* msg_size, long age);
void sendConnectionEstablishedHeader(int serv_fd);
/*************************************************************************************************************************************/
void add_tokens(bufferList buffer_list, int listen_sock, int length);
int use_tokens(bufferList buffer_list, char *msg, int recv_fd, int msg_size);
void check_cached_messages(Cache_T cache, bufferList buffer_list);

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
                add_tokens(buffer_list, listen_fd, max_sock); 
                check_cached_messages(cache, buffer_list);  
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
                                        handleDisconnect(fd, &master_fd_set, buffer_list, &max_sock, cache, secure_list);
                                }
                        }
                        //print_cache(cache);
                }
                // need to check if fd in server list 
                //delete_expired(cache);
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
                printf("Read 0 bytes\n");
                free(buffer);
                return 0;
        }
        else {
                fprintf(stderr, "Read %d bytes from fd: %d\n", n, fd);
                insertPartialBuffer(buffer_list->buffers[fd], buffer, n);
                printf("successfully added to partial message buffer\n");
                char *url = isServer(*server_list, fd);
                if(url){
                        handleServer(fd, master_fd_set, buffer_list, cache, url, server_list, secure_list);
                }
                else{
                        handleClient(fd, master_fd_set, max_sock_ptr, buffer_list, cache, server_list, secure_list);
                }
        }
        free(buffer);
        return 1;
}

void handleDisconnect(int fd, fd_set* master_fd_set, bufferList buffer_list, int *max_sock_ptr, Cache_T cache, secureNodeList node_list)
{
        fprintf(stderr, "Handle disconnection of fd %d\n", fd);
        deleteFromBufferList(buffer_list, fd);
        printf("out of deleteFromBufferList\n");

        delete_by_sockfd(cache, fd);
        printf("deleted from cache\n");
        close(fd);
        printf("connection with fd %d closed\n", fd);
        if(*max_sock_ptr == fd) 
                *max_sock_ptr -=1;
        printf("going into deleteSecureNode\n");
        deleteSecureNode(node_list, fd);
        printf("out of deleteSecureNode\n");
        FD_CLR(fd, master_fd_set);
        printf("finished with handleDisconnect\n");
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

                        /*write to server if GET request*/
                        if (strcmp(req_header->method, "GET") == 0) {
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

                                printf("writing this to the server:\n");
                                for (int i = 0; i < cache_obj->request_length; i++) {
                                        printf("%c", cache_obj->request_buffer[i]);
                                }
                                int n = write(serv_fd, cache_obj->request_buffer, cache_obj->request_length);
                                if (n < 0)
                                        server_error("ERROR writing to server");
                                fprintf(stderr, "wrote %d bytes to server\n", n);

                        }
                        /*establish tunnel if CONNECT request*/
                        else if (strcmp(req_header->method, "CONNECT") == 0) {
                                printf("CONNECT request received\nsending confirmation back to client\n");
                                sendConnectionEstablishedHeader(fd);
                                printf("Sent header\n");
                                secureNode_ptr node = newSecureNode(fd, serv_fd);
                                insertSecureNode(secure_list, node);
                                printSecureNodeList(secure_list);
                                //delete_by_sockfd(cache, fd); // secure connections should not be cached
                        }
                }
                else if(cache_obj->last_updated != -1 && !is_expired(cache_obj)){
                        fprintf(stderr, "%s\n", "Valid response in cache");
                        fprintf(stderr, "age: %ld, max age: %d\n", (time(NULL) - cache_obj->last_updated), cache_obj->res_header->max_age);
                        int final_msg_size = cache_obj->response_length;
                        cache_obj->last_requested = time(NULL);
                        char *final_msg = headerWithAge(cache_obj->response_buffer, &final_msg_size, (time(NULL) - cache_obj->last_updated));
                        //char *final_msg  = cache_obj->response_buffer;
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
                printf("entering clearFromBufferList\n");
                clearFromBufferList(buffer_list, fd, partial_buffer->length);
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
                clearFromBufferList(buffer_list, fd, partial_buffer->length);
        }
}

void handleServer(int fd, fd_set *master_fd_set, bufferList buffer_list, Cache_T cache, char* url, serverNode_ptr *server_list, secureNodeList secure_list){
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
                //char *final_msg  = cache_obj->response_buffer;
                fprintf(stderr, "final_msg_size: %d\n", final_msg_size);
                for(int *p=(int*)utarray_front(cache_obj->client_fds); p!=NULL; p=(int*)utarray_next(cache_obj->client_fds,p)) {
                        printf("in handleServer for loop\n");

                        int n = use_tokens(buffer_list, final_msg, *p, final_msg_size);
                        if (n >= 0) {
                                printf("wrote %d bytes to fd %d\n", n, *p);
                                delete_from_clientfds(cache_obj, *p);
                        }
                        else {
                                printf("Not enough tokens to write to fd %d\n", *p);
                        }
                }
                free(final_msg);
                close(fd);
                deleteFromServerList(server_list, fd);
                printf("%s\n", "printing server list ");
                printServerList(*server_list);
                FD_CLR(fd, master_fd_set);
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
                clearFromBufferList(buffer_list, fd, partial_buffer->length);
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

void deleteFromServerList(serverNode_ptr *server_list, int fd){
    serverNode_ptr temp = *server_list;
    serverNode_ptr prev; 
    if (temp != NULL && temp->fd == fd){ 
        *server_list = temp->next;   // Changed head 
        free(temp);               // free old head 
        return; 
    } 
    while (temp != NULL && temp->fd != fd) 
    { 
        prev = temp; 
        temp = temp->next; 
    } 
    if (temp == NULL) return; 
  
    prev->next = temp->next; 
  
    free(temp);
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

char* headerWithAge(char* msg, int* msg_size, long age){
        int bytes_copied =0;
        char input[100];
        sprintf(input, "\r\nAge: %ld", age);
        //printf("%s, %d\n", input, strlen(input));
        *msg_size += strlen(input);
        char* response = malloc(*msg_size);

        char* first_line_end = "\r\n";
        char* end_of_first_line = strstr(msg, first_line_end);
        int len_first_line = end_of_first_line-msg;

        /*copy first line*/
        memcpy(response, msg, len_first_line);
        bytes_copied += len_first_line;
        /*copy my line*/
        memcpy(response + bytes_copied, input, strlen(input));
        bytes_copied += strlen(input);
        /*copy rest of msg*/
        memcpy(response + bytes_copied, msg + len_first_line, *msg_size-bytes_copied);
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
        printf("in insertSecureNode\n");
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
        printf("in deleteSecureNode. length = %d. before deleting:\n", node_list->length);
        printSecureNodeList(node_list);
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
                free(node_list->nodes[node_list->length-1]);
                node_list->length--;
        }
        printf("after deleting:\n");
        printSecureNodeList(node_list);
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

/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/
/*************************************************************************************************************************************/

void add_tokens(bufferList buffer_list, int listen_sock, int length) {
        printf("in add_tokens\n");
        for (int i = listen_sock+1; i < length; i++) {
                if (buffer_list->buffers[i] != NULL) {
                        Bucket_ptr bucket = buffer_list->buffers[i]->bucket;
                        printf("%d not NULL, has %d tokens\n", i, buffer_list->buffers[i]->bucket->tokens);
                        struct timeval now;
                        gettimeofday(&now, NULL);
                        int now_time = 1000000*now.tv_sec + now.tv_usec;
                        int last_time = 1000000*bucket->last_updated.tv_sec + bucket->last_updated.tv_usec;
                        printf("time now: %d\n last update: %d\n", now_time, last_time);
                        int between_updates = now_time - last_time;
                        printf("time since last update: %d\n", between_updates);
                        int num_tokens = (between_updates * bucket->token_rate)/1000000;
                        if (bucket->tokens + num_tokens > bucket->bucket_size) {
                                bucket->tokens = bucket->bucket_size;
                                printf("fd %d has a full bucket\n", i);
                        }
                        else {
                                bucket->tokens += num_tokens;
                                printf("added %d tokens to fd %d. bucket contains %d tokens\n", num_tokens, i, bucket->tokens);
                        }
                        gettimeofday(&bucket->last_updated, NULL);
                }
        }
}

int use_tokens(bufferList buffer_list, char *msg, int recv_fd, int msg_size) {
        printf("fd %d has %d tokens\n", recv_fd, buffer_list->buffers[recv_fd]->bucket->tokens);
        if (buffer_list->buffers[recv_fd]->bucket->tokens >= msg_size) {
                int n = write(recv_fd, msg, msg_size);
                if (n < 0)
                        server_error("ERROR writing to client");
                buffer_list->buffers[recv_fd]->bucket->tokens -= msg_size;
                printf("after writing, fd %d has %d tokens\n", recv_fd, buffer_list->buffers[recv_fd]->bucket->tokens);
                return n;
        }
        else
                return -1;
}

void check_cached_messages(Cache_T cache, bufferList buffer_list) {
        for (int i = 0; i < cache->num_obj; i++) {
                if (cache->arr[i]->response_buffer != NULL) {
                        for(int *p=(int*)utarray_front(cache->arr[i]->client_fds); p!=NULL; p=(int*)utarray_next(cache->arr[i]->client_fds,p)) {
                                use_tokens(buffer_list, cache->arr[i]->response_buffer, *p, cache->arr[i]->response_length);
                        }
                }
        }
}
