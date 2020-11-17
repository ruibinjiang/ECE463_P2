/* INCLUDES */
#include "ne.h"
#include "router.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

/* GLOBAL VARIABLES */
int ne_fd;
int router_ID;
int numNeighbors;
long long int t_convergence = 0;
long long int t_update = 0;
long long int t_init = 0;
pthread_mutex_t lock;
struct pkt_INIT_RESPONSE initResponse;
struct sockaddr_in ne_serveraddr;
struct pkt_RT_UPDATE RT_request;
FILE* fptr;
long long int timeLastChecked[MAX_ROUTERS] = {0};
int isNeighborDead[MAX_ROUTERS] = {0};
int isConverged = 0;

void * udp_update(void);
void * timer_update(void);

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

void * udp_update(void * args){
    struct pkt_RT_UPDATE updateRequest;
    //int isUpdated = 0;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    for (;;)
    {
        //rec response
        recvfrom(ne_fd, &updateRequest, sizeof(updateRequest), 0, NULL, NULL);
        
        pthread_mutex_lock(&lock);
        ntoh_pkt_RT_UPDATE(&updateRequest);
        //for loop to check for update time

        isUpdated = UpdateRoutes(&updateRequest,?????cost,router_ID);
        if(isUpdated){
            PrintRoutes(fptr,router_ID);
            t_convergence = time(NULL);
            isConverged = ;//??????????????
        }

        pthread_mutex_unlock(&lock);
    }
#pragma clang diagnostic pop
}

void * timer_update(void * args){
    struct pkt_RT_UPDATE RoutingTablePacket_Outbound;
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    for (;;)
    {
        //first check the update interval
        pthread_mutex_lock(&lock);
        if (time(NULL) - t_checkUpdate >= UPDATE_INTERVAL)
        {
            //convert routing table to a packet
            memset(&RoutingTablePacket_Outbound, 0, sizeof(RoutingTablePacket_Outbound));
            ConvertTabletoPkt(&RoutingTablePacket_Outbound, router_ID);
            int i;
            for (i = 0; i < numNeighbors; i++)
            {
                //now send that mf to all the nodes
                RoutingTablePacket_Outbound.dest_id = initResponse.nbrcost[i]; //my brain hurts
                hton_pkt_RT_UPDATE(&RoutingTablePacket_Outbound);
                sendto(ne_fd, &RoutingTablePacket_Outbound, sizeof(RoutingTablePacket_Outbound), 0, &ne_serveraddr, sizeof(ne_serveraddr));
                ntoh_pkt_RT_UPDATE(&RoutingTablePacket_Outbound);
            }
            t_update = time(NULL);
        }
        //pthread_mutex_unlock(&lock);

        //then see if the neighbors are dead

        //pthread_mutex_lock(&lock);
        int i;
        for (i = 0; i < numNeighbors; i++)
        {
            //check if neighbors are dead
            //if they are dead, table has not converged
            //uninstall the dead router
            //then print the routes
            if (((time(NULL) - timeLastChecked[i]) > FAILURE_DETECTION) && !(isNeighborDead[i]))
            {
                isNeighborDead[i] = 1;
                UninstallRoutesOnNbrDeath(initResponse.nbrcost[i].nbr);
                PrintRoutes(fptr, router_ID);
                isConverged = 0;
            }
        }
        //pthread_mutex_unlock(&lock);

        //finally we check if the table has finally fucking converged
        //pthread_mutex_lock(&lock);
        //somehow...
        if (time(NULL) - t_update > CONVERGE_TIMEOUT)
        {
            fprintf(fptr, "%d: Converged\n", (int)(t_update - t_init));
            fflush(fptr);
            isConverged = 1;
        }
        pthread_mutex_unlock(&lock);
    }
#pragma clang diagnostic pop
}

int main (int argc, char ** argv)
{
    /* MAIN VARIABLES */
    struct hostent * hp;
    struct pkt_INIT_REQUEST initRequest;
    char log_filename[20];
    pthread_t udp_update_id;
	pthread_t timer_update_id;

    //parse inputs
    if (argc != 5)
    {
        fprintf(stderr, "ERROR: incorrect arguments\n");
        printf("format: <router ID> <ne hostname> <ne UDP port> <router UDP port>\n");
        return EXIT_FAILURE;
    }

    router_ID = atoi(argv[1]);
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
    //init routing table
    ntoh_pkt_INIT_RESPONSE(&initResponse);
	InitRoutingTbl(&initResponse, router_ID);
	numNeighbors = initResponse.no_nbr;


	sprintf(log_filename, "router%d.log", router_ID);
	fptr = fopen(log_filename, "w");

	PrintRoutes(fptr, router_ID);

    //threads stuff
    //????????????????????????????????????????
    int c;
	for(c = 0; c < MAX_ROUTERS; c++){
		realtime[c] = time(NULL);
	}

	t_update = t_convergence = t_init = time(NULL);

	if(pthread_create(&udp_update_id, NULL, udp_update, NULL)){
		perror("Error creating thread for UDP update!");
		return EXIT_FAILURE;
	}
	if(pthread_create(&timer_update_id, NULL,timer_update, NULL)){
		perror("Error creating thread for timer update!");
		return EXIT_FAILURE;
	}

	pthread_join(udp_update_id,NULL);
	pthread_join(timer_update_id,NULL);

    fclose(fptr);
    return 0;
}

