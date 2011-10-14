#include<stdio.h>
#include "semaphore.h"

/* argument number (should be 3) */
#define ARGNUM 3

/* length of buffer */
#define BUFSIZE 16

/* number of threads for reading source file */
#define NUMRTHREADS 3

/* number of threads for writing target file */
#define NUMWTHREADS 3

/* For control access to source file */
Semaphore read; 

/* For control access to target file */
Semaphore write;

/* wrapper functions */
void Pthread_create( pthread_t *restrict tidp, 
                     const pthread_attr_t *restrict attr,
                     void *(*start_rtn)(void*),
                     void *restrict arg );

void* Malloc( size_t size );


int 
main( int argc, char** argv )
{
  /* threads for reading source file */
  pthread_t readt1, readt2, readt3;
  /* threads for writing target file */
  pthread_t writet1, writet2, writet3;
  /* shared buffer */
  char **buffer;
  int i;

  if ( argc < ARGNUM ){
     fprintf( stderr, "Missing source file or target file\n");
     exit( EXIT_FAILURE );
  }
 
  /* Initialising semaphores */
  semaphore_init( &read );
  semaphore_init( &write );

  /* Initialising buffer */
  buffer = (char**)Malloc( sizeof(char*) * BUFSIZE );
 
  for( i=0; i<BUFSIZE; i++ ){
    buffer[i] = NULL;
  }

  /* start reading threads */
  Pthread_create( &read1, NULL, read_sourcefile, gloinfo );
  Pthread_create( &read2, NULL, read_sourcefile, gloinfo );
  Pthread_create( &read3, NULL, read_sourcefile, gloinfo );
  
  /* start writing threads */
  Pthread_create( &write1, NULL, write_targetfile ,gloinfo );
  Pthread_create( &write2, NULL, write_targetfile ,gloinfo );
  Pthread_create( &write3, NULL, write_targetfile ,gloinfo );


  free( buffer );
  return EXIT_SUCCESS;
}


void Pthread_create( pthread_t *restrict tidp, 
                     const pthread_attr_t *restrict attr,
                     void *(*start_rtn)(void*),
                     void *restrict arg ){
  int err;

  err = pthread_create( tidp, attr, start_rtn, arg );

  if( err != 0 ){
     perror("Thread error");
     exit(EXIT_FAILURE);
  }

}


void* Malloc( size_t size )
{
   void* result;
   result = malloc( size );
   if ( result == NULL ){
     perror("Memory error");
     exit(EXIT_FAILURE);
   }
   return result;
}
