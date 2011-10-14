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

/* For control access to source file */
Semaphore read; 

/* For control access to target file */
Semaphore write;

Semaphore readbuffer;
Semaphore writebuffer;
Semaphore full;
Semaphore empty;

/* wrapper functions */
void Pthread_create( pthread_t *tidp, 
                     const pthread_attr_t *attr,
                     void *(*start_rtn)(void*),
                     void *arg );
void* Malloc( size_t size );
FILE* Fopen( const char *filename, const char *mode );


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


/* function for reading source file */
void* read_sourcefile( void *arg );
/* function for writing target file */
void* write_targetfile( void *arg );


int 
main( int argc, char** argv )
{
  /* threads for reading source file */
  pthread_t readt1, readt2, readt3;
  /* threads for writing target file */
  pthread_t writet1, writet2, writet3;
  global_info gloinfo;
  
  
  int i;

  if ( argc < ARGNUM ){
     fprintf( stderr, "Missing source file or target file\n");
     exit( EXIT_FAILURE );
  }
 
  /* Initialising semaphores */
  semaphore_init( &read );
  semaphore_init( &write );
  semaphore_init( &readbuffer );
  semaphore_init( &writebuffer );
  semaphore_init( &empty );
  semaphore_init( &full );
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
  Pthread_create( &readt1, NULL, read_sourcefile, &gloinfo );
  Pthread_create( &readt2, NULL, read_sourcefile, &gloinfo );
  Pthread_create( &readt3, NULL, read_sourcefile, &gloinfo );
  
  /* start writing threads */ 
  
  Pthread_create( &writet1, NULL, write_targetfile ,&gloinfo );
  Pthread_create( &writet2, NULL, write_targetfile ,&gloinfo );
  Pthread_create( &writet3, NULL, write_targetfile ,&gloinfo );
  

  pthread_join( readt1 , NULL );
  pthread_join( readt2 , NULL );
  pthread_join( readt3 , NULL );

  pthread_join( writet1 , NULL );
  pthread_join( writet2 , NULL );
  pthread_join( writet3 , NULL );

  /* destory everything */
  semaphore_destroy( &read );
  semaphore_destroy( &write ); 
  /* free buffer */
  for( i=0; i<BUFSIZE; i++ ){
    free(gloinfo.buffer[i]);
  }
  free( gloinfo.buffer );
  fclose( gloinfo.readfd);
  fclose( gloinfo.writefd);
  return EXIT_SUCCESS;
}



void* write_targetfile( void *gloinfo )
{
  
   global_info *gloinfoptr;
   char line[LINELEN];

   /* for test */
   /*fprintf(stderr, "%ld start\n", (long)pthread_self() );*/

   gloinfoptr = ( global_info* )gloinfo;

   while(1){
  
     /* check if eof flag is on */
     if( gloinfoptr->eofflag == ON && 
         semaphore_value(&full)== 0 ){
        fprintf(stderr, "%ld quit\n", (long)pthread_self() );
        pthread_exit( (void *)EXIT_SUCCESS );
     }

     semaphore_down( &full );
     semaphore_down( &readbuffer );     
     semaphore_down( &write );
  
     /* get line from buffer */
     strcpy( line, gloinfoptr->buffer[gloinfoptr->rptr] );

     /* free read line */
     free( gloinfoptr->buffer[gloinfoptr->rptr] );
     gloinfoptr->buffer[gloinfoptr->rptr] = NULL;

      
     gloinfoptr->rptr = (gloinfoptr->rptr+1)%LINELEN;
     semaphore_up( &readbuffer );

     if ( fputs( line, gloinfoptr->writefd ) == EOF ){
        perror("I/O error");
        exit(EXIT_FAILURE);
     }
    
     /* for test */
     fprintf(stderr, "%s", line );

     semaphore_up( &empty ); 
     semaphore_up( &write );
   }
}



void* read_sourcefile( void *gloinfo )
{
  
   global_info *gloinfoptr;
   char* line;

   /* for test */
   /*fprintf(stderr, "%ld start\n", (long)pthread_self() );*/

   gloinfoptr = ( global_info* )gloinfo;

   while(1){
  
     /* check if eof flag is on */
     if( gloinfoptr->eofflag == ON  ){
        /*fprintf(stderr, "%ld quit\n", (long)pthread_self() );*/
        pthread_exit( (void *)EXIT_SUCCESS );
     }

     semaphore_down( &empty );
     semaphore_down( &read );     
     semaphore_down( &writebuffer );
   
     line = (char*)Malloc( sizeof(char) * LINELEN ); 
     line = fgets(line, LINELEN, gloinfoptr->readfd );

     if( line == NULL ){

        if( ferror( gloinfoptr->readfd ) != 0 ){
          /* error occurs */
          perror("I/O error");
          exit( EXIT_FAILURE );
        }
        else{ /* eof */
          gloinfoptr->eofflag = ON; 
        }
     }

     semaphore_up( &read );

     /* if only eofflag is off, add line to buffer */
     if( gloinfoptr->eofflag == OFF ){
         gloinfoptr->buffer[gloinfoptr->wptr] = line;
         /* for test */ 
         /*fprintf(stderr, "%s", line );*/
         /* move ptr to next available position */
         gloinfoptr->wptr = (gloinfoptr->wptr+1)%LINELEN;
     } 

     semaphore_up( &full ); 
     semaphore_up( &writebuffer );
   }
}



/* wrapper functions */

FILE* Fopen( const char *filename, const char *mode )
{
   FILE* result;

   result = fopen( filename, mode );
   if( result == NULL ){
     perror("File error");
     exit(EXIT_FAILURE);
   }

   return result;
}


void Pthread_create( pthread_t *tidp, 
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

