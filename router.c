/* INCLUDES */
#include "ne.h"
#include "router.h"
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>


int main (int argc, char ** argv)
{
    /* THREAD VARIABLES */

    //parse inputs
    if (argc != 5)
    {
        printf("ERROR: incorrect arguments\n";
        printf("format: <router ID> <ne hostname> <ne UDP port> <router UDP port>\n");

        fprintf(stderr, "incorrect args\n");
        return EXIT_FAILURE;
    }

    int routerID = atoi(argv[1]);
    char * host = argv[2];
    int nePort = atoi(argv[3]);
    int routerPort = atoi(argv[4]);

}