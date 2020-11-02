#include "ne.h"
#include "router.h"


/* ----- GLOBAL VARIABLES ----- */
struct route_entry routingTable[MAX_ROUTERS];
int NumRoutes;


void InitRoutingTbl (struct pkt_INIT_RESPONSE *InitResponse, int myID){
	//init other paths
	int i;
	for (i = 0; i < InitResponse->no_nbr; i++)
    {
	    struct route_entry * cur = &(routingTable[(InitResponse->nbrcost[i].nbr)]);

	    cur->cost = InitResponse->nbrcost[i].cost;
	    cur->dest_id = InitResponse->nbrcost[i].nbr;
	    cur->next_hop = InitResponse->nbrcost[i].nbr;
	    cur->path[0] = myID;
	    cur->path[1] = InitResponse->nbrcost->nbr;
	    cur->path_len = 2;
    }

	//init path to self
	routingTable[myID].path_len = 1;
	routingTable[myID].path[0] = myID;
	routingTable[myID].next_hop = myID;
    routingTable[myID].dest_id = myID;
    routingTable[myID].cost = 0;

    NumRoutes = (int) InitResponse->no_nbr + 1;
}


int UpdateRoutes(struct pkt_RT_UPDATE *RecvdUpdatePacket, int costToNbr, int myID){
	int updated = 0;
	//d = c(x,z)+d(z,y) cost of path
	//if next_hop(x,y)=z|(d<d(x,y) &x=nexthop(z,y)) force updated
	//else ....
	int i;
	for (i = 0; i < RecvdUpdatePacket->no_routes; i++){
		//total cost INF check


	}
	return updated;
}


void ConvertTabletoPkt(struct pkt_RT_UPDATE *UpdatePacketToSend, int myID){
	UpdatePacketToSend->sender_id = myID;
	UpdatePacketToSend->no_routes = NumRoutes;

	int i;
	for (i = 0; i < NumRoutes; i++)
    {
	    UpdatePacketToSend->route[i] = routingTable[i];
    }
}


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
	for (i = 0; i < NumRoutes; i++)
	{
		if (routingTable[i].next_hop == DeadNbr)
		{
			routingTable[i].cost = INFINITY;
		}
	}
}