#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUMBER_OF_THREADS 8

/* ------------------------------------------- */
const char *messages[NUMBER_OF_THREADS] =
{
   "English: Hello World!",
   "French: Bonjour, le monde!",
   "Spanish: Hola al mundo",
   "Klingon: Nuq neH!",
   "German: Guten Tag, Welt!",
   "Russian: Zdravstvytye, mir!",
   "Japan: Sekai e konnichiwa!",
   "Latin: Orbis, te saluto!"
};

/* Vlaknu predavame vice ruznych parametru. Proto je zabalime do
 * struktury a predame odkaz na takovou strukturu.
 */
struct thread_data
{
  int  thread_id;
  int  result;
  const char *message;
};

/* ------------------------------------------- */
/* Formalni tvar funkce vlakna ma parametr typu void * a navratovou hodnotu take typu void *.
 * Tento tvar je predepsany funkci pthread_create. Nam by se hodilo predavat parametry jineho
 * typu (zde konkretne struct thread_data *). Toho nelze dosahnout primo, lze to ale vyresit
 * pretypovanim. Vime, ze pri volani pthread_create predame ukazatel na pripravenou strukturu
 * struct thread_data. Tedy vime, ze ve funkci vlakna nepracujeme s obecnym void * ukazatelem,
 * ale s adresou priprave struktury struct thread_data *. Proto si predany ukazatel pretypujeme,
 * abychom s predanymi daty mohli pracovat. Vlastne jsme jen obesli typovy system C/C++.
 */

void * helloThread ( void * threadarg )
{
  struct thread_data *my_data;

  sleep(1);
  my_data = (struct thread_data *) threadarg;
  my_data->result = my_data->thread_id * my_data->thread_id;
  printf("Thread %d: %s\n", my_data->thread_id, my_data->message);
  return NULL;
}

/* ------------------------------------------- */
int main(int argc, char *argv[])
{
  pthread_attr_t       attr;
  struct thread_data   thread_data_array[NUMBER_OF_THREADS];
  pthread_t            threads          [NUMBER_OF_THREADS];
  int                  rc, i;

  printf("Main:     Start\n");

  /* Setup thread attributes */
  pthread_attr_init(&attr);                                                 
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);   

  for( i = 0; i < NUMBER_OF_THREADS; i++) 
  {
    /* Initialize thread arguments */
    thread_data_array[i].thread_id = i;
    thread_data_array[i].result    = 0;
    thread_data_array[i].message   = messages[i];

    /* Create threads */
    printf("Main:     Creating thread %d\n", i);
    rc = pthread_create(&threads[i], &attr, helloThread, (void *) &thread_data_array[i]);
    if (rc) 
    {
      printf("ERROR; return code from pthread_create() is %d\n", rc);
      return EXIT_FAILURE;
    }
  }

  /* Realse thread attributes */
  pthread_attr_destroy(&attr);                                                    

  /* Wait for threads */
  for ( i = 0; i < NUMBER_OF_THREADS; i++) 
  {
    rc = pthread_join(threads[i], NULL);
    if (rc) 
    {
      printf("ERROR return code from pthread_join() is %d\n", rc);
      return EXIT_FAILURE;
    }
  }

  /* Print result */
  printf("Main:     results: ");
  for ( i = 0; i < NUMBER_OF_THREADS; i++) 
  {
    printf("%d, ", thread_data_array[i].result);
  }

  printf("\nMain:     Stop\n");
  return 0;
}

