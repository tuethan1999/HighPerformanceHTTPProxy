// #include <string.h>
// #include <stdlib.h>
// #include <stdio.h>
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

#define BUFFSIZE 4096

typedef struct Serv_request{
        char *buffer;
        int child_fd;
} *Serv_request;

void error(const char *msg){
  perror(msg);
  exit(1);}
int SetupProxyServer(int port_number);
Serv_request ProxyServer( int parent_fd);
HttpReqHead_T parseClientRequest(Serv_request serv_r);


int main(int argc, char *argv[])
{
        if(argc != 2){
                fprintf(stderr, "usage %s <port>\n", argv[0]);
                exit(1);
        }
        int c_portnum = atoi(argv[1]);
        int parent_fd = SetupProxyServer(c_portnum);
        //char *server_response;
        while(1){
                sleep(2);
                Serv_request serv_r = ProxyServer(parent_fd);
                HttpReqHead_T req_h = parseClientRequest(serv_r);
                print_http_req_head(req_h);
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
        if(listen(parent_fd, 5) < 0 ){ error("ERROR on listen!"); }

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
        return s_req;}

HttpReqHead_T parseClientRequest(Serv_request serv_r)
{       
        HttpReqHead_T request_header = new_req_head();
        parse_http_req(request_header, serv_r->buffer, BUFFSIZE);
        return request_header;
}
