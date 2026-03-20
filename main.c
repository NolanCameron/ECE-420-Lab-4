/*
    Serial Implementation of Lab 4
*/

#define LAB4_EXTEND

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#include "Lab4_IO.h"
#include "timer.h"

#define EPSILON 0.00001
#define DAMPING_FACTOR 0.85

int main (int argc, char* argv[]){
    // instantiate variables
    struct node *nodehead;
    int nodecount;
    double *r, *r_pre;
    int i, j;
    int iterationcount;
    double start, end;
    FILE *ip;
    /* INSTANTIATE MORE VARIABLES IF NECESSARY */

    
    // load data 
    if ((ip = fopen("data_input_meta","r")) == NULL) {
        printf("Error opening the data_input_meta file.\n");
        return 253;
    }
    fscanf(ip, "%d\n", &nodecount);
    fclose(ip);
    if (node_init(&nodehead, 0, nodecount)) return 254;
    
    // initialize variables
    r = malloc(nodecount * sizeof(double));
    r_pre = malloc(nodecount * sizeof(double));

    GET_TIME(start);
    
    iterationcount = 0;
    for ( i = 0; i < nodecount; ++i)
        r[i] = 1.0 / nodecount;
    /* INITIALIZE MORE VARIABLES IF NECESSARY */

    //Split up work
    int sumOfInLinks = 0;
    for( i = 0; i < nodecount; ++i){
        sumOfInLinks += nodehead[i].num_in_links;
    }

    int myRank, commSZ;
    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    MPI_Comm_size(MPI_COMM_WORLD, &commSZ);

    int InLinksPer = sumOfInLinks / commSZ;

    //Assign which nodes each process will do
    sumOfInLinks = 0;
    int nodeStart = 0;
    int nodeEnd = 0;
    int segNum = 0;
    for( i = 0; i < nodecount; ++i){
        sumOfInLinks += nodehead[i].num_in_links;
        if(sumOfInLinks > InLinksPer || i == nodecount - 1){
            nodeStart = nodeEnd;
            nodeEnd = i;
            if(segNum == myRank){
                break;
            }
            ++segNum;
            sumOfInLinks = 0;
        }
    }


    // core calculation
    do{
        ++iterationcount;
        vec_cp(r, r_pre, nodecount);
        /* IMPLEMENT ITERATIVE UPDATE */
        for( i = nodeStart; i < nodeEnd + 1; ++i){
            struct node currnode = nodehead[i];
            double sum = 0;
            //Find Sum
            for (j = 0; j < currnode.num_in_links; ++j){
                int jNodeNum = currnode.inlinks[j];
                struct node jNode = nodehead[jNodeNum];
                sum += r_pre[jNodeNum]/jNode.num_out_links;

            }
            r[i] = (1-DAMPING_FACTOR)/nodecount+DAMPING_FACTOR*sum;
            //printf("%d: %d : %f : %f\n",iterationcount, i, r[i], r_pre[i]);
        }

        //Sync R now
        MPI_Bcast(&r[nodeStart], nodeEnd - nodeStart + 1, MPI_DOUBLE, myRank, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);

    }while(rel_error(r, r_pre, nodecount) >= EPSILON);

    MPI_Finalize();

    GET_TIME(end);



    Lab4_saveoutput(r, nodecount, end - start);

    // post processing
    node_destroy(nodehead, nodecount);
    free(r); free(r_pre);
    return 0;
}
