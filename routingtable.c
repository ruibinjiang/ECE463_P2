#include "ne.h"
#include "router.h"


/* ----- GLOBAL VARIABLES ----- */
struct route_entry routingTable[MAX_ROUTERS];
int NumRoutes;
int id2Index[MAX_ROUTERS] = {0};

void InitRoutingTbl (struct pkt_INIT_RESPONSE *InitResponse, int myID){
	//init other paths

	int i;
	for(i=0;i<MAX_ROUTERS;i++){
		id2Index[i] = -1;
	}
	
	for (i = 0; i < InitResponse->no_nbr; i++)
    {
	    struct route_entry * cur = &(routingTable[i+1]);
	    cur->cost = InitResponse->nbrcost[i].cost;
	    cur->dest_id = InitResponse->nbrcost[i].nbr;
	    cur->next_hop = InitResponse->nbrcost[i].nbr;
	    cur->path[0] = myID;
	    cur->path[1] = InitResponse->nbrcost[i].nbr;
	    cur->path_len = 2;
		id2Index[InitResponse->nbrcost[i].nbr] = i+1;
    }

	//init path to self
	routingTable[0].path_len = 1;
	routingTable[0].path[0] = myID;
	routingTable[0].next_hop = myID;
    routingTable[0].dest_id = myID;
    routingTable[0].cost = 0;
	id2Index[myID] = 0;



    NumRoutes = (int) InitResponse->no_nbr + 1;
	//test print
	/*int j;
	for(i = 0; i < NumRoutes; i++){
		printf("<R%d -> R%d> Path: R%d", myID, routingTable[i].dest_id, myID);

		for(j = 1; j < routingTable[i].path_len; j++){
			printf(" -> R%d", routingTable[i].path[j]);	
		}
		printf( ", Cost: %d\n", routingTable[i].cost);
	}
	*/
}


int UpdateRoutes(struct pkt_RT_UPDATE *RecvdUpdatePacket, int costToNbr, int myID){
	int isUpdated = 0;  //update flag
	struct route_entry * currRoute;
	int destID, j,destID_Index;
	int splitHorizon = 0;
	int sum;

	//d = c(x,z)+d(z,y) cost of path
	//if next_hop(x,y)=z|(d<d(x,y) &x=nexthop(z,y)) force updated
	//else ....
	int i;
	for (i = 0; i < RecvdUpdatePacket->no_routes; i++)
	{	
		currRoute = &(RecvdUpdatePacket->route[i]);
		destID = (int) currRoute->dest_id;
		destID_Index = id2Index[destID];

        //check for existence
        if (destID_Index==-1){
			isUpdated=1;
			NumRoutes++;
			id2Index[destID] = NumRoutes-1;
			destID_Index = id2Index[destID];
			//add route to dest
			routingTable[destID_Index].dest_id = currRoute->dest_id;
			routingTable[destID_Index].next_hop = RecvdUpdatePacket->sender_id;
			if((currRoute->cost + costToNbr) >= INFINITY){
				routingTable[destID_Index].cost = INFINITY;
			}
			else{
				routingTable[destID_Index].cost = currRoute->cost + costToNbr;
			}
			routingTable[destID_Index].path_len = currRoute->path_len + 1;
			routingTable[destID_Index].path[0]=myID;
			memcpy(&(routingTable[destID_Index].path[1]),currRoute->path,(currRoute->path_len)*sizeof(int));
		}
		//forced update
		else if (routingTable[destID_Index].next_hop == RecvdUpdatePacket->sender_id){
			//change route, cost
			//????if not changing anything
			for (j = 0; j < currRoute->path_len; j++){
					if (currRoute->path[j] == myID){
						splitHorizon = 1;
						break;
					}
				}
			
			if(splitHorizon){
				if(routingTable[destID_Index].cost != INFINITY){
					routingTable[destID_Index].cost = INFINITY;
					isUpdated=1;
				}
			}
			else{
				if((currRoute->cost + costToNbr) >= INFINITY){
					if(routingTable[destID_Index].cost != INFINITY){
						routingTable[destID_Index].cost = INFINITY;
						isUpdated=1;
					}
				}
				else{
					if(routingTable[destID_Index].cost != currRoute->cost + costToNbr){
						isUpdated=1;
						routingTable[destID_Index].cost = currRoute->cost + costToNbr;
					}
				}
				//check path_len change
				if(routingTable[destID_Index].path_len != currRoute->path_len + 1){
					isUpdated=1;
					routingTable[destID_Index].path_len = currRoute->path_len + 1;
				}
				
				if(isUpdated){
					routingTable[destID_Index].path[0]=myID;
					memcpy(&(routingTable[destID_Index].path[1]),currRoute->path,(currRoute->path_len)*sizeof(int));
				}
				else if(memcmp(&(routingTable[destID_Index].path[1]),&(currRoute->path),(currRoute->path_len)*sizeof(int))){
					isUpdated=1;
					routingTable[destID_Index].path[0]=myID;
					memcpy(&(routingTable[destID_Index].path[1]),currRoute->path,(currRoute->path_len)*sizeof(int));
				}
			}
		}
		else{//split horizon
			sum = currRoute->cost + costToNbr;
			if (sum>INFINITY){
				sum=INFINITY;
			}
			if(sum < routingTable[destID_Index].cost){
				for (j = 0; j < currRoute->path_len; j++){
					if (currRoute->path[j] == myID){
						splitHorizon = 1;
						break;
					}
				}
				if(splitHorizon==0){
					isUpdated=1;
					//update everything
					//routingTable[destID_Index].dest_id = currRoute->dest_id;
					routingTable[destID_Index].next_hop = RecvdUpdatePacket->sender_id;
					if((currRoute->cost + costToNbr) >= INFINITY){
						routingTable[destID_Index].cost = INFINITY;
					}
					else{
						routingTable[destID_Index].cost = currRoute->cost + costToNbr;
					}
					routingTable[destID_Index].path_len = currRoute->path_len + 1;
					routingTable[destID_Index].path[0]=myID;
					memcpy(&(routingTable[destID_Index].path[1]),currRoute->path,(currRoute->path_len)*sizeof(int));
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
	        UpdatePacketToSend->route[x] = routingTable[i];
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
