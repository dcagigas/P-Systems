#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h> /* for struct timeval, gettimeofday */
#include "p-systems_lib.h"

enum objects { OBJECT_A, OBJECT_B, OBJECT_C, OBJECT_D};

int number_a, number_b, number_c;

pthread_t ThreadMatrix[NTYPES][NMEMB];
Node OpMM[NTYPES][NMEMB];
Node OpMMqueve[NTYPES][NMEMB];
//int hasexp[NTYPES][NMEMB];


int priority[NMEMB];
	
/* Threads (object) bodies: */
void *object_type_a (void *node);
void *object_type_b (void *node);
void *object_type_c (void *node);
void *object_type_d (void *node);
void *total_exp_thread (void *arg);


/* Barriers used to start all threads at the same time */
/* A second barrier 'bar2' is neccesary for execution time measurement */
static pthread_barrier_t bar1, bar2, bar_hasexp, bar_next_step;

int main()
{
	time_t t;
	unsigned long long time1, time2;
	struct timeval start, end;
	double secs = 0;
	int j;
	int total_objects = 0;
	pthread_t thread_create_id;
	
	/* Intializes random number generator */
	srand((unsigned) time(&t));
	
	printf ("Number of \'a\' objects : ");
	scanf ("%d", &number_a);
	if (number_a > MAX_NUMBER_OBJECTS || number_a < 0) {
		printf ("Number of \'a\' objects should be positive or cero and not greater than %d\n", MAX_NUMBER_OBJECTS);
		exit(0);
	}
	
	printf ("Number of \'b\' objects : ");
	scanf ("%d", &number_b);	
	if (number_b > MAX_NUMBER_OBJECTS || number_b < 0) {
		printf ("Number of \'b\' objects should be positive or cero and not greater than %d\n", MAX_NUMBER_OBJECTS);
		exit(0);
	}
	
	printf ("Number of \'c\' objects : ");
	scanf ("%d", &number_c);	
	if (number_c > MAX_NUMBER_OBJECTS || number_c < 0) {
		printf ("Number of \'c\' objects should be positive or cero and not greater than %d\n", MAX_NUMBER_OBJECTS);
		exit(0);
	}
	
	total_objects = number_a + number_b + number_c;
	
	/* Set barrier 'bar1' and 'bar2' to number of threads/objects created. 
	The first instruction of every thread/object is to wait for these barriers. 
	This ensures that all threads/objects start at the same time but in
	arbitrary order. And with 'bar2' that execution time is right measured */
	pthread_barrier_init(&bar1, NULL, NTYPES*NMEMB+1);
	pthread_barrier_init(&bar2, NULL, NTYPES*NMEMB+1);
	
	/* Same mechanism as before but for synchronizing threads in maximal rule execution */
	pthread_barrier_init(&bar_hasexp, NULL, NTYPES*NMEMB+1);
	pthread_barrier_init(&bar_next_step, NULL, NTYPES*NMEMB+1);
	
	
	/* Create object threads */
	/* Parameters: type of object (row), membrane (col), number of objects, thread, ThreadMatrix, OpMM, OpMMqueve, hasexp */
	init_element_ThreadMatrix_OpMM_OpMMqueve_hasexp (OBJECT_A, 0, number_a, &object_type_a, ThreadMatrix, OpMM, OpMMqueve);
	init_element_ThreadMatrix_OpMM_OpMMqueve_hasexp (OBJECT_B, 0, number_b, &object_type_b, ThreadMatrix, OpMM, OpMMqueve);
	init_element_ThreadMatrix_OpMM_OpMMqueve_hasexp (OBJECT_C, 0, number_c, &object_type_c, ThreadMatrix, OpMM, OpMMqueve);
	init_element_ThreadMatrix_OpMM_OpMMqueve_hasexp (OBJECT_D, 0, 1, &object_type_d, ThreadMatrix, OpMM, OpMMqueve);
	pthread_create ( &thread_create_id, NULL, total_exp_thread, NULL);
	
	/* Initial priority in each membrane */
	for (j=0; j< NMEMB; j++)
		priority[j] = PRIORITY1;
	
	
	/* Time measurement */
	pthread_barrier_wait(&bar1);
		//clock_gettime(CLOCK_REALTIME, &start);
		gettimeofday(&start, NULL);
		time1 = rdtsc();	
	pthread_barrier_wait(&bar2);
	
	/* Wait for 'd' type object thread in membrane 0 to finish */
	pthread_join(ThreadMatrix[NTYPES-1][0], NULL);
	gettimeofday(&end, NULL);
	//clock_gettime(CLOCK_REALTIME, &end);
	time2 = rdtsc();
	
	pthread_barrier_destroy(&bar1);
	pthread_barrier_destroy(&bar2);
	
	
	/* Clean up pending threads? or just show statistics */
	printf ("\n\t INITIAL number of objects: %d \n", total_objects);
	show_statistics (OpMM);

	secs = (double)(end.tv_usec - start.tv_usec) / 1000000 + (double)(end.tv_sec - start.tv_sec);
	printf("\t Time elpased is %lf seconds\n", secs);
	printf("\t Clock ticks: %llu\n", time2-time1);
	
	return 0;
}



/* Threads (object) bodies: */

void *object_type_a (void *node) {
	Node *myself;
	int minimum, objects_B, objects_C, number_objects, random_rule;
	int blocked_B, blocked_C;
	
	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	pthread_barrier_wait(&bar1);
	pthread_barrier_wait(&bar2);
	
	myself = (Node *)node;
	while (1) {
		/* First of all, block this current object type (thread) */
		pthread_mutex_lock ( &(myself->CS) );	
		objects_B = OpMM[OBJECT_B][myself-> j_index].objects;
		objects_C = OpMM[OBJECT_C][myself-> j_index].objects;
		if (OpMM[myself-> i_index][myself-> j_index].objects == 0 || priority[myself-> j_index] != PRIORITY1 || (objects_B<=0 && objects_C<=0) ) {
			/* hasexp condition */
			set_hasexp (myself, &bar_hasexp, &bar_next_step);
		} else {
			/* Check rules (LHS types) . In this case:
				'b' because: 'ab -> c' and
				'c' because: 'ac -> b' */
			random_rule = rand() % 2;
			switch (random_rule) 
			{
			case 0:
				/* Apply 'ab -> c' */
				blocked_B = pthread_mutex_trylock(&(OpMM[OBJECT_B][myself-> j_index].CS));
				if (blocked_B==0 ) {
					/* Successful block of B */
					objects_B = OpMM[OBJECT_B][myself-> j_index].objects;
					if (objects_B >0) {
						minimum = objects_B;
						if (OpMM[myself-> i_index][myself-> j_index].objects < minimum) {
							/* A < B. Substract A objects from B */
							number_objects = OpMM[OBJECT_A][myself-> j_index].objects;
							OpMM[OBJECT_B][myself-> j_index].objects -= number_objects;
							OpMM[OBJECT_A][myself-> j_index].objects = 0;
						} else {
							/* B <= A. Substract B objects from A */
							number_objects = OpMM[OBJECT_B][myself-> j_index].objects;
							OpMM[OBJECT_A][myself-> j_index].objects -= number_objects;
							OpMM[OBJECT_B][myself-> j_index].objects = 0;						
						} 
						/* Add C objects */
						pthread_mutex_lock (&(OpMMqueve[OBJECT_C][myself-> j_index].CS));
						OpMMqueve[OBJECT_C][myself-> j_index].objects += number_objects;
						pthread_mutex_unlock (&(OpMMqueve[OBJECT_C][myself-> j_index].CS));
					}
					/* Unlock B */
					pthread_mutex_unlock(&(OpMM[OBJECT_B][myself-> j_index].CS));
				} 
				break;
			case 1:
				/* Apply 'ac->b' rule */
				blocked_C = pthread_mutex_trylock(&(OpMM[OBJECT_C][myself-> j_index].CS));
				if (blocked_C ==0 ) {  
					/* Successful block of C */
					objects_C = OpMM[OBJECT_C][myself-> j_index].objects;
					if ( objects_C >0) {
						minimum = objects_C;
						if (OpMM[myself-> i_index][myself-> j_index].objects < minimum) {
							/* A < C. Substract A objects from C */
							number_objects = OpMM[OBJECT_A][myself-> j_index].objects;
							OpMM[OBJECT_C][myself-> j_index].objects -= number_objects;
							OpMM[OBJECT_A][myself-> j_index].objects = 0;
						} else {
							/* C <= A. Substract C objects from A */
							number_objects = OpMM[OBJECT_C][myself-> j_index].objects;
							OpMM[OBJECT_A][myself-> j_index].objects -= number_objects;
							OpMM[OBJECT_C][myself-> j_index].objects = 0;						
						} 
						/* Add B objects */
						pthread_mutex_lock (&(OpMMqueve[OBJECT_B][myself-> j_index].CS));
						OpMMqueve[OBJECT_B][myself-> j_index].objects += number_objects;
						pthread_mutex_unlock (&(OpMMqueve[OBJECT_B][myself-> j_index].CS));
					}
					/* Unlock C */
					pthread_mutex_unlock(&(OpMM[OBJECT_C][myself-> j_index].CS));
				}
				break;
			}
			pthread_mutex_unlock ( &(myself->CS) );	
		
		}
	}	
}


void *object_type_b (void *node) {
	Node *myself;
	int minimun, number_objects;
	
	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	pthread_barrier_wait(&bar1);
	pthread_barrier_wait(&bar2);
	

	myself = (Node *)node;
	while (1) {
		/* First of all, block this current object type (thread) */
		pthread_mutex_lock ( &(myself->CS) );	
		if (OpMM[myself-> i_index][myself-> j_index].objects <= 0 || priority[myself-> j_index] != PRIORITY1) {
			/* hasexp condition */
			set_hasexp (myself, &bar_hasexp, &bar_next_step);
		} else {
			/* Check every rule (LHS types) . In this case only 'a' because: 'ab -> c' */
			if (pthread_mutex_trylock(&(OpMM[OBJECT_A][myself-> j_index].CS)) == 0) {
				/* Successful block. Is OBJECT_A <=0? */
				minimun = OpMM[OBJECT_A][myself-> j_index].objects;
				if (minimun<=0) {
					/* hasexp condition */
					pthread_mutex_unlock(&(OpMM[OBJECT_A][myself-> j_index].CS));
					set_hasexp (myself, &bar_hasexp, &bar_next_step);
				} else {
					/* Reaction, apply rule ab -> c */
					if (OpMM[myself-> i_index][myself-> j_index].objects < minimun) {
						/* B < A. Substract B objects from A */
						number_objects = OpMM[OBJECT_B][myself-> j_index].objects;
						OpMM[OBJECT_A][myself-> j_index].objects -= number_objects;
						OpMM[OBJECT_B][myself-> j_index].objects = 0;
					} else {
						/* A <= B. Substract A objects from B */
						number_objects = OpMM[OBJECT_A][myself-> j_index].objects;
						OpMM[OBJECT_B][myself-> j_index].objects -= number_objects;
						OpMM[OBJECT_A][myself-> j_index].objects = 0;						
					} 
					/* Add B objects */
					pthread_mutex_lock (&(OpMMqueve[OBJECT_C][myself-> j_index].CS));
					OpMMqueve[OBJECT_C][myself-> j_index].objects += number_objects;
					pthread_mutex_unlock (&(OpMMqueve[OBJECT_C][myself-> j_index].CS));
					
					pthread_mutex_unlock(&(OpMM[OBJECT_A][myself-> j_index].CS));
					pthread_mutex_unlock ( &(myself->CS) );
				}
			} else {
				/* Not Successful block. Try again. Sleep a time period? */
				pthread_mutex_unlock ( &(myself->CS) );	
			}
		}
	}
}


void *object_type_c (void *node) {
	Node *myself;
	int minimun, number_objects;
	
	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	pthread_barrier_wait(&bar1);
	pthread_barrier_wait(&bar2);
	
	myself = (Node *)node;
	while (1) {
		/* First of all, block this current object type (thread) */
		pthread_mutex_lock ( &(myself->CS) );	
		if (OpMM[myself-> i_index][myself-> j_index].objects <= 0 || priority[myself-> j_index] != PRIORITY1) {
			/* hasexp condition */
			set_hasexp (myself, &bar_hasexp, &bar_next_step);
		} else {
			/* Check every rule (LHS types) . In this case only 'a' because: 'ac -> b' */
			if (pthread_mutex_trylock(&(OpMM[OBJECT_A][myself-> j_index].CS)) == 0) {
				/* Successful block. Is OBJECT_A <=0? */
				minimun = OpMM[OBJECT_A][myself-> j_index].objects;
				if (minimun<=0) {
					/* hasexp condition */
					pthread_mutex_unlock(&(OpMM[OBJECT_A][myself-> j_index].CS));
					set_hasexp (myself, &bar_hasexp, &bar_next_step);
				} else {
					/* Reaction, apply rule ac -> b */
					if (OpMM[myself-> i_index][myself-> j_index].objects < minimun) {
						/* C < A. Substract C objects from A */
						number_objects = OpMM[OBJECT_C][myself-> j_index].objects;
						OpMM[OBJECT_A][myself-> j_index].objects -= number_objects;
						OpMM[OBJECT_C][myself-> j_index].objects = 0;
					} else {
						/* A <= C. Substract A objects from C */
						number_objects = OpMM[OBJECT_A][myself-> j_index].objects;
						OpMM[OBJECT_C][myself-> j_index].objects -= number_objects;
						OpMM[OBJECT_A][myself-> j_index].objects = 0;						
					} 
					/* Add B objects */
					pthread_mutex_lock (&(OpMMqueve[OBJECT_B][myself-> j_index].CS));
					OpMMqueve[OBJECT_B][myself-> j_index].objects += number_objects;
					pthread_mutex_unlock (&(OpMMqueve[OBJECT_B][myself-> j_index].CS));
					
					pthread_mutex_unlock(&(OpMM[OBJECT_A][myself-> j_index].CS));
					pthread_mutex_unlock ( &(myself->CS) );
				}
			} else {
				/* Not Successful block. Try again. Sleep a time period? */
				pthread_mutex_unlock ( &(myself->CS) );	
			}
		}
	}
}


void *object_type_d (void *node) {
	Node *myself;
	char end = 0;
	int res, value;
	
	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	pthread_barrier_wait(&bar1);
	pthread_barrier_wait(&bar2);
	
	/* Membrane disolution */
	myself = (Node *)node;
	
	while (!end) {
		/* Block myself */
		pthread_mutex_lock ( &(myself->CS) );
		if (priority[myself-> j_index] >= PRIORITY2) {
			/* Check if priority has changed */
			end = 1;
			pthread_mutex_unlock ( &(myself->CS) );
		} else {
			/* set hasexp  = 1 */
			set_hasexp (myself, &bar_hasexp, &bar_next_step);

		}
	}
    delete_membrane_threads (myself->j_index, 0, NTYPES-1, ThreadMatrix);
}


void *total_exp_thread (void *arg) {
	int i, j, enqueved_any_item;
	
	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	
	while (1) {
        
	/* Wait for all threads to reach the hasexp condition */
	pthread_barrier_wait(&bar_hasexp);
	
	enqueved_any_item = 0;
	for (j=0; j<NMEMB ; j++) {
		for (i=0; i< NTYPES; i++) {
			OpMM[i][j].objects = OpMM[i][j].objects + OpMMqueve[i][j].objects;
			if (!enqueved_any_item && OpMMqueve[i][j].objects>0) {
				enqueved_any_item = 1;
			}
			OpMMqueve[i][j].objects = 0;
		}
		if (!enqueved_any_item) {
			/* Not enqueved any object. No steps further can be done.
			Priority must be changed, to disolve membrane and finish? */
			priority[j]++;
		}
	}

	/* All threads are waiting to reach 'bar_next_step'. Now, they can continue. */
	pthread_barrier_wait(&bar_next_step);

	}
}

	
