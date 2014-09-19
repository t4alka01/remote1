/*
 * t2.c
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
    char targetFile[ 256 ] = { 0 };
    FILE *sourceF, *targetF;

    for( i = 1; i < argc; i++ )
    {
        printf( "%s ", argv[ i ] );
    }

    if( argc != 3 )
    {
        printf( "Check arguments!\n" );
    }
    else
    {
        strcpy( sourceFile, "/home/kari/temp/" );
        strcat( sourceFile, argv[ 1 ] );
        strcpy( targetFile, "/home/kari/temp/" );
        strcat( targetFile, argv[ 2 ] );

        sourceF = fopen( sourceFile, "r" );
        if( sourceF == NULL )
        {
            printf( "%s not found!\n", sourceFile );
        }
        else
        {
            char ch;

            targetF = fopen( targetFile, "w" );

            while( ! feof( sourceF ) )
            {
                ch = fgetc( sourceF );
                fputc( ch, targetF );
            }

            fclose( sourceF );
            fclose( targetF );
        }
    }

    return 0;
}

