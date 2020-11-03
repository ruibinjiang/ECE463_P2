/* INCLUDES */
#include "ne.h"
#include "router.h"
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

/* GLOBAL VARIABLES */
int ne_fd;
struct sockaddr_in serveraddr;

//from the last project :)
int open_udpfd(int port)
{
    int listenfd, optval=1;
    //struct sockaddr_in serveraddr;

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
    struct hostent * hp;

    //parse inputs
    if (argc != 5)
    {
        fprintf(stderr, "ERROR: incorrect arguments\n";
        printf("format: <router ID> <ne hostname> <ne UDP port> <router UDP port>\n");
        return EXIT_FAILURE;
    }

    int routerID = atoi(argv[1]);
    char * host = argv[2];
    int nePort = atoi(argv[3]);
    int routerPort = atoi(argv[4]);

    ne_fd = open_udpfd(nePort);
    if (ne_fd == -1)
    {
        fprintf(stderr, "ERROR: could not open network emulator UDP\n");
        return EXIT_FAILURE;
    }
    if ((hp = gethostbyname(host)) == NULL)
    {
        fprintf(stderr, "ERROR: hostname not found");
        return EXIT_FAILURE;
    }
    memset(&serveraddr, 0, sizeof(serveraddr));




}