#include "ppmrw.h"

const int BUFFER_SIZE = 500;


void fail( char *s ) {

    printf( "Error: %s\n\n", s );
    printf( "Usage:\n" );
    printf( "ppmrw MAGICNUM INPUT OUTPUT\n\n" );
    exit(1);
}

FILE *read_header( FILE *fp, char *header_num, int *width, int *height, int *max_val ) {

    char width_buffer[ BUFFER_SIZE ];
    char height_buffer[ BUFFER_SIZE ];
    char max_val_buffer[ BUFFER_SIZE ];
    char iter;
    int index;

    if( fp == NULL ) {
        printf( "File not Found. Please check input filename." );
    }

    // Get the magic number from the file.
    header_num[ 0 ] = fgetc( fp );
    header_num[ 1 ] = fgetc( fp );
    header_num[ 2 ] = '\0';

    // Check if there are more characters than needed.
    if( fgetc( fp ) != '\n') {
        printf( "Error: Magic Number Invalid" );
        exit(1);
    }

    // Skip past the comments in the file if they exist.
    iter = fgetc( fp );
    while( iter == '#' ) {
        while( iter != '\n' ) {
            // Spool through the comment's characters until the newline
            iter = fgetc( fp );
        }
        // Check the starting character of the next line to see if it's 
        // another comment.
        iter = fgetc( fp );

    }

    // Iterate through and parse the width of the file.
    index = 0;
    while( iter != ' ' && index < BUFFER_SIZE - 1 ) {
        width_buffer[ index ] = iter;
        iter = fgetc( fp );
        index++;
    }

    width_buffer[ index ] = '\0';

    // Iterate through the rest of the line and parse the height of the file.
    iter = fgetc( fp );
    index = 0;
    while( iter != '\n' && index < BUFFER_SIZE - 1 ) {
        height_buffer[ index ] = iter;
        iter = fgetc( fp );
        index++;
    }

    height_buffer[ index ] = '\0';

    // Convert the width and the height into integers then assign the values
    // to the passed-in local variables.
    *width = atoi( width_buffer );
    *height = atoi( height_buffer );

    iter = fgetc( fp );
    index = 0;
    while( iter != '\n' && index < BUFFER_SIZE - 1 ) {
        max_val_buffer[index] = iter;
        iter = fgetc( fp );
        index++;
    }

    max_val_buffer[ index ] = '\0';

    // Convert the max_val string into an integer then assign the value
    // to the passed-in local variable.
    *max_val = atoi( max_val_buffer );

    // Return the file pointer so that we don't lose its place.
    return fp;
}

void read_p3( FILE *fp, uint8_t *pixmap, int length ) {

    // Initialize variables and buffers for reading.
    char line_string[ BUFFER_SIZE ];
    char *endptr;
    int parsed_int;

    for( int index = 0; index < length; index++ ) {
        // if statement will fail if EOF is reached.
        if( fgets( line_string, 5, fp ) ) {
            // Get the size of the parsed line.
            size_t parsed_len = strlen( line_string );

            // Check if nothing was parsed or if the parsed line is too long.
            if( parsed_len == 0 || line_string[ parsed_len - 1 ] != '\n' ) {
                char error = line_string[ parsed_len - 1  ];
                printf( "Error: One of the RGB channels in the input file " );
                printf( "does not contain a valid value" );
                printf( "\nlast char == %c\nlen== %ld", error, parsed_len );
                exit( 1 );
            }
        } else {
            printf( "Error: Not enough channels provided in input file." );
            exit( 1 );
        }

        // Parse the int using strtol (base 10).
        parsed_int = strtol( line_string, &endptr, 0 );

        // endptr will contain more than the the newline character if non-numerical
        // characters are parsed.
        if( *endptr != '\n') {
            printf( "Error: An RGB channel contained a non-numerical value." );
            printf( "\nSpecifically: %c", *endptr );
            exit( 1 );
        }

        if( parsed_int < 0 || parsed_int > 255) {
            printf( "Error: An RGB channel in the provided file is not an 8-Bit value." );
            printf( "\nSpecifically: %d", parsed_int );
            exit( 1 );
        }

        pixmap[ index ] = parsed_int;
    }
}

void write_p3( FILE *fp, uint8_t *pixmap, int width, int height, int max_val ) {
    
    int length = width * height * 3;
    char str_num[ BUFFER_SIZE ];

    // Add the first line of the header
    putc( 'P', fp );
    putc( '3', fp );
    putc( '\n', fp );

    // Convert the width integer to a string for iteration and file writing.
    snprintf( str_num, 10, "%d", width );

    // prime the while loop
    char iter = str_num[ 0 ];
    int index = 0;

    // Iterate through the string containing the width and write it to the file.
    while( iter != '\0' ) {
        putc( iter, fp );
        index++;
        iter = str_num[ index ];
    }

    // Add a space to delineate the width and height
    putc( ' ', fp );

    // Convert the height integer to a string for iteration and file writing.
    snprintf( str_num, 10, "%d", height );

    // Prime the while loop
    iter = str_num[ 0 ];
    index = 0;
    // Iterate through the string containing the height and write it to the file.
    while( iter != '\0' ) {
        putc( iter, fp );
        index++;
        iter = str_num[ index ];
    }

    // Add a newline to delineate the width/height line from the max_val line.
    putc( '\n', fp );

    // Convert the max value integer to a string for iteration and file writing.
    snprintf( str_num, 10, "%d", max_val );

    // Prime the while loop
    iter = str_num[ 0 ];
    index = 0;

    // Iterate through the string containing the max value and write it to the file.
    while( iter != '\0' ) {
        putc( iter, fp );
        index++;
        iter = str_num[ index ];
    }

    // Add a newline to delineate the max_val line from the RGB Channels
    putc( '\n', fp );

    for( int index = 0; index < length; index++ ) {
        snprintf( str_num, 10, "%d\n", pixmap[ index ] );
        fputs( str_num, fp );
    }
}

void read_p6( FILE *fp, uint8_t *pixmap, int length ) {
    
    int index;
    size_t num_elements;

    // Loop until the EOF is reached (num_elements == 0) or when the length is hit.
    for( index = 0; index < length; index++ ) {
        num_elements = fread( &pixmap[ index ], 1, 1, fp );

        if( num_elements == 0 ) {
            break;
        }
    }

    // Check there were enough RGB channels
    if( index < length ) {
        printf( "Error: The provided input file did not have enough RGB channels.");
        printf( "\nElements parsed: %d\nElements Expected: %d", index, length );
        exit(1);
    }

    if( fgetc( fp ) != EOF ) {
        printf( "Error: The provided input file had too many RGB channels.");
        exit(1);
    }
}

void write_p6( FILE *fp, uint8_t *pixmap, int width, int height, int max_val ) {
    
    int length = width * height * 3;
    char str_num[ BUFFER_SIZE ];

    putc( 'P', fp );
    putc( '6', fp );
    putc( '\n', fp );

    // Convert the width integer to a string for iteration and file writing.
    snprintf( str_num, 10, "%d", width );

    // prime the while loop
    char iter = str_num[ 0 ];
    int index = 0;

    // Iterate through the string containing the width and write it to the file.
    while( iter != '\0' ) {
        putc( iter, fp );
        index++;
        iter = str_num[ index ];
    }

    // Add a space to delineate the width and height
    putc( ' ', fp );

    // Convert the height integer to a string for iteration and file writing.
    snprintf( str_num, 10, "%d", height );

    // Prime the while loop
    iter = str_num[ 0 ];
    index = 0;
    // Iterate through the string containing the height and write it to the file.
    while( iter != '\0' ) {
        putc( iter, fp );
        index++;
        iter = str_num[index];
    }

    // Add a newline to delineate the width/height line from the max_val line.
    putc( '\n', fp );

    // Convert the max value integer to a string for iteration and file writing.
    snprintf( str_num, 10, "%d", max_val );

    // Prime the while loop
    iter = str_num[ 0 ];
    index = 0;

    // Iterate through the string containing the max value and write it to the file.
    while( iter != '\0' ) {
        putc( iter, fp );
        index++;
        iter = str_num[ index ];
    }

    // Add a newline to delineate the max_val line from the RGB Channels
    putc( '\n', fp );

    for( int index = 0; index < length; index++ ) {
        fwrite( &pixmap[ index ], sizeof( uint8_t ), 1, fp );
    }
}

