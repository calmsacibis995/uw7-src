/* @(#)ldaptab.c	1.3
 *
 * This library deals with any functions relating to /etc/ldap/ldaptab
 *
 *
 */

#include <stdio.h>
#include <string.h>
#include <nl_types.h>

/* Messages */

#define MSG_LINEERR1 \
    7,1,"%s: Line %d: Invalid line (ignored)\n"


#define LDAPTABFILE "/etc/ldap/ldaptab"

/* Globals */

static nl_catd  msgcatalog;


/* char *ldaptabIDtoFile( char *id )
 *
 *	pass in an identifier, and return the config file.
 *
 *	NB. Returns a malloc'd string
 */

char *
ldaptabIDtoFile( const char *id )
{
	FILE	*fp;
	char	buffer[BUFSIZ];
	char	ldaptabLine[5][50];
	int	lineNo=0;
	char	*returnedFile=NULL;

	msgcatalog=catopen("ldap.cat",0);

	if ( ( fp = fopen( LDAPTABFILE, "r" )) == NULL )
	{
		perror( LDAPTABFILE );
		exit( 1 );
	}

	/* Get the first line */
	while( ! feof( fp ) )
	{
		if ( fgets( buffer, BUFSIZ, fp ) != NULL )
		{
			int	colonNo=0;
			char	*ptr;
			char	*startOfField;
			int	i;
			int	lineLength;

			lineNo++;

			/* Get rid of carriage returns */
			lineLength = strlen(buffer);
			if ( buffer[lineLength-1] == '\n' ) {
				buffer[lineLength-1] = '\0';
				lineLength=lineLength-1;
			}

			/* get rid of comments */
			ptr = buffer;
			while( (*ptr != '\0') && isspace( *ptr ) ) ptr++;
			if ( *ptr == '#' ) continue;

			/* Check number of ':' */
			ptr=buffer;
			while( (ptr=strchr(ptr,':'))!=NULL )
			{
				colonNo++;
				ptr++;
			}
	
			if ( colonNo != 4)  {
				fprintf(stderr,
				    catgets(msgcatalog, MSG_LINEERR1),
				    LDAPTABFILE,
				    lineNo);
				continue;
			}

			/* Now get fields and put them in DS */
			startOfField=buffer;	
			ptr = buffer;
			for( i=0; i<4; i++ )
			{
				ptr=strchr( startOfField, ':' );
				*ptr = '\0';
	
				strcpy( ldaptabLine[i], startOfField );
				startOfField=++ptr;
			}
			strcpy(ldaptabLine[4], startOfField);

#ifdef DEBUG
			for ( i = 0; i<=4; i++ ) {
				printf( "\"%s\"\n", ldaptabLine[i] );
			}
#endif
	
			if ( strcmp( id, ldaptabLine[0] ) == 0 ) {
				returnedFile = strdup( ldaptabLine[2] );
				break;
			}
		}
	}
	return( returnedFile );
}
