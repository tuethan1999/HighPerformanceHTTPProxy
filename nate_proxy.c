// #include <string.h>
// #include <stdlib.h>
// #include <stdio.h>
#include "assert.h"
#include "HttpReqParser.h"
#include "HttpResParser.h"
#include "HttpCache.h"
#include "HandleMessage.h"
#include "ServerHandler.h"

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

#define BUFFSIZE 4096

typedef struct Serv_request{
        char *buffer;
        int child_fd;
} *Serv_request;

int SetupProxyServer(int port_number);
Serv_request ProxyServer( int parent_fd);
HttpReqHead_T parseClientRequest(Serv_request serv_r);
void handleClient(Cache_T cache, msg_buffer *buffer_obj, int fd, ServerListNode *server_list);
void handleServer(Cache_T cache, msg_buffer *buffer_obj, int fd);


int main(int argc, char *argv[])
{
        if(argc != 2){
                fprintf(stderr, "usage %s <port>\n", argv[0]);
                exit(1);
        }
        int c_portnum = atoi(argv[1]);
        int listen_fd = SetupProxyServer(c_portnum);
        
        int rv;
        struct timeval timeout;
        fd_set master_fd_set, copy_fd_set;
        FD_ZERO(&master_fd_set);
        FD_ZERO(&copy_fd_set);
        FD_SET(listen_fd, &master_fd_set);
        int max_sock = listen_fd;

        msg_buffer buf_array[20]; // TODO discuss size of the array
        Cache_T cache = new_cache();
        ServerListNode *server_list = create_server_list();

        //char *server_response;
        while(1){
                //sleep(2);
                
                FD_ZERO(&copy_fd_set);
                memcpy(&copy_fd_set, &master_fd_set, sizeof(master_fd_set));
                timeout.tv_sec = 1; // does this have to be in the while loop?
                timeout.tv_usec = 1000; // does this have to be in the while loop?
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
                                if (FD_ISSET(fd, &copy_fd_set)) {
                                        printf("incoming message from fd %d\n", fd);
                                        handle_incoming_message(buf_array, fd, listen_fd, &master_fd_set, &max_sock);
                                        //print_partial_msg_buffer(buf_array, 20);
                                        char *url = is_server(fd, server_list);
                                        if (url == NULL) {
                                                printf("message from client\n");
                                                handleClient(cache, buf_array, fd, server_list);
                                        }
                                        else {
                                                printf("message from server\n");
                                                handleServer(cache, buf_array, fd);
                                        }

                                }
                        }
                }

                //Serv_request serv_r = ProxyServer(listen_fd);
                //HttpReqHead_T req_h = parseClientRequest(serv_r);
                //print_http_req_head(req_h);
        }  
        return 0;
}

int SetupProxyServer(int port_number)
{
        int parent_fd, opt_val;
        struct sockaddr_in server_addr;

        /*socket: create parent socket*/
        parent_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(parent_fd < 0){ error("ERROR opening socket!"); }

        /*Debugging trick to avoid "ERROR on binding: Address already in use"*/
        opt_val =1;
        setsockopt(parent_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt_val, sizeof(int));

        /*build server's internal address*/
        bzero((char *) &server_addr, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        server_addr.sin_port = htons((unsigned short)port_number);
        
        /*binding: associating the server with a port*/
        if(bind(parent_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){ error("ERROR on binding!"); }
        
        /*listening: make this socket ready to accept connection requests*/
        if(listen(parent_fd, 5) < 0 ) { error("ERROR on listen!"); }

        return parent_fd;
}

Serv_request ProxyServer(int parent_fd){
        int child_fd, client_len, n;
        char *host_addr;
        struct hostent *host_info;
        struct sockaddr_in client_addr;
        Serv_request s_req = malloc(sizeof(*s_req));
        s_req->buffer = malloc(sizeof(char)* BUFFSIZE);
        client_len = sizeof(client_addr);
        /* accept: wait for a connection request!*/
        child_fd = accept(parent_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_len);
        if(child_fd < 0){ error("ERROR on accept\n"); }
        
        /* DNS: find who sent the message*/
        host_info = gethostbyaddr((const char *)&client_addr.sin_addr.s_addr, sizeof(client_addr.sin_addr.s_addr), AF_INET);
        if(host_info == NULL){ error("ERROR on gethostbyaddr\n"); }
        host_addr = inet_ntoa(client_addr.sin_addr);
        if(host_addr == NULL){ error("ERROR on inet_ntoa\n"); }
        fprintf(stdout, "server established connection with %s (%s)\n", host_info->h_name, host_addr);

        /*read: read input string from the client*/
        bzero(s_req->buffer, BUFFSIZE);
        n = read(child_fd, s_req->buffer, BUFFSIZE);
        if(n < 0){ error("ERROR reading from socket\n"); }

        s_req->child_fd = child_fd;
        return s_req;
}

HttpReqHead_T parseClientRequest(Serv_request serv_r)
{       
        HttpReqHead_T request_header = new_req_head();
        parse_http_req(request_header, serv_r->buffer, BUFFSIZE);
        return request_header;
}

void handleClient(Cache_T cache, msg_buffer* buf_array, int fd, ServerListNode *server_list){
        printf("in handleClient\n");
        HttpReqHead_T req_header = new_req_head();
        if(parse_http_req(req_header, buf_array[fd].buffer, buf_array[fd].length))
        {
                CacheObj_T cache_obj = find_by_url(cache, req_header->url);
                if(cache_obj == NULL){ 
                        cache_obj = new_cache_object();
                        strcpy(cache_obj->url, req_header->url);
                        memcpy(cache_obj->request_buffer, buf_array[fd].buffer, buf_array[fd].length);
                        cache_obj->request_length = buf_array[fd].length;
                        cache_obj->last_requested = time(NULL);
                        utarray_push_back(cache_obj->client_fds, &fd);
                        insert_into_cache(cache, cache_obj);
                        printf("about to enter inititate_server_connection\n");
                        int serv_fd = inititate_server_connection(req_header, server_list);
                }
                else{
                        /*write back to client*/
                        /*remove client from clientfd*/
                }
        }
        else{
                free_req_head(req_header);    
        }
}

/*void handleServer(Cache_T cache, msg_buffer* buf_array, int fd){
        HttpResHead_T res_header = new_res_head();
        if(parse_http_res(res_header, buf_array[fd].buffer, buf_array[fd].length))
        {
                /*get url from file descriptor
                CacheObj_T cache_obj = find_by_url(cache, req_header->url);
                assert(cache_obj == NULL);
                cache_obj->response_buffer = NULL;
        cache_obj->response_length = -1;
        cache_obj->last_updated = -1;
                /*add age to header*/
                /*forward to client*
        }
        free_res_head(res_header);
}*/
