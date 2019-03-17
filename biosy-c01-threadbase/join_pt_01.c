#include <pthread.h>                                                               
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#define NUMBER_OF_THREADS 5 

/* ------------------------------------------- */
void * threadFunc ( void * threadid )			
{
  int tid = (int) (intptr_t) threadid;

  printf("Thread %d: Start\n", tid);
  
  /* Simulation of some work */
  sleep(5);                                                                        

  printf("Thread %d: Stop\n", tid);

  return NULL;
}

/* ------------------------------------------- */
int main ( int argc, char * argv[] )
{
  pthread_attr_t   attr;
  pthread_t        threads[NUMBER_OF_THREADS];
  int              i, rc;
 
  printf("Main:     Start\n");

  /* Setup thread attributes */
  pthread_attr_init(&attr);                                                       /* <--- #1 */
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);                    /* <--- #2 */

  /* Create threads */
  for ( i = 0; i < NUMBER_OF_THREADS; i++) 
  {
    printf("Main:     Creating thread %d\n", i);
    rc = pthread_create( &threads[i], &attr, threadFunc, (void *)(intptr_t) i );   
    if ( rc ) 
    {
      printf("ERROR; return code from pthread_create() is %d\n", rc);
      return EXIT_FAILURE;
    }
  }

  /* Realse thread attributes */
  pthread_attr_destroy(&attr);                                                    /* <--- #3 */

  /* Wait for threads */
  for ( i = 0; i < NUMBER_OF_THREADS; i++) 
  {
    rc = pthread_join(threads[i], NULL);                                         /* <--- #4 */
    if (rc) 
    {
      printf("ERROR return code from pthread_join() is %d\n", rc);
      return EXIT_FAILURE;
    }
  }

  printf("Main:     Stop\n");
  return 0;
}

