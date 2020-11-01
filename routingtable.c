#include "ne.h"
#include "router.h"


/* ----- GLOBAL VARIABLES ----- */
struct route_entry routingTable[MAX_ROUTERS];
int NumRoutes;


////////////////////////////////////////////////////////////////
void InitRoutingTbl (struct pkt_INIT_RESPONSE *InitResponse, int myID){
	/* ----- YOUR CODE HERE ----- */

	//init other paths
	int i;
	for (i = 0; i < InitResponse->no_nbr; i++)
    {
	    routingTable[i].cost = InitResponse->nbrcost[i].cost;
	    routingTable[i].next_hop = InitResponse->nbrcost[i].nbr;
	    routingTable[i].dest_id = InitResponse->nbrcost[i].nbr;
    }

	//init path to self
	routingTable[i].cost = 0;
	routingTable[i].next_hop = myID;
	routingTable[i].dest_id = myID;
	NumRoutes = i + 1;
}


////////////////////////////////////////////////////////////////
int UpdateRoutes(struct pkt_RT_UPDATE *RecvdUpdatePacket, int costToNbr, int myID){
	/* ----- YOUR CODE HERE ----- */
	return 0;
}


////////////////////////////////////////////////////////////////
void ConvertTabletoPkt(struct pkt_RT_UPDATE *UpdatePacketToSend, int myID){
	/* ----- YOUR CODE HERE ----- */
	return;
}


////////////////////////////////////////////////////////////////
//It is highly recommended that you do not change this function!
void PrintRoutes (FILE* Logfile, int myID){
	/* ----- PRINT ALL ROUTES TO LOG FILE ----- */
	int i;
	int j;
	for(i = 0; i < NumRoutes; i++){
		fprintf(Logfile, "<R%d -> R%d> Path: R%d", myID, routingTable[i].dest_id, myID);

		/* ----- PRINT PATH VECTOR ----- */
		for(j = 1; j < routingTable[i].path_len; j++){
			fprintf(Logfile, " -> R%d", routingTable[i].path[j]);	
		}
		fprintf(Logfile, ", Cost: %d\n", routingTable[i].cost);
	}
	fprintf(Logfile, "\n");
	fflush(Logfile);
}

void UninstallRoutesOnNbrDeath(int DeadNbr){
	int i;
	for (i = 0; i < MAX_ROUTERS; i++)
	{
		if (routingTable[i].next_hop == DeadNbr)
		{
			routingTable[i].cost = INFINITY;
		}
	}
}