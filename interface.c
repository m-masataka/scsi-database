/*
non-blocking connection.
epoll and threading
*/

#include<stdlib.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<netdb.h>
#include<string.h>
#include<sys/sendfile.h>
#include<sys/epoll.h>
#include<unistd.h>
#include<errno.h>

#include "lib.h"
#include "jsmn.h"
#include "util_sgio.h"

#define MAXEPOLLSIZE 1000
#define BACKLOG 200

int fd;

typedef struct KeyValue {
    char *key;
    int  key_size;
    char *value;
    int  value_size;
    struct KeyValue *next;
} KeyValue;

int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
			strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

void free_kv(struct KeyValue *kv) {
    free(kv->key);
    free(kv->value);
    free(kv->next);
    free(kv);
}

struct KeyValue *parse_json(unsigned char *js) {
	jsmn_parser p;
	jsmntok_t t[100] = {0};
	int r;
	jsmn_init(&p);
	r = jsmn_parse(&p, js, strlen(js), t, sizeof(t)/sizeof(t[0]));
	if( r < 0) {
		printf("Failed to parse JSON: %d\n", r);
		return NULL;
	}
	if (r < 1 || t[0].type != JSMN_OBJECT) {
		printf("Object expected\n");
		return NULL;
	}
	
	struct KeyValue *kv;
	int cur;
	int i;
	for ( i = 1; i < 5; i += 2 ){
		if (strncmp(js + t[i].start , "key", t[i].end -t[i].start) == 0) {
			// KEY
			cur = i + 1;
    			kv = malloc(sizeof(struct KeyValue));
			kv->key_size = t[cur].end -t[cur].start;
    			kv->key = malloc(sizeof(char) * (t[cur].end - t[cur].start));
			memset(kv->key, 0, t[cur].end - t[cur].start + 1);
			strncpy(kv->key, js + t[cur].start, t[cur].end -t[cur].start);
			printf("cur : %d\n", i);
		} else if (strncmp(js + t[i].start , "value", t[i].end -t[i].start) == 0) {
			// VALUE
			cur = i + 1;
    			kv->value = (char *)malloc(sizeof(char) * (t[cur].end - t[cur].start));
			kv->value_size = t[cur].end -t[cur].start;
			memset(kv->value, 0, t[cur].end - t[cur].start + 1);
			strncpy(kv->value, js + t[cur].start, t[cur].end -t[cur].start);
		}
	}
	return kv;
}

int send_msg(int sockfd, char *msg) {
	int len;
	len = strlen(msg);
	if ( send(sockfd, msg, len, 0) == -1 ){
		fprintf(stderr, "error: writing.");
	}
	printf("sendmsg %s, %d\n", msg, len);
	return len;
}

void http(int sockfd){
	char buf[1024];
	memset( &buf[0] , 0x00 , sizeof(buf) );
	if (read(sockfd, buf, 1024) <= 0 ) {
		fprintf(stderr, "error: reading a request.\n");
	} else {
		struct Request *req = parse_request(buf);
		if (req) {
			if (req->method == PUT ) {
				struct KeyValue *kv;
				kv = parse_json(req->body);
				printf("%s\n",kv->key);
				put(kv->key, atof(kv->value), kv->key_size, sizeof(float));
				float f = find(kv->key, kv->key_size);
				debug(0);
				printf("result %f\n", f);
				send_msg(sockfd, "put sucess\n");
				free_kv(kv);
			} else if ( req->method == GET ) {
				
			} else {
				send_msg(sockfd, "Unsuported Method\n");
			}
		}
		free_request(req);
	}
	//close(sockfd);
}

int set_non_blocking(int sockfd) {
	int flags;
	flags = fcntl(sockfd, F_GETFL, 0);
	if(flags < 0){
		perror("fnctl");
		return -1;
	}
	if(fcntl(sockfd, F_SETFL, flags)){
		perror("fnctl");
		return -1;
	}
	return 0;
}

int main(int argc, char* argv[]) {
	int status;
	int sockfd, new_fd, kdpfd, nfds, n, curfds;
	struct addrinfo hints;
	struct addrinfo *servinfo; 				// will point to the results 
	struct addrinfo *p;
	struct sockaddr_storage client_addr;
	struct epoll_event ev;
	struct epoll_event *events;
	socklen_t addr_size;

	unsigned short servPort;
	unsigned int clitLen;
	char hbuf [NI_MAXHOST];
	char sbuf [NI_MAXSERV];

	if((fd = open(DEVICE_NAME,O_RDWR))<0){
		printf("device file opening failed\n");
		return  1;
	}

	if ( argc != 2) {
		fprintf(stderr, "argument count mismatch error.\n");
		exit(EXIT_FAILURE);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if((status = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0 ) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		return 2;
	}

	for (p = servinfo; p != NULL; p = p->ai_next ) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}
	
		set_non_blocking(sockfd);		
		if ((bind(sockfd, p->ai_addr, p->ai_addrlen)) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}
		break;
	}

	if(p == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo);

	if (listen(sockfd, BACKLOG) < 0) {
		perror("listen() failed.");
		exit(EXIT_FAILURE);
	}

	printf("server: waiting for connections...\n");

	kdpfd = epoll_create(MAXEPOLLSIZE);
	ev.events = EPOLLIN|EPOLLET;
	ev.data.fd = sockfd;

	if(epoll_ctl(kdpfd, EPOLL_CTL_ADD, sockfd, &ev) < 0)
	{
		fprintf(stderr, "epoll set insert error.");
		return -1;
	} else {
		printf("success insert listening socket into epoll.\n");
	}
	events = calloc (MAXEPOLLSIZE, sizeof ev);
	curfds = 1;
	while(1) 
	{ //loop for accept incoming connection

		nfds = epoll_wait(kdpfd, events, curfds, -1);
		if(nfds == -1)
		{
			perror("epoll_wait");
			break;
		}		
		printf("nfds %d, sockf  %d \n", events[0].data.fd, sockfd);
		for (n = 0; n < nfds; ++n)
		{
			if(events[n].data.fd == sockfd){
				addr_size = sizeof client_addr;
				new_fd = accept(events[n].data.fd, (struct sockaddr *)&client_addr, &addr_size);
				if (new_fd == -1)
				{
					if((errno == EAGAIN) ||
						 (errno == EWOULDBLOCK))
					{
						break;
					}
					else
					{
						perror("accept");
						break;
					}
				}
				printf("server: connection established...\n");
				int rc;
				rc = getnameinfo((struct sockaddr *)&client_addr, addr_size, hbuf, sizeof(hbuf), sbuf, sizeof(sbuf),0);
				printf("shub[%s], hbuf[%s]\n", sbuf, hbuf);
				set_non_blocking(new_fd);
				ev.events = EPOLLIN|EPOLLET;
				ev.data.fd = new_fd;
				if(epoll_ctl(kdpfd,EPOLL_CTL_ADD, new_fd, &ev)<0)
				{
					printf("Failed to insert socket into epoll.\n");
				}
				curfds++;
			} else {
				http(events[n].data.fd);
				/*if(send(events[n].data.fd, "Hello, world!\n", 14, 0) == -1)
				{
					perror("send");
					break;
				}*/
				epoll_ctl(kdpfd, EPOLL_CTL_DEL, events[n].data.fd, &ev);
				curfds--;
				close(events[n].data.fd);
			}
		}
	}
	free(events);
	close(sockfd);			
	return 0;
}
