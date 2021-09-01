#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

void fail( char *s );

FILE *read_header( FILE *fp, char *header_num, int *width, int *height, int *max_val );

void read_p3( FILE *fp, uint8_t *pixmap, int length );

void write_p3( FILE *fp, uint8_t *pixmap, int width, int height, int max_val );

void read_p6( FILE *fp, uint8_t *pixmap, int length );

void write_p6( FILE *fp, uint8_t *pixmap, int width, int height, int max_val );

