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
	int isUpdated = 0;  //update flag
	struct route_entry * currRoute;
	int destID, j;
	int splitHorizon = 0;

	//d = c(x,z)+d(z,y) cost of path
	//if next_hop(x,y)=z|(d<d(x,y) &x=nexthop(z,y)) force updated
	//else ....
	int i;
	for (i = 0; i < RecvdUpdatePacket->no_routes; i++)
	{	
		currRoute = &(RecvdUpdatePacket->route[i]);
		destID = (int) currRoute->dest_id;

        //check for existence
        if (routingTable[destID].dest_id != destID){
			isUpdated=1;
			NumRoutes++;
			//add route to dest
			routingTable[destID].dest_id = currRoute->dest_id;
			routingTable[destID].next_hop = RecvdUpdatePacket->sender_id;
			if((currRoute->cost + costToNbr) >= INFINITY){
				routingTable[destID].cost = INFINITY;
			}
			else{
				routingTable[destID].cost = currRoute->cost + costToNbr;
			}
			routingTable[destID].path_len = currRoute->path_len + 1;
			routingTable[destID].path[0]=myID;
			memcpy(&(routingTable[destID].path[1]),currRoute->path,(currRoute->path_len)*sizeof(int));
		}
		//forced update
		else if (routingTable[destID].next_hop == RecvdUpdatePacket->sender_id){
			//change route, cost
			//????if not changing anything
			isUpdated=1;
			if((currRoute->cost + costToNbr) >= INFINITY){
				routingTable[destID].cost = INFINITY;
			}
			else{
				routingTable[destID].cost = currRoute->cost + costToNbr;
			}
			routingTable[destID].path_len = currRoute->path_len + 1;
			routingTable[destID].path[0]=myID;
			memcpy(&(routingTable[destID].path[1]),currRoute->path,(currRoute->path_len)*sizeof(int));
		}
		else{//split horizon
			if((currRoute->cost + costToNbr) < routingTable[destID].cost){
				for (j = 0; j < currRoute->path_len; j++){
					if (currRoute->path[j] == myID)
						splitHorizon = 1;
						break;
				}
				if(splitHorizon){
					isUpdated=1;
					//update everything
					routingTable[destID].dest_id = currRoute->dest_id;
					routingTable[destID].next_hop = RecvdUpdatePacket->sender_id;
					if((currRoute->cost + costToNbr) >= INFINITY){
						routingTable[destID].cost = INFINITY;
					}
					else{
						routingTable[destID].cost = currRoute->cost + costToNbr;
					}
					routingTable[destID].path_len = currRoute->path_len + 1;
					routingTable[destID].path[0]=myID;
					memcpy(&(routingTable[destID].path[1]),currRoute->path,(currRoute->path_len)*sizeof(int));
				}
			}
		}
	}

	return isUpdated;
}


void ConvertTabletoPkt(struct pkt_RT_UPDATE *UpdatePacketToSend, int myID){
	UpdatePacketToSend->sender_id = myID;
	UpdatePacketToSend->no_routes = NumRoutes;

    int x = 0;
	int i;
	for (i = 0; i < MAX_ROUTERS; i++)
    {
	    if (routingTable[i].path_len != 0)
        {
	        struct route_entry * cur = &(UpdatePacketToSend->route[x]);

	        cur->path_len= routingTable[i].path_len;
	        cur->dest_id = routingTable[i].dest_id;
	        cur->cost = routingTable[i].cost;
	        cur->next_hop = routingTable[i].next_hop;

	        int j;
	        for (j = 0; j < MAX_PATH_LEN; j++)
            {
	            cur->path[j] = routingTable[i].path[j];
            }

	        x++;
        }
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