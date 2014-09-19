/*
 * t5.c
 *
 *  Created on: Sep 18, 2014
 *      Author: kari
 */

#include <stdio.h>
#include <string.h>

void laske_sanat( FILE * );
void laske_rivit( FILE * );

int main( int argc, char *argv[ ] )
{
    int i;
    char sourceFile[ 256 ] = { 0 };
    char countType[ 256 ] = { 0 };
    FILE *sourceF;

    for( i = 1; i < argc; i++ )
    {
        printf( "%s \n", argv[ i ] );
    }

    if( argc != 3 )
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
            strcpy( countType, argv[ 2 ] );
            if( strcmp( countType, "-sanat" ) != 0 && strcmp( countType, "-rivit" ) != 0 )
            {
                printf( "Check 2. argument!\n" );
            }
            else
            {
                if( ! strcmp( countType, "-sanat" ) )
                {
                    laske_sanat( sourceF );
                }
                else
                {
                    laske_rivit( sourceF );
                }
            }
            fclose( sourceF );
        }
    }

    return 0;
}

void laske_sanat( FILE *file )
{
    int count;
    char ch;
    int isWord;

    isWord = 0;
    count = 0;
    while( 1 )
    {
        ch = fgetc( file );
        if( feof( file ) )
            break;
        if( ch != ' ' && ch != '\n' && ! isWord )
        {
            count++;
            isWord = 1;
        }
        if( ch == ' ' || ch == '\n' )
        {
            isWord = 0;
        }
    }
    printf( "Number of words = %d\n", count );
}

void laske_rivit( FILE *file )
{
    int count;
    char dummyTbl[ 512 ];

    count = 0;
    while( 1 )
    {
        fgets( dummyTbl, 512, file );
        if( feof( file ) )
            break;
        count++;
    }
    printf( "Number of rows = %d\n", count );
}

