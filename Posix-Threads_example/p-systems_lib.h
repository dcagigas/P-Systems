#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#define MAX_NUMBER_OBJECTS 1000

#define NTYPES  4 	// a, b, c, d.
#define NMEMB   1   // ab -> c
// ac -> b
//  d -> d& (Membrane disolution)

#define PRIORITY1 1
#define PRIORITY2 2

typedef struct node {
	pthread_mutex_t CS;
	int objects;
	unsigned short int i_index;
	unsigned short int j_index;
} Node;


/* Function rtdsc() to get the number of cycles elapsed */
#if defined(__i386__)
static __inline__ unsigned long long rdtsc(void)
{
    unsigned long long int x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
}

#elif defined(__x86_64__)
static __inline__ unsigned long long rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}
#endif

/*
pthread_t ThreadMatrix[NTYPES][NMEMB];
Node OpMM[NTYPES][NMEMB];
Node OpMMqueve[NTYPES][NMEMB];
//int hasexp[NTYPES][NMEMB];


int priority[NMEMB];
*/

/* Initilization */
void init_element_ThreadMatrix_OpMM_OpMMqueve_hasexp (int row, int col, int number_objects, void *object_function (void *node), pthread_t ThreadMatrix[][NMEMB], Node OpMM[][NMEMB], Node OpMMqueve[][NMEMB]);

/* Instructions to set the 'total expiritation' condition. This means that
	an object type in a membrane can no longer be used to apply rules. */
void set_hasexp (Node *node, pthread_barrier_t *bar_hasexp, pthread_barrier_t *bar_next_step);

/* Finish functions */
void delete_membrane_threads (int membrane, int from, int to, pthread_t ThreadMatrix[][NMEMB]);

/* Functions to debug and show statistics at the end of execution  */
void print_matrix (Node OpMM[][NMEMB]);
void print_matrix2 (int hasexp[][NMEMB]);
void show_statistics (Node OpMM[][NMEMB]);

