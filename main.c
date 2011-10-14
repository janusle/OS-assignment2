#include<stdio.h>
#include "semaphore.h"

/* argument number (should be 3) */
#define ARGNUM 3

int 
main( int argc, char** argv )
{

  if ( argc < ARGNUM ){
     fprintf( stderr, "Missing source file or target file\n");
     exit( EXIT_FAILURE );
  }
  


  return EXIT_SUCCESS;
}
