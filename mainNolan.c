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

double error_sum(double *r, double *t, int size){
    int i;
    double norm_diff = 0, norm_vec = 0;
    for (i = 0; i < size; ++i){
        norm_diff += (r[i] - t[i]) * (r[i] - t[i]);
        norm_vec += t[i] * t[i];
    }
    return sqrt(norm_diff/norm_vec);
}

int main (int argc, char* argv[]){
    // instantiate variables
    struct node *nodehead;
    int nodecount;
    double *r, *r_pre;
    int i, j;
    //int iterationcount;
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
    
    for ( i = 0; i < nodecount; ++i)
        r[i] = 1.0 / nodecount;
    /* INITIALIZE MORE VARIABLES IF NECESSARY */

    //Split up work
    int sumOfInLinks = 0;
    for( i = 0; i < nodecount; ++i){
        sumOfInLinks += nodehead[i].num_in_links;
    }

    GET_TIME(start);

    int myRank, commSZ;
    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    MPI_Comm_size(MPI_COMM_WORLD, &commSZ);

    int* displs = malloc(sizeof(int)*commSZ);
    int* counts = malloc(sizeof(int)*commSZ);
    int* displsEven = malloc(sizeof(int)*commSZ);
    int* countsEven = malloc(sizeof(int)*commSZ);
    double* rOutNorm = malloc(nodecount * sizeof(double));

    int InLinksPer = sumOfInLinks / commSZ;
    // printf("%d\n",InLinksPer);
    // printf("%d\n",nodecount);
    
    //Assign which nodes each process will do
    sumOfInLinks = 0;
    int nodeStart = 0;
    int nodeEnd = -1;
    int localStart = 0;
    int localEnd = -1;
    int segNum = 0;
    for( i = 0; i < nodecount; ++i){
        sumOfInLinks += nodehead[i].num_in_links;

        if(sumOfInLinks >= InLinksPer || i == nodecount - 1){

            localStart = localEnd + 1;
            localEnd = i;
            displs[segNum] = localStart;
            counts[segNum] = localEnd - localStart + 1;

            
            //printf("%d %d\n",localStart,localEnd);
            if(segNum == myRank){
                nodeEnd = localEnd;
                nodeStart = localStart;
            }
            
            ++segNum;
            sumOfInLinks = 0;

        }
        
        //printf("%d\n",sumOfInLinks);
    }

    //Set up evenly distrubited counts and displs

    for (i = 0; i < commSZ; ++i){
        countsEven[i] = nodecount/commSZ + (nodecount%commSZ>0 ? (i < nodecount%commSZ ? 1 : 0) : 0);
        displsEven[i] = (i == 0 ? 0 : countsEven[i-1] + displsEven[i-1]);
    } 

    //printf("displs counts myRank - %d %d %d\n",displs[myRank],counts[myRank],myRank);


    // core calculation
    //double* rbuff = malloc(sizeof(double)*nodecount);
    double damping = (1-DAMPING_FACTOR)/nodecount;
    GET_TIME(start);
    //vec_cp(r, r_pre, nodecount);
    do{
        vec_cp(r, r_pre, nodecount);
        /* IMPLEMENT ITERATIVE UPDATE */

        //First Calculate all r_pre/out_going_edge for all r_pre
        GET_TIME(start);
        for (i = displsEven[myRank]; i < displsEven[myRank] + countsEven[myRank] ; ++i){
            rOutNorm[i] = r_pre[i]/nodehead[i].num_out_links;
        }
        GET_TIME(end);

        

        MPI_Allgatherv(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, rOutNorm, countsEven, displsEven, MPI_DOUBLE, MPI_COMM_WORLD);

        GET_TIME(start);
        for(i = displsEven[myRank]; i < displsEven[myRank] + countsEven[myRank] ; ++i){
            //printf("%d\n",i);
            struct node* currnode = &nodehead[i];
            double sum = 0;
            //Find Sum
            for (j = 0; j < currnode->num_in_links; ++j){
                int jNodeNum = currnode->inlinks[j];
                sum += rOutNorm[jNodeNum];
            }
            r[i] = damping+DAMPING_FACTOR*sum;
            
            //printf("%d: %d : %f : %f\n",iterationcount, i, r[i], r_pre[i]);
        }
        GET_TIME(end);

        // printf("%d : %d\n", myRank, nodeEnd - nodeStart);
        // printf("%d : %d\n", myRank, nodeEnd - nodeStart);
        printf("%d : %lf\n", myRank, end - start);
        

        //Sync R now
        //MPI_Bcast(&r[nodeStart], nodeEnd - nodeStart + 1, MPI_DOUBLE, myRank, MPI_COMM_WORLD);
        //int num = nodeEnd - nodeStart;
        //printf("%d\n",num);
        //MPI_Allgather(&r[nodeStart], num, MPI_DOUBLE, r, num, MPI_DOUBLE, MPI_COMM_WORLD);
        //if (myRank == 0) printf("doing");
        //printf("displs counts myRank - %d %d %d\n",displs[myRank],counts[myRank],myRank);
        MPI_Allgatherv(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, r, countsEven, displsEven, MPI_DOUBLE, MPI_COMM_WORLD);
        //if (myRank == 0) printf("done");
        //MPI_Barrier(MPI_COMM_WORLD);

    }while(rel_error(r, r_pre, nodecount) >= EPSILON);

    nodeStart = nodeEnd;
    nodeEnd = nodeStart;

    free(displs); free(counts); free(rOutNorm);
    free(displsEven); free(countsEven);

    MPI_Finalize();

    GET_TIME(end);



    Lab4_saveoutput(r, nodecount, end - start);

    // post processing
    node_destroy(nodehead, nodecount);
    free(r); free(r_pre); 
    return 0;
}
