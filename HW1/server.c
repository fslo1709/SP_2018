//B06902102
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>

#define ERR_EXIT(a) { perror(a); exit(1); }

typedef struct {
	int id;
	int amount;
	int price;
} Item;

typedef struct {
    char hostname[512];  // server's hostname
    unsigned short port;  // port to listen
    int listen_fd;  // fd to wait for a new connection
} server;

typedef struct {
    char host[512];  // client's host
    int conn_fd;  // fd to talk with client
    char buf[512];  // data sent by/to client
    size_t buf_len;  // bytes used by buf
	int wait_for_read;
    // you don't need to change this.
	int item;
    int wait_for_write;  // used by handle_read to know if the header is read or not.
} request;

Item bid;				//input read of each item
int blockedlist[20];
server svr;  // server
request* requestP = NULL;  // point to a list of requests
int maxfd;  // size of open file descriptor table, size of request list

const char* accept_read_header = "ACCEPT_FROM_READ";
const char* accept_write_header = "ACCEPT_FROM_WRITE";

// Forwards

static void init_server(unsigned short port);
// initailize a server, exit for error

static void init_request(request* reqP);
// initailize a request instance

static void free_request(request* reqP);
// free resources used by a request instance

static int handle_read(request* reqP);
// return 0: socket ended, request done.
// return 1: success, message (without header) got this time is in reqP->buf with reqP->buf_len bytes. read more until got <= 0.
// It's guaranteed that the header would be correctly set after the first read.
// error code:
// -1: client connection error

int main(int argc, char** argv) {
    int i, ret;

    struct sockaddr_in cliaddr;  // used by accept()
    int clilen;

    int conn_fd;  // fd for a new connection with client
    int file_fd;  // fd for file that we open for reading
    char buf[512];
    char secbuf[512];
    int buf_len;

    // Parse args.
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    // Initialize server
    init_server((unsigned short) atoi(argv[1]));

    // Get file descripter table size and initialize request table
    maxfd = getdtablesize();
    requestP = (request*) malloc(sizeof(request) * maxfd);
    if (requestP == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (i = 0; i < maxfd; i++) {
        init_request(&requestP[i]);
    }
    requestP[svr.listen_fd].conn_fd = svr.listen_fd;
    strcpy(requestP[svr.listen_fd].host, svr.hostname);
    
    // Loop for handling connections
    fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);

    fd_set LARGEST;
    FD_ZERO(&LARGEST);
    FD_SET(svr.listen_fd, &LARGEST);
    maxfd = svr.listen_fd;		//max we can use for the select
    
	struct timeval timout;
	timout.tv_sec = 5;
	timout.tv_usec = 0;			// time used for waiting

	struct flock fl;
	// fd_set checked;
	// FD_ZERO(&checked);			//things we have checked

    while (1) {
        // TOO: Add IO multiplexing

    	fd_set TEMPSET;
    	FD_ZERO(&TEMPSET);
    	TEMPSET = LARGEST;

    	int readtime = select(maxfd + 1, &TEMPSET, NULL, NULL, &timout);

		if (FD_ISSET(svr.listen_fd, &TEMPSET)) {
			// Check new connection
			// fprintf(stderr, "%d ", readtime);
			readtime--;
		    clilen = sizeof(cliaddr);
		    conn_fd = accept(svr.listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
		    if (conn_fd < 0) {
		        if (errno == EINTR || errno == EAGAIN) continue;  // try again
		        if (errno == ENFILE) {
		            (void) fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
		            continue;
		        }
		        ERR_EXIT("accept")
		    }
		    requestP[conn_fd].conn_fd = conn_fd;
		    strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
		    fprintf(stderr, "getting a new request... fd %d from %s\n", conn_fd, requestP[conn_fd].host);
		    maxfd = (conn_fd > maxfd) ? conn_fd : maxfd;
		    FD_SET(conn_fd, &LARGEST);
		    // fcntl(conn_fd, F_SETFL, O_NONBLOCK);
		    // FD_SET(conn_fd, &TEMPSET);
		}

		for (int CFD = svr.listen_fd+1; CFD <= maxfd && readtime > 0; ++CFD) {
			if (FD_ISSET(CFD, &TEMPSET)) {
				readtime--;
				ret = handle_read(&requestP[CFD]); // parse data from client to requestP[CFD].buf
				if (ret < 0) {
					fprintf(stderr, "bad request from %s\n", requestP[CFD].host);
					continue;
				}
				// if (ret == 0)
					// fprintf(stderr, "0");
	#ifdef READ_SERVER
			int file_fd = open("item_list", O_RDONLY);
			int ind;
			fl.l_type = F_RDLCK;
			fl.l_whence = SEEK_CUR;
			fl.l_start = 0;
			fl.l_len = sizeof(Item);
			sscanf(requestP[CFD].buf, "%d", &ind);
			if (sprintf(buf, "Error");lseek(file_fd, (ind-1)*sizeof(Item), SEEK_CUR)==-1) {
				sprintf(buf, "Error in item.");
				write(requestP[CFD].conn_fd, buf, strlen(buf));
			}
			else {
				if (fcntl(file_fd, F_SETLK, &fl) == -1) {
					if (errno == EACCES || errno == EAGAIN) {
			        	sprintf(secbuf, "This item is locked.\n");
			            write(requestP[CFD].conn_fd, secbuf, strlen(secbuf));
			        } else {
			            fprintf(stderr, "Unexpected error.\n");
			        }
				}
				else {
					read(file_fd, &bid,  sizeof(Item));
					sprintf(buf,"item%d $%d remain:%d\n", bid.id, bid.price, bid.amount);
					write(requestP[CFD].conn_fd, buf, strlen(buf));
				}
			}
			close(file_fd);
			close(requestP[CFD].conn_fd);
			FD_CLR(CFD, &LARGEST);
			free_request(&requestP[CFD]);
	#else
			requestP[CFD].wait_for_write = open("item_list", O_RDWR);
			fl.l_type = F_WRLCK;
			fl.l_whence = SEEK_CUR;
			fl.l_start = 0;
			fl.l_len = sizeof(Item);
			if (requestP[CFD].wait_for_read == 0) {		
				sscanf(requestP[CFD].buf, "%d", &requestP[CFD].item);
				lseek(requestP[CFD].wait_for_write, (requestP[CFD].item-1)*sizeof(Item), SEEK_SET);
				if (blockedlist[requestP[CFD].item] == 1) {				//same server tries to modify previous item
					sprintf(secbuf, "This item is locked.\n");
			        write(requestP[CFD].conn_fd, secbuf, strlen(secbuf));
			        requestP[CFD].wait_for_read = 2;
				}
				else {
					if (fcntl(requestP[CFD].wait_for_write, F_SETLK, &fl) == -1) {
				        if (errno == EACCES || errno == EAGAIN) {
				        	sprintf(secbuf, "This item is locked.\n");
				            write(requestP[CFD].conn_fd, secbuf, strlen(secbuf));
				        } else {
				            fprintf(stderr, "Unexpected error.\n");
				        }
				        requestP[CFD].wait_for_read = 2;
				    } else { /* Lock was granted... */
						read(requestP[CFD].wait_for_write, &bid, sizeof(Item));
						sprintf(secbuf, "This item is modifiable.\n");
						write(requestP[CFD].conn_fd, secbuf, strlen(secbuf));
						requestP[CFD].wait_for_read = 1;
						blockedlist[requestP[CFD].item] = 1;
					}
				}
			}
			else if(requestP[CFD].wait_for_read == 1){
					char temp[7];
					int am;
					sscanf(requestP[CFD].buf, "%s %d", &temp, &am);
					// int ret2 = handle_read(&requestP[CFD]);
					// sscanf(requestP[CFD].buf, "%s %d", &temp, &am);		//Get second values
					// if (ret < 0){
					// 	fprintf(stderr, "bad request from %s\n", requestP[CFD].host);
					// 	continue;
					// }
					// char buf2[512];
					// read(requestP[CFD].conn_fd, buf2, sizeof(buf2));
					// sscanf(buf2, "%s %d", &temp, &am);
					lseek(requestP[CFD].wait_for_write, (requestP[CFD].item-1)*sizeof(Item), SEEK_SET);
					if (strcmp(temp, "buy") == 0) {
						if (bid.amount-am >= 0) {
							bid.amount -= am;
							write(requestP[CFD].wait_for_write, &bid, sizeof(Item));
						}
						else {
							sprintf(buf, "Operation failed.\n");
							write(requestP[CFD].conn_fd, buf, strlen(buf));
						}
					}
					else if (strcmp(temp, "sell") == 0) {
						if (bid.amount+am >= 0) {
							bid.amount += am;
							write(requestP[CFD].wait_for_write, &bid, sizeof(Item));
						}
						else {
							sprintf(buf, "Operation failed.\n");	
							write(requestP[CFD].conn_fd, buf, strlen(buf));
						}
					}
					else if (strcmp(temp, "price") == 0) {
						if (am > 0) {
							bid.price = am;
							write(requestP[CFD].wait_for_write, &bid, sizeof(Item));
						}
						else {
							sprintf(buf, "Operation failed.\n");
							write(requestP[CFD].conn_fd, buf, strlen(buf));
						}
					}
					else {
						sprintf(buf, "Operation failed.\n");
						write(requestP[CFD].conn_fd, buf, strlen(buf));
					}
					lseek(requestP[CFD].wait_for_write, (requestP[CFD].item-1)*sizeof(Item), SEEK_SET);
					fl.l_type = F_UNLCK;
			        fl.l_whence = SEEK_CUR;
			        fl.l_start = 0;
			        fl.l_len = sizeof(Item);
			        if (fcntl(requestP[CFD].wait_for_write, F_SETLK, &fl) == -1)
			            fprintf(stderr, "Unexpected error.\n");  
			        requestP[CFD].wait_for_read = 2;
			        blockedlist[requestP[CFD].item] = 0;
				}
				if (requestP[CFD].wait_for_read == 2) {
					blockedlist[requestP[CFD].item] = 0;
			    	close(requestP[CFD].wait_for_write);
					close(requestP[CFD].conn_fd);
					FD_CLR(CFD, &LARGEST);
					free_request(&requestP[CFD]);
				}
	#endif
			}
		}
    }
    free(requestP);
    return 0;
}


// ======================================================================================================
// You don't need to know how the following codes are working
#include <fcntl.h>

static void* e_malloc(size_t size);


static void init_request(request* reqP) {
    reqP->conn_fd = -1;
    reqP->buf_len = 0;
    reqP->item = 0;
    reqP->wait_for_write = 0;
    reqP->wait_for_read;
}

static void free_request(request* reqP) {
    /*if (reqP->filename != NULL) {
        free(reqP->filename);
        reqP->filename = NULL;
    }*/
    init_request(reqP);
}

// return 0: socket ended, request done.
// return 1: success, message (without header) got this time is in reqP->buf with reqP->buf_len bytes. read more until got <= 0.
// It's guaranteed that the header would be correctly set after the first read.
// error code:
// -1: client connection error
static int handle_read(request* reqP) {
    int r;
    char buf[512];

    // Read in request from client
    r = read(reqP->conn_fd, buf, sizeof(buf));
    if (r < 0) return -1;
    if (r == 0) return 0;
	char* p1 = strstr(buf, "\015\012");
	int newline_len = 2;
	// be careful that in Windows, line ends with \015\012
	if (p1 == NULL) {
		p1 = strstr(buf, "\012");
		newline_len = 1;
		if (p1 == NULL) {
			ERR_EXIT("this really should not happen...");
		}
	}
	size_t len = p1 - buf + 1;
	memmove(reqP->buf, buf, len);
	reqP->buf[len - 1] = '\0';
	reqP->buf_len = len-1;
    return 1;
}

static void init_server(unsigned short port) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0) ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }
}

static void* e_malloc(size_t size) {
    void* ptr;

    ptr = malloc(size);
    if (ptr == NULL) ERR_EXIT("out of memory");
    return ptr;
}

