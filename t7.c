/*
 * t7.c
 *
 *  Created on: Sep 19, 2014
 *      Author: kari
 */


#include <stdio.h>
#include <string.h>

int main( void )
{
    char merkki;
    char jono[ ] = { "taateli" };
    char *p;

    printf( "Anna merkki: " );
    scanf( "%ch\n", & merkki );

    p = strchr( jono, merkki );

    if( p != NULL )
    {
        printf( "merkki loytyi!\n");
        printf( "merkin osoite: %p \n", p );
    }
    else
    {
        printf( "merkki ei loytynyt!\n");
    }

    return 0;
}

