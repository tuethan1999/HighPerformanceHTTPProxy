/*
 * d1.c - A simple TCP Proxy server
 * usage - ./a1.out <port>
 */
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


#define CACHE_SIZE 10
#define DEFAULT_MAX_AGE 3600
#define TEN_MB 100000000
#define PORT_NUM 80
#define BUFFSIZE 4096

typedef struct Cl_request{
	char *host;
	int port_number;
	char *http_request;
	char *path;
} *Cl_request;

typedef struct CacheBlock{
	time_t last_accessed;
	char *object_name;
	char *object;
	int max_age;
	time_t first_added;
	int port_number;
} *CacheBlock;

typedef struct  Cache{
	CacheBlock block[CACHE_SIZE];
	int elems;

} *Cache;


typedef struct Serv_request{
	char *buffer;
	int child_fd;
} *Serv_request;

void replyClient(char *http_response, int child_fd);
int SetupProxyServer(int port_number);
char *ProxyClient(Cl_request cl_req );
Serv_request ProxyServer( int parent_fd);
int setupProxyServer(int port_number);
void printBinaryFile(char *binary_file, int size);
void error(const char *msg){
  perror(msg);
  exit(1);}
Cl_request parseClientRequest(Serv_request serv_r);
char * checkCache(Cache proxy_cache, Cl_request client_request);
int extractMaxAge(char *server_response);
void removeLeastAccessed(Cache proxy_cache);
void removeBlockOfCache(Cache proxy_cache, int index);
int checkStale(Cache proxy_cache, int index);
void addToCache(Cache proxy_cache, char *server_response, char *host, int port_number);
void addAge(Cache proxy_cache, int index);
void printProxyCache(Cache proxy_cache);
int ResponseLength(char *buffer);

int main(int argc, char *argv[]){
	
	if(argc != 2){
		fprintf(stderr, "usage %s <port>\n", argv[0]);
		exit(1);
	}
	int c_portnum = atoi(argv[1]);
	int parent_fd = SetupProxyServer(c_portnum);
	char *server_response;
	Cache PROXY_CACHE = malloc(sizeof(*PROXY_CACHE) * CACHE_SIZE);
	while(1){
		sleep(2);
		Serv_request serv_r = ProxyServer(parent_fd);
		Cl_request cl_r = parseClientRequest(serv_r);
		server_response = checkCache(PROXY_CACHE, cl_r);
		if( server_response  == NULL ){ 
			server_response = ProxyClient(cl_r); 
			addToCache(PROXY_CACHE, server_response, cl_r->path, cl_r->port_number);
		}
		replyClient(server_response, serv_r->child_fd);
		printProxyCache(PROXY_CACHE);
	}  
	return 0;
}








/***********************************************************/
/* CACHE FUNCTIONALITY!! */
char * checkCache(Cache proxy_cache, Cl_request client_request){
	printf("The searched object here is: %s\n",client_request->path);
	int foundCache = 0;
	int index = 0;
	int total_elems = proxy_cache->elems;
	char *response = client_request->path;

	while(foundCache == 0 && index < total_elems){
		
		if(strcmp(proxy_cache->block[index]->object_name, response) == 0){ 
			foundCache = 1; 
		} 
		index += 1;
	}
	index -= 1;
	if(foundCache){
		printf("FOUND IT IN THE CACHE!!!\n");
		int old_data = checkStale(proxy_cache, index);
		if(old_data == 0){
			proxy_cache->block[index]->last_accessed = time(NULL);
			addAge(proxy_cache, index);
			return proxy_cache->block[index]->object;
		}else{
			printf("IT IS STALE DATA!!!\n");
			removeBlockOfCache(proxy_cache, index);
			return NULL;
		}
	}else{
		printf("CANNNNNT FIND IT IN THE CACHE!!!\n");
		int elements = proxy_cache->elems;
		if(elements >= CACHE_SIZE){ removeLeastAccessed(proxy_cache);
			printf("CACHE IS FULL \n");
		}
		return NULL;
		
	}
	return NULL;
}

void addToCache(Cache proxy_cache, char *server_response, char *path , int port_number){
	//printf("Add to cache:\n");
	CacheBlock cache = malloc(sizeof(*cache));
	cache->last_accessed = time(NULL);
	cache->object_name = malloc(sizeof(char) * 1024);
	memcpy(cache->object_name, path, 1024);
	int length = ResponseLength(server_response);
	cache->object = malloc(sizeof(char)* length * 2);
	memcpy(cache->object, server_response, sizeof(char)* length * 2);
	cache->max_age = extractMaxAge(server_response);
	cache->first_added = time(NULL);
	cache->port_number = port_number;
	proxy_cache->block[proxy_cache->elems] = cache;
	proxy_cache->elems += 1;
//	printf("ENDDDD Add to cache:\n");
}

void removeBlockOfCache(Cache proxy_cache, int index){
	//printf("REMOVING BLOCK OF CACHE!!! \n");
//	printf("??????????????????????????????\n");
	int last_position = proxy_cache->elems - 1;
	proxy_cache->block[index] = proxy_cache->block[last_position];
	proxy_cache->block[last_position] = NULL;
	proxy_cache->elems -= 1; 
//	printf("??????????????????????????????\n");
}

void removeLeastAccessed(Cache proxy_cache){
//	printf("REACHED REMOVE LEAST ACCESSED FUNNCTION\n\n");
	int index = 0;
	int last_accessed = time(NULL);
	for (int i = 0; i < CACHE_SIZE; i++){
		int current_last_accessed = proxy_cache->block[i]->last_accessed;
		if(last_accessed > current_last_accessed){
			last_accessed = current_last_accessed;
			index = i;
		}
	}
	proxy_cache->block[index] = proxy_cache->block[CACHE_SIZE - 1];
	proxy_cache->block[CACHE_SIZE - 1] = NULL;
	proxy_cache->elems -= 1;

}



/***********************************************************/
/*UTILITY FUNCTIONS FOR CACHE*/
int extractMaxAge(char *server_response){

	char buffer[BUFFSIZE]= "hello" ;
	memcpy(buffer, server_response, BUFFSIZE + 1);
	char *pch = strtok(buffer, "\n");
	char temp_age[100] = "";
	int found = -1;
	while(pch != NULL && found == -1){
		if(strstr(pch, "Cache-Control") != NULL ||
			strstr(pch, "Cache-control") != NULL ){
			char *age = strstr(pch, "max-age=");
			if(age != NULL){
				memcpy(temp_age, age, 100);
				char *c_age = strtok(temp_age, "=");
				c_age = strtok(NULL, "=");
				found = atoi(c_age);
			}
		}
		pch = strtok(NULL, "\n");
	}
	if(found == -1){
		return DEFAULT_MAX_AGE ;
	}else{
		return found;
	}
}


int checkStale(Cache proxy_cache, int index){
	CacheBlock block = proxy_cache->block[index];
	time_t last_accessed = block->last_accessed;
	int max_age = block->max_age;
	time_t current_time = time(NULL);

	if ( last_accessed + max_age <= current_time ) { return 1; } else { return 0; }
}

void addAge(Cache proxy_cache, int index){

	time_t age_number = time(NULL) - proxy_cache->block[index]->first_added;
	char *response = proxy_cache->block[index]->object;
	char *result = strstr(response, "Age");
	char age[100] = "";
	int position = 0;
	sprintf(age, "Age: %lu\n", age_number);

	int length = ResponseLength(response);
	char *new_response = malloc(sizeof(char) * length*2);
	if(result != NULL){
		position = result - response;
		strncpy(new_response, response, position);
		new_response[position] = '\0';
		int idx = strcspn (response + position, "\n");
		strcat(new_response, age);
		int mini_length = strlen(new_response);
		memcpy(new_response + mini_length, response+position+idx+1, length*2 - mini_length);
		//strcat(new_response, response + position + idx + 1);
	} else{	

		result = strstr(response, "Server:");
		if (result == NULL) { result = strstr(response, "Date:");}
		if (result == NULL) { result = strstr(response, "Location:");}
		position = result - response;
		strncpy(new_response, response, position);
		new_response[position] = '\0';
		strcat(new_response, age);
		int mini_length = strlen(new_response);
		memcpy(new_response + mini_length, response+position, length*2 - mini_length);
	//	strcat(new_response, response + position);
	}
	proxy_cache->block[index]->object = NULL;
	if(response != NULL){
		free(response);
	}
	proxy_cache->block[index]->object = new_response;
}

void printProxyCache(Cache proxy_cache){
	
	printf("%s\n\n\n","PRINTING ALL THE ELEMENTS IN THE CACHE!!");
	printf("********************************************************\n" );
	int elements = proxy_cache->elems;
	for(int i=0; i < elements; i++){
		if(proxy_cache->block[i] == NULL){
			printf("PROXY IS NULL!! %s\n");
		}else{
			printf("Host name:%s\nMax-Age: %d\nPort_number: %d\nFirst Added: %lu\nLast Accessed: %lu\n\n",
			proxy_cache->block[i]->object_name, proxy_cache->block[i]->max_age, proxy_cache->block[i]->port_number, proxy_cache->block[i]->first_added, proxy_cache->block[i]->last_accessed);
		}
	}
	printf("********************************************************\n\n\n" );
}


/***********************************************************/


/* PROXY SERVER FUNCTIONALITY!! */
void replyClient(char *http_response, int child_fd){
	//printf("********************************************************\n" );
//	fprintf(stderr, "RETURNING THE CONTENT TO THE CLIENT%s\n",http_response);
	int n = write(child_fd, http_response, TEN_MB);
//	close(child_fd);
	//printf("********************************************************\n" );
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
	child_fd = accept(parent_fd, (struct sockaddr *)&client_addr, &client_len);
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

int SetupProxyServer(int port_number){
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

	return parent_fd;}	

int ResponseLength(char *buf){
		char buffer[BUFFSIZE] = " ";
		memcpy(buffer, buf, BUFFSIZE);
        char *pch = strtok(buffer, "\n");
        while(pch != NULL && strcmp(pch, "Content-Length") != 0){
                pch = strtok(NULL, ":\n");
        }
        if(pch == NULL){
        	printf("NO Content-Length FOUND%s\n");
        	return BUFFSIZE;
        }else{
        	char *h_length = strtok(NULL, ":\n");
        	int len = atoi(h_length);
        	return len;
 	   }
	}

char * ProxyClient(Cl_request cl_req){
	int sockfd, portno, bytes_read;
	struct sockaddr_in serveraddr;
	struct hostent *server;	
	char *host_name;
	char buf[BUFFSIZE] = " ";
	portno = cl_req->port_number;
	host_name = cl_req->host;

	/*create a socket connection*/
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){ error("ERROR opening the socket"); }
	
	/*get the hostname from the server's DNS entry*/
	server = gethostbyname(host_name);
	if(server == NULL){
		fprintf(stderr, "ERROR, no such host as %s\n", host_name);
		exit(0);
	}
	/*build server's internet address*/
	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(portno);
	bcopy((char *)server->h_addr, &serveraddr.sin_addr.s_addr, server->h_length);
	
	/*connect: create the connection with the server*/
	if(connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0){ error("ERROR connecting!"); }

	/*send the body to the server */
	bzero(buf, BUFFSIZE);
	memcpy(buf, cl_req->http_request, BUFFSIZE);
	bytes_read = write(sockfd, buf, BUFFSIZE);

	/*recieve the reply from the server*/
	bytes_read =  read(sockfd, buf, BUFFSIZE);
	int length = ResponseLength(buf);
	char *response = malloc(sizeof(char) * length * 2);
	if(bytes_read > 0){
		memcpy(response, buf, bytes_read);
	}
	int total_bytes = bytes_read;
	while(bytes_read > 0){
		bytes_read = read(sockfd, response + total_bytes, length);	
		total_bytes += bytes_read;
	}	
	close(sockfd);
	return response;
}

void printBinaryFile(char *binary_file, int size){
	for(int i = 0; i < size; i++){
		printf("%c\n",binary_file[i]);
	}}




Cl_request parseClientRequest(Serv_request serv_r){
	char* host;
	char p_number[16] = "80";
	int port_number = PORT_NUM ;
	char header[256] = " ";
	char temp_header[256] = " ";
	char http_request[BUFFSIZE];
	Cl_request cl_r = malloc(sizeof(*cl_r));

	strcpy(http_request, serv_r->buffer);
	char *pch = strtok(http_request, "\n");
	memcpy(header, pch, 256);

	cl_r->path = malloc(sizeof(char) * 1024);
	memcpy(cl_r->path, header, 256);
	while(strcmp(pch, "Host")){ pch = strtok(NULL, ":\n"); }
	host = strtok(NULL, ": \n");
	
	for(int i = 0; i <= strlen(host); i++){
		if(host[i] == '\r'){
			host[i] = '\0';
		}
	}

	char *head = strtok(header, ":");
	head = strtok(NULL, ":");
	head = strtok(NULL, ":");
	
	int index = 0;
	while(head != NULL && head[index] >= '0' && head[index] <= '9'){
		p_number[index] = head[index];
		index += 1;
	}
	if(index){ port_number = atoi(p_number); }
	

	cl_r->host = malloc(sizeof(char) * 1024);
	memcpy(cl_r->host, host, 1024);
	cl_r->port_number = port_number;
	cl_r->http_request = malloc(sizeof(char) * BUFFSIZE);
	memcpy(cl_r->http_request, serv_r->buffer, BUFFSIZE);
	return cl_r;	
}	

/***********************************************************/