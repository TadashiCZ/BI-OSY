#include <pthread.h>                                                               /* <--- #1 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#define NUMBER_OF_THREADS 5 

/* ------------------------------------------- */
void * threadFunc ( void * threadid )						   /* <--- #2 */
{
  int tid = (int) (intptr_t) threadid;

  printf("Thread %d: Start\n", tid);

  /* Simulation of some work */
  sleep(10);                                                                        /* <--- #3 */

  printf("Thread %d: Stop\n", tid);

  /* exit(0);             */                                                       /* <--- #4 */
  /* pthread_exit(NULL);  */                                                       /* <--- #5 */
  return NULL;
}

/* ------------------------------------------- */
int main ( int argc, char * argv[] )
{
  pthread_t    threads[NUMBER_OF_THREADS];
  int          i, rc;
 
  printf("Main:     Start\n");

  /* Create threads */
  for ( i = 0; i < NUMBER_OF_THREADS; i++) 
  {
    printf("Main:     Creating thread %d\n", i);
    rc = pthread_create( &threads[i], NULL, threadFunc, (void *)(intptr_t) i);    /* <--- #6 */
    if ( rc ) 
    {
      printf("ERROR; return code from pthread_create() is %d\n", rc);
      return EXIT_FAILURE;
    }
  }

  printf("Main:     Stop\n");

  pthread_exit(NULL);                                                              /* <--- #7 */
  return 0;
}

