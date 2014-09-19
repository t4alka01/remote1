/*
 * t9.c
 *
 *  Created on: Sep 19, 2014
 *      Author: kari
 */

#include <stdio.h>
#include <string.h>

int main( void )
{
    char merkki;
    char jono[ 256 ] = { 0 };
    char *p;

    printf( "Anna merkkijono: " );
    fgets( jono, 256, stdin );

    printf( "Anna merkki: " );
    scanf( "%ch", & merkki );

    p = strchr( jono, merkki );
    if( p != NULL )
    {
        int count;

        printf( "merkin osoite: %p \n", p );
        count = 1;
        while( 1 )
        {
            p = strchr( p + 1, merkki );
            if( p != NULL )
            {
                printf( "merkin osoite: %p \n", p );
                count++;
            }
            else
            {
                break;
            }
        }
        printf( "merkki loytyi %d kertaa\n", count );
    }
    else
    {
        printf( "merkki ei loytynyt kertaakaan!\n" );
    }

    return 0;
}

