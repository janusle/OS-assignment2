#include<stdio.h>
#include<string.h>
#include "semaphore.h"

/* argument number (should be 3) */
#define ARGNUM 3

/* length of buffer */
#define BUFSIZE 16

/* number of threads for reading source file */
#define NUMRTHREADS 3

/* number of threads for writing target file */
#define NUMWTHREADS 3

/* position of source file and target file in arguments */
#define SOURCEFILE 1
#define TARGETFILE 2

/* flag state */
#define ON 1
#define OFF 0

/* length of line */
#define LINELEN 1024

/* the struct is used to store global information 
 * such as shared buffer, close flag , read and write descriptor */
typedef struct {
  /* shared buffer */
  char **buffer; 
  /* pointer to next position in buffer */
  int wptr;
  /* pointer to next position in buffer */
  int rptr;
  /* end of file flag */
  int eofflag; 
  /* read descriptor */ 
  FILE* readfd;
  /* write descriptor */
  FILE* writefd;
} global_info;


/* Mutex for controlling access to source file */
Semaphore read; 

/* Mutex for controlling access to target file */
Semaphore write;

/* Mutex for controlling access to read buffer */
Semaphore readbuffer;

/* Mutex for  controlling access to write buffer */
Semaphore writebuffer;

/* For P/V operation */
Semaphore full;
Semaphore empty;

/* Mutex for controlling access to semaphore full 
 * Each time ,there is just one thread can access 
 * to check value of semaphore full */
Semaphore fullmutex;


/* global information about shared buffer, eof flag and etc */
global_info gloinfo;

/* wrapper functions */
void Pthread_create( pthread_t *tidp, 
                     const pthread_attr_t *attr,
                     void *(*start_rtn)(void*),
                     void *arg );
void* Malloc( size_t size );
FILE* Fopen( const char *filename, const char *mode );
void Pthread_join( pthread_t thread, void **rval_ptr );

/* function for reading source file */
void* read_sourcefile( void *arg );
/* function for writing target file */
void* write_targetfile( void *arg );


/* Exit handler 
 * When any error occurs, the exit() function
 * will be called, and before program termination,
 * exit() function will call exit_handler() to free all
 * memory the program allocated and destroy all semaphore */
void exit_handler();


int 
main( int argc, char** argv )
{
  /* threads for reading source file */
  pthread_t readt1, readt2, readt3;
  /* threads for writing target file */
  pthread_t writet1, writet2, writet3;
  int i; 
  void *res;

  /* check if source file and target file are specified */
  if ( argc < ARGNUM ){
     fprintf( stderr, "Missing source file or target file\n");
     exit( EXIT_FAILURE );
  }


  /* set exit handler */
  if ( atexit( exit_handler ) != 0 ){
     perror("set exit handler error");
     exit( EXIT_FAILURE );
  }

  /* Initialising semaphores and mutex */
  semaphore_init( &read );
  semaphore_init( &write );
  semaphore_init( &readbuffer );
  semaphore_init( &writebuffer );
  semaphore_init( &empty );
  semaphore_init( &full );
  semaphore_init( &fullmutex );

  /* set semaphores for P/V operation */
  empty.v = BUFSIZE;
  full.v = 0;
  
  /* Initialising global infomation */
  gloinfo.buffer = (char**)Malloc( sizeof(char*) * BUFSIZE ); 
  for( i=0; i<BUFSIZE; i++ ){
    gloinfo.buffer[i] = NULL;
  }
  gloinfo.wptr = 0;
  gloinfo.rptr = 0;
  gloinfo.eofflag = OFF;
  gloinfo.readfd = Fopen( argv[SOURCEFILE], "r");
  gloinfo.writefd = Fopen( argv[TARGETFILE], "w");


  /* start reading threads */
  Pthread_create( &readt1, NULL, read_sourcefile, NULL );
  Pthread_create( &readt2, NULL, read_sourcefile, NULL );
  Pthread_create( &readt3, NULL, read_sourcefile, NULL );
  
  /* start writing threads */  
  Pthread_create( &writet1, NULL, write_targetfile , NULL );
  Pthread_create( &writet2, NULL, write_targetfile , NULL );
  Pthread_create( &writet3, NULL, write_targetfile , NULL );
  

  Pthread_join( readt1 , &res );
  Pthread_join( readt2 , &res );
  Pthread_join( readt3 , &res );

  Pthread_join( writet1 , &res );
  Pthread_join( writet2 , &res );
  Pthread_join( writet3 , &res );

  /* exit_handler will be called to free memory the program allocated,
   * destory all semalhores and close all file descriptors */ 
  return EXIT_SUCCESS;
}


void* write_targetfile( void *data )
{
  
   char line[LINELEN];
 
   while(1){

     semaphore_down( &fullmutex ); /* get lock of reading semaphore full */
 
     /* check if eof flag is on */ 
     if( semaphore_value(&full) == 0 ){
        if( gloinfo.eofflag == ON  ){ 
           /*read threads are done, buffer is empty ,the thread exits*/
          
           /* release lock of reading semaphore 'full' */
           semaphore_up( &fullmutex );
           pthread_exit( (void*)EXIT_SUCCESS );
        }
        else{ /* buffer is empty , but read threads arn't done */ 
           semaphore_up( &fullmutex );
           continue;
        }
     }

     semaphore_down( &full ); /* decrement semaphore full, operation P */
     semaphore_up( &fullmutex );/* release lock of reading semaphore full */ 
     
     semaphore_down( &readbuffer ); /* get lock of reading shared buffer */ 
     semaphore_down( &write ); /* get lock of writing target file */
  
     /* get line from buffer */
     strcpy( line, gloinfo.buffer[gloinfo.rptr] );

     /* free read line */
     free( gloinfo.buffer[gloinfo.rptr] );
     gloinfo.buffer[gloinfo.rptr] = NULL;

     /* move rptr to next position */ 
     gloinfo.rptr = (gloinfo.rptr+1)%BUFSIZE;
     semaphore_up( &readbuffer );

     if ( fputs( line, gloinfo.writefd ) == EOF ){
        perror("I/O error");
        exit(EXIT_FAILURE);
     }
    
     semaphore_up( &empty ); /* increment semaphore empty, operation V */ 
     semaphore_up( &write ); /* release lock of writing target file */
   }

}



void* read_sourcefile( void *data )
{
   char* line;

   while(1){
  
     /* check if eof flag is on */
     if( gloinfo.eofflag == ON  ){
        pthread_exit( (void *)EXIT_SUCCESS );
     }

     semaphore_down( &empty ); /* decrement semaphore empty, Operation P */
     semaphore_down( &read );  /* get lock of reading source file */ 
     semaphore_down( &writebuffer ); /* get lock of writing to buffer */
   
     line = (char*)Malloc( sizeof(char) * LINELEN ); 
     line = fgets(line, LINELEN, gloinfo.readfd );

     if( line == NULL ){
        
        if( ferror( gloinfo.readfd ) != 0 ){
          /* I/O error occurs */
          perror("I/O error");
          exit( EXIT_FAILURE );
        }
        else{ /* eof */
          gloinfo.eofflag = ON;  /* set eof flag */
        }
     }

     semaphore_up( &read ); /* release lock of reading source file */

     /* if eof flag is off, add line to buffer */
     if( gloinfo.eofflag == OFF ){

         gloinfo.buffer[gloinfo.wptr] = line;

         /* move ptr to next available position */
         gloinfo.wptr = (gloinfo.wptr+1)%BUFSIZE;
         /* incerement semaphore full, operation V */ 
         semaphore_up( &full ); 
     } 

     semaphore_up( &writebuffer ); /* release lock of writing to buffer */
   }
}


/* wrapper functions */
FILE* 
Fopen( const char *filename, const char *mode )
{
   FILE* result;

   result = fopen( filename, mode );
   if( result == NULL ){
     perror("File error");
     exit(EXIT_FAILURE);
   }

   return result;
}


void 
Pthread_join( pthread_t thread, void **rval_ptr )
{
  if( pthread_join( thread, rval_ptr ) != 0 ){ /* join error */
    perror("thread join error"); 
    exit(EXIT_FAILURE);
  }

  if( (int)(*rval_ptr) != EXIT_SUCCESS ){ /* thread is failure */
    fprintf(stderr, "The return value of thread is not EXIT_SUCCESS\n");
    exit(EXIT_FAILURE);
  }
}


void 
Pthread_create( pthread_t *tidp, 
                     const pthread_attr_t *attr,
                     void *(*start_rtn)(void*),
                     void *arg ){
  int err;

  err = pthread_create( tidp, attr, start_rtn, arg );

  if( err != 0 ){
     perror("Thread error");
     exit(EXIT_FAILURE);
  }

}


void* 
Malloc( size_t size )
{
   void* result;
   result = malloc( size );
   if ( result == NULL ){
     perror("Memory error");
     exit(EXIT_FAILURE);
   }
   return result;
}


/* exit handler */
void 
exit_handler()
{ 
  int i;

  /* free buffer */
  for( i=0; i<BUFSIZE; i++ ){
    free(gloinfo.buffer[i]);
  }
  free( gloinfo.buffer );

  /* close all descriptors */
  fclose( gloinfo.readfd);
  fclose( gloinfo.writefd);

  /* destroy all semaphores */
  semaphore_destroy( &read );
  semaphore_destroy( &write );
  semaphore_destroy( &readbuffer );
  semaphore_destroy( &writebuffer );
  semaphore_destroy( &empty );
  semaphore_destroy( &full );
  semaphore_destroy( &fullmutex );
}



