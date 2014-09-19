/*
 * t1.c
 *
 *  Created on: Sep 18, 2014
 *      Author: kari
 */

#include <stdio.h>

int main(  int argc, char *argv[] )
{
    int i;

    for( i = argc - 1; i > 0; i-- )
    {
        printf( "%s ", argv[ i ] );
    }

    return 0;
}
