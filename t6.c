/*
 * t6.c
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

    printf( "merkin osoite: %p \n", p );

    return 0;
}
