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
char log_filename[20];
struct pkt_INIT_RESPONSE initResponse;
struct sockaddr_in ne_serveraddr;
struct pkt_RT_UPDATE RT_request;
FILE* fptr;
struct nbr_cost nbrcost[MAX_ROUTERS];
long long int timeLastHeard[MAX_ROUTERS] = {0};
int isNbrDead[MAX_ROUTERS] = {0};
int nbrIndex[MAX_ROUTERS] = {0};
int isConverged = 0;

void * udp_update(void* args);
void * timer_update(void* args);

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
    int isUpdated = 0;
    int recIndex = 0;

    while(1)
    {
        //rec response
        recvfrom(ne_fd, &updateRequest, sizeof(updateRequest), 0, NULL, NULL);
        //printf("AAAAAA");
        pthread_mutex_lock(&lock);
        ntoh_pkt_RT_UPDATE(&updateRequest);
        //for loop to check for update time
        recIndex = updateRequest.sender_id;
        timeLastHeard[recIndex] = time(NULL);
        isNbrDead[recIndex] = 0;
        isUpdated = UpdateRoutes(&updateRequest,nbrcost[nbrIndex[recIndex]].cost,router_ID);
        if(isUpdated){
            fptr = fopen(log_filename, "a");
            PrintRoutes(fptr,router_ID);
            fclose(fptr);
            t_convergence = time(NULL);
            isConverged = 0;//??????????????
        }

        pthread_mutex_unlock(&lock);
    }
}

void * timer_update(void * args){
    struct pkt_RT_UPDATE RoutingTablePacket_Outbound;
    int i = 0;
    int nbrID = 0;
    int total_time = 0;

    while(1)
    {
        //first check the update interval
        pthread_mutex_lock(&lock);
        if (time(NULL) - t_update >= UPDATE_INTERVAL)
        {
            //convert routing table to a packet
            memset(&RoutingTablePacket_Outbound, 0, sizeof(RoutingTablePacket_Outbound));
            ConvertTabletoPkt(&RoutingTablePacket_Outbound, router_ID);
            
            for (i = 0; i < numNeighbors; i++)
            {
                //now send that to all the nodes
                RoutingTablePacket_Outbound.dest_id = nbrcost[i].nbr; //my brain hurts
                hton_pkt_RT_UPDATE(&RoutingTablePacket_Outbound);
                sendto(ne_fd, &RoutingTablePacket_Outbound, sizeof(RoutingTablePacket_Outbound), 0, (struct sockaddr *) &ne_serveraddr, sizeof(ne_serveraddr));
                ntoh_pkt_RT_UPDATE(&RoutingTablePacket_Outbound);
            }
            t_update = time(NULL);
        }
        //pthread_mutex_unlock(&lock);

        //then see if the neighbors are dead

        //pthread_mutex_lock(&lock);
        for (i = 0; i < numNeighbors; i++)
        {
            //check if neighbors are dead
            //if they are dead, table has not converged
            //uninstall the dead router
            //then print the routes
            nbrID = nbrcost[i].nbr;
            if (((time(NULL) - timeLastHeard[nbrID]) > FAILURE_DETECTION) && !(isNbrDead[nbrID]))
            {
                isNbrDead[nbrID] = 1;
                UninstallRoutesOnNbrDeath(nbrID);
                fptr = fopen(log_filename, "a");
                PrintRoutes(fptr, router_ID);
                fclose(fptr);
                isConverged = 0;
                t_convergence = time(NULL);
            }
        }
        //pthread_mutex_unlock(&lock);

        //finally we check if the table has finally  converged
        //pthread_mutex_lock(&lock);
        //somehow...
        if ((time(NULL) - t_convergence > CONVERGE_TIMEOUT) && !isConverged)
        {   
            total_time = (int)(time(NULL) - t_init);
            fptr = fopen(log_filename, "a");
            fprintf(fptr, "%d: Converged\n", total_time);
            fflush(fptr);
            fclose(fptr);
            isConverged = 1;
        }
        pthread_mutex_unlock(&lock);
    }
}

int main (int argc, char ** argv)
{
    /* MAIN VARIABLES */
    struct hostent * hp;
    struct pkt_INIT_REQUEST initRequest;
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
    memcpy(&nbrcost,&(initResponse.nbrcost),(MAX_ROUTERS)*sizeof(struct nbr_cost));
    
    sprintf(log_filename, "router%d.log", router_ID);
	fptr = fopen(log_filename, "w");
	PrintRoutes(fptr, router_ID);
    fclose(fptr);
    //threads stuff
    int i, temp;
	for(i = 0; i < numNeighbors; i++){
        temp = nbrcost[i].nbr;
		timeLastHeard[temp] = time(NULL);
        nbrIndex[temp] = i;
	}

	t_update = t_convergence = t_init = time(NULL);

	if(pthread_create(&udp_update_id, NULL, udp_update, NULL)){
		perror("Error creating thread for UDP update thread!\n");
		return EXIT_FAILURE;
	}
	if(pthread_create(&timer_update_id, NULL,timer_update, NULL)){
		perror("Error creating thread for timer update thread!\n");
		return EXIT_FAILURE;
	}

	pthread_join(udp_update_id,NULL);
	pthread_join(timer_update_id,NULL);

    
    return 0;
}

