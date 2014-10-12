#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

int main( void )
{
    FILE *f;
    int file = 0;
    char buffer[ 80 ] = { 0 };
    
    f = fopen( "/dev/virtualkb_dev", "r+" );
    file = fileno( f );
    printf( "file = %d\n", file ); 
     
    if( lseek( file, 2, SEEK_SET ) < 0 )
    {
       printf( "lseek error\n" ); 
    };
    if( read( file, buffer, 4 ) != 4 )
    {
       printf( "read error\n" ); 
    };
    printf( "%s\n", buffer );
    
    fclose( f );
    
    return 0;
}
