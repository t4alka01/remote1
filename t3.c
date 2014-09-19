/*
 * t3.c
 *
 *  Created on: Sep 18, 2014
 *      Author: kari
 */

#include <stdio.h>
#include <string.h>

int main( int argc, char *argv[ ] )
{
    int i;
    char sourceFile[ 256 ] = { 0 };
    FILE *sourceF;

    for( i = 1; i < argc; i++ )
    {
        printf( "%s \n", argv[ i ] );
    }

    if( argc != 2 )
    {
        printf( "Check arguments!\n" );
    }
    else
    {
        strcpy( sourceFile, argv[ 1 ] );

        sourceF = fopen( sourceFile, "r" );
        if( sourceF == NULL )
        {
            printf( "%s not found!\n", sourceFile );
        }
        else
        {
            int count;

            count = 0;
            while( 1 )
            {
                fgetc( sourceF );
                if( feof( sourceF ) )
                    break;
                count++;
            }
            printf( "Number of chars = %d\n", count );

            fclose( sourceF );
        }
    }

    return 0;
}

