#include "p-systems_lib.h"

/* Initilization */
void init_element_ThreadMatrix_OpMM_OpMMqueve_hasexp (int row, int col, int number_objects, void *object_function (void *node), pthread_t ThreadMatrix[][NMEMB], Node OpMM[][NMEMB], Node OpMMqueve[][NMEMB]) {
	pthread_t thread_id;
	Node *aux;
	
	/* OpMM */
	pthread_mutex_init (&(OpMM[row][col].CS), NULL);
	OpMM[row][col].objects = number_objects  ;
	OpMM[row][col].i_index = row;
	OpMM[row][col].j_index = col;
	
	
	/* OpMMqueve */
	pthread_mutex_init (&(OpMMqueve[row][col].CS), NULL);
	OpMMqueve[row][col].objects = 0  ;
	OpMMqueve[row][col].i_index = row;
	OpMMqueve[row][col].j_index = col;
	
	
	/* ThreadMatrix */
	aux = &(OpMM[row][col]);
	pthread_create ( &thread_id, NULL, object_function, (void *) aux);
	ThreadMatrix[row][col] = thread_id;
}


/* Instructions to set the 'total expiritation' condition. This means that
an object type in a membrane can no longer be used to apply rules. */
void set_hasexp (Node *node, pthread_barrier_t *bar_hasexp, pthread_barrier_t *bar_next_step) {

	pthread_mutex_unlock ( &(node->CS) );
	pthread_barrier_wait(bar_hasexp);
	pthread_barrier_wait(bar_next_step);
}


/* Finish functions */
void delete_membrane_threads (int membrane, int from, int to, pthread_t ThreadMatrix[][NMEMB]) {
    int i;
    
    if (from > to || membrane >= NMEMB || membrane < 0) {
        printf ("\t Can't delete threads in membrane %d in range (%d,%d)\n", membrane, from, to);
        return;
    }
    for (i=from; i<to; i++) {
        pthread_cancel (ThreadMatrix[i][membrane]);
    }
}


/* Functions to debug and show statistics at the end of execution  */
void print_matrix (Node OpMM[][NMEMB]) {
	int i, j;
	for (j=0; j<NMEMB; j++) {
		printf ("\t Membrane %d: \n", j);
		for (i=0; i<NTYPES; i++) {
			printf ("\t\t %d \n", OpMM[i][j].objects);
		}
		printf ("\n");	
	}
}

void print_matrix2 (int hasexp[][NMEMB]) {
	int i, j;
	for (j=0; j<NMEMB; j++) {
		printf ("\t Membrane %d: \n", j);
		for (i=0; i<NTYPES; i++) {
			printf ("\t\t %d \n", hasexp[i][j]);
		}
		printf ("\n");	
	}
}


void show_statistics (Node OpMM[][NMEMB]) {
	int i, j, total;
	printf ("----- Statistics: ----- \n");
	for (j=0; j<NMEMB; j++) {
		printf ("\t Membrane %d: \n", j);
		total = 0;
		for (i=0; i<NTYPES; i++) {
			printf ("\t\t Object type %d: %d\n", i, OpMM[j][i].objects);
			total = total + OpMM[i][j].objects;
		}
		printf ("\t TOTAL number of objects in membrane %d: %d\n\n", j, total);
	}
}


