#include "config.h"
#include <stdio.h>
#include <string.h>

#include <ctype.h>
#include <stdlib.h>

int parse_line( const char* line, char** key, char** val ) {
	const char* cursor = line;
	char key_buf[1024];
	char val_buf[1024];
	int i = 0;

	/* parse \s*(\w*)\s*=\s*(\w*) */

	while( *cursor && isspace(*cursor) ) cursor++;  /* \s* */
	if( !*cursor || *cursor == '#' ) return 1; /* comment */
	while( i < 1024 && *cursor && !isspace(*cursor) ) { key_buf[i++] = *cursor; cursor++; }
	key_buf[i] = 0;
	i = 0;
	while( *cursor && isspace(*cursor) ) cursor ++;
	if( ! *cursor ) return -1;
	while( i < 1024 && *cursor ) {val_buf[i++] = *cursor; cursor ++;}
	val_buf[i] = 0;

	*key = strdup(key_buf);
	*val = strdup(val_buf);

	return 0;
}

int read_config( const char* filename, conf_lambda_t f, void* closure ) {
	char extra[1024];

	char* key;
	char* val;

	int linenr = 0;
	int rc;

	FILE* conf = fopen( filename, "r" );

	if( ! conf ) return 1;

	char* line = NULL;
	while( (line = fgets(extra, 1024, conf)) != NULL ) {
		++ linenr ;
		rc = parse_line(line, &key, &val) ;
		if( rc < 0 ) {
			fprintf(stderr, "Syntax error line: %d\n", linenr);
			return 1;
		} else if (rc == 0) {
			if( f( closure, key, val ) ) {
				free(key);
				free(val);
				return 2;
			}
			free(key);
			free(val);
		}
	}

	return 0;
}

