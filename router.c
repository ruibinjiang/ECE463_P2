/* INCLUDES */
#include "ne.h"
#include "router.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

/* GLOBAL VARIABLES */

//from the last project :)
int open_udpfd(int port)
{
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;

    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        return -1;

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&optval , sizeof(int)) < 0)
        return -1;

    /* Listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);
    if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;

    return listenfd;
}


int main (int argc, char ** argv)
{
    /* MAIN VARIABLES */
    int ne_fd;
    struct sockaddr_in ne_serveraddr;
    FILE* fptr;
    struct hostent * hp;
    struct pkt_INIT_REQUEST initRequest;
    struct pkt_INIT_RESPONSE initResponse;
    char log_filename[20];

    //parse inputs
    if (argc != 5)
    {
        fprintf(stderr, "ERROR: incorrect arguments\n");
        printf("format: <router ID> <ne hostname> <ne UDP port> <router UDP port>\n");
        return EXIT_FAILURE;
    }

    int router_ID = atoi(argv[1]);
    char * host = argv[2];
    int ne_port = atoi(argv[3]);
    int router_port = atoi(argv[4]);

    if ((ne_fd = open_udpfd(router_port)) == -1)
    {
        fprintf(stderr, "ERROR: could not open network emulator UDP\n");
        return EXIT_FAILURE;
    }
    if ((hp = gethostbyname(host)) == NULL)
    {
        fprintf(stderr, "ERROR: hostname not found\n");
        return EXIT_FAILURE;
    }

    bzero((char *) &ne_serveraddr, sizeof(ne_serveraddr));
    ne_serveraddr.sin_family = AF_INET;
    //strcpy((char *) &(ne_serveraddr.sin_addr), hp->h_addr_list[0]);
    bcopy(hp->h_addr_list[0],(char*)&ne_serveraddr.sin_addr.s_addr,hp->h_length);
    ne_serveraddr.sin_port = htons(ne_port);
    
    //prep request
    initRequest.router_id = htonl(router_ID);
    //send request
    sendto(ne_fd, &initRequest, sizeof(initRequest), 0, (struct sockaddr *) &ne_serveraddr, sizeof(ne_serveraddr));
    //rec response
    recvfrom(ne_fd, &initResponse, sizeof(initResponse), 0, NULL, NULL);

    ntoh_pkt_INIT_RESPONSE(&initResponse);
	InitRoutingTbl(&initResponse, router_ID);

	sprintf(log_filename, "router%d.log", router_ID);
	fptr = fopen(log_filename, "w");

	PrintRoutes(fptr, router_ID);

    fclose(fptr);
    return 0;
}