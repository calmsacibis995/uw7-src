#ident	"@(#)ivar.c	15.1	98/03/04"
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>

/*#define TESTING*/

#ifdef TESTING
#define DEBUG(a)	printf(a);
#else
#define DEBUG(a)	;
#endif

/* Format of an ifile entry.  Length is in records. */
/* [ used | length | flags | ASCIIZ name | ASCIIZ value | fill ] */

#define	RECORD	32

#ifdef TESTING
#define IFILE_NAME	"ifile_bin"
#else
#define IFILE_NAME	"/isl/ifile_bin"
#endif

#ifdef TESTING
#define	IFILE_MIN_SZ	(2*RECORD)
#define	IFILE_BUMP_SZ	(2*RECORD)
#else
#define	IFILE_MIN_SZ	(250*RECORD)
#define	IFILE_BUMP_SZ	(25*RECORD)
#endif

#define FILE_PERMS	(S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)

#define USED	'\x00'
#define UNUSED	'\x01'

#define IFILE_OKAY		0
#define E_IFILE_NOTRUNNING	1
#define E_IFILE_ALREADYRUNNING	2

char *ifile_ptr;		/* pointer to mmap'ed ifile */
char *ifile_end_ptr;		/* pointer to end of mmap'ed ifile */
long ifile_map_size;		/* size of mmap'ed ifile */
int ifile_fd;			/* file descriptor for mmap'ed ifile */
int ifile_running=0;

char start[]="=\"";
char end[]="\"\n";
char buffer[RECORD];		/* filled with UNUSED for resizing ifile */


int ivar_init( void )
{
	if( ifile_running )
		return E_IFILE_ALREADYRUNNING;

	memset( buffer, UNUSED, sizeof(buffer) );

	if( (ifile_fd=open( IFILE_NAME, O_RDWR|O_CREAT, FILE_PERMS )) == -1 ) {
		return 1;
	}

	ifile_map_size = lseek( ifile_fd, 0, SEEK_END );

/* If the ifile doesn't end on a RECORD boundary, we assume there is
 * just garbage at the end (but how would it get there?) and we drop
 * that section of the file.  The important thing is that it needs to
 * end on a RECORD boundary, and this should ensure that.
 */
	ifile_map_size = lseek( ifile_fd, ifile_map_size%RECORD, SEEK_END );

	while( ifile_map_size < IFILE_MIN_SZ ) {
		write( ifile_fd, &buffer, sizeof(buffer) );
		ifile_map_size += sizeof(buffer);
	}

	ifile_ptr = mmap( 0, ifile_map_size, PROT_READ|PROT_WRITE, MAP_SHARED,
			ifile_fd, 0 );
	ifile_end_ptr=ifile_ptr+ifile_map_size;

	ifile_running=1;

	return IFILE_OKAY;
}


int ivar_shutdown( void )
{
	if( !ifile_running )
		return E_IFILE_NOTRUNNING;

	msync( ifile_ptr, ifile_map_size, MS_SYNC );
	munmap( ifile_ptr, ifile_map_size );
	close( ifile_fd );

	ifile_running=0;
	return IFILE_OKAY;
}


/*
 * get_more_memory first packs the memory space and than,
 * depending on how much is empty at the end, it allocates
 * more space by unmapping the file, resizing it, and
 * re-mapping it.  The return value is the address of the
 * first free record in the ifile.
 */

char *get_more_memory( need_len )
{
	long oldsize, newsize;
	char *src_p=ifile_ptr+RECORD;
	char *dest_p=src_p;
	long incr;

/* Pack all of the ifile variables to the front of the ifile. */

	while( src_p != ifile_end_ptr ) {
		if( *src_p == USED && src_p != dest_p ) {
			incr=((int)*(src_p+1))*RECORD;
			memcpy( dest_p, src_p, incr );
			src_p+=incr;
			dest_p+=incr;
		}
		else
			src_p+=RECORD;
	}

	oldsize = (dest_p-ifile_ptr);

/* Mark the rest of the ifile unused */

	for( ; dest_p != ifile_end_ptr; dest_p+=RECORD )
		*dest_p=UNUSED;

/* Since we've probably changed a number of pages in the ifile,
 * we sync the whole thing.  If we end up unmapping it, it will
 * have to get sync'ed anyway.
 */
	msync( ifile_ptr, ifile_map_size, MS_SYNC );

/* See if the block of memory that's now available is big enough.
 * If not, resize the file and re-mmap.
 */

	if( oldsize+IFILE_BUMP_SZ > ifile_map_size ) {
		munmap( ifile_ptr, ifile_map_size );

		ifile_map_size = lseek( ifile_fd, 0, SEEK_END );
		newsize = ifile_map_size + IFILE_BUMP_SZ;

		while( ifile_map_size < newsize ) {
			write( ifile_fd, &buffer, sizeof(buffer) );
			ifile_map_size += sizeof(buffer);
		}

		ifile_ptr = mmap( 0, ifile_map_size, PROT_READ|PROT_WRITE,
				MAP_SHARED, ifile_fd, 0 );
		ifile_end_ptr=ifile_ptr+ifile_map_size;
	}

/* The first available slot in the ifile will be right after the
 * block of used slots, whose length is oldsize.
 */

	return ifile_ptr+oldsize;
}


/*
 * ifile_var_set finds an area in the mmapped ifile to store
 * the variable, value, and flags, calling get_more_memory
 * if it can't find enough space.  If it finds that the
 * variable was previously set, it sets the previously used
 * block of memory to unused as it goes along.
 */

int ifile_var_set( char *name, char *value, char flags )
{
	int i, found=0;
	char *p=ifile_ptr+RECORD;

	char *block_start;
	int block_size=0;

	int need_len=strlen(name)+strlen(value)+5;

	if( !ifile_running )
		return E_IFILE_NOTRUNNING;

	while( p != ifile_end_ptr && (block_size < need_len || !found) ) {
		if( *p == UNUSED ) {
			if( block_size < need_len ) {
				if( block_size == 0 )
					block_start=p;
				block_size+=RECORD;
			}
			p+=RECORD;
		}
		else {
			if( strcmp( p+3, name ) == 0 && *(p+2)==flags ) {
				found=1;
				for( i=0; i<(int)*(p+1); i++ )
					*(p+RECORD*i)=UNUSED;
				msync( p, ((int)*(p+1))*RECORD, MS_ASYNC );
			}
			else {
				if( block_size < need_len )
					block_size=0;
				p+=((int)*(p+1))*RECORD;
			}
		}
	}
	if( block_size < need_len )
		block_start=get_more_memory( need_len );

	*block_start=USED;
	*(block_start+1)=(char)((need_len+RECORD-1)/RECORD);
	*(block_start+2)=flags;
	strcpy( block_start+3, name );
	strcpy( block_start+4+strlen(name), value );
	msync( block_start, need_len, MS_ASYNC );

	return IFILE_OKAY;
}


/*
 * ifile_var_get returns a pointer to the copy of the value of
 * the variable whose name matches "name" and whose flags
 * match "flags".  The caller should free this memory when it
 * is no longer needed.  The function returns NULL if the
 * variable was not found.
 */

char *ifile_var_get( char *name, char flags )
{
	char *p=ifile_ptr+RECORD;

	if( !ifile_running )
		return NULL;

	while( p != ifile_end_ptr ) {
		if( *p == USED ) {
			if( strcmp( p+3, name ) == 0 && *(p+2)==flags ) {
				return strdup( p+4+strlen(p+3) );
			}
			p+=((int)*(p+1))*RECORD;
		}
		else
			p+=RECORD;
	}
	return NULL;
}


int ifile_dump( char *filename, char flags )
{
	char *p=ifile_ptr+RECORD;
	int fd, sl;

	if( !ifile_running )
		return E_IFILE_NOTRUNNING;

	if( (fd=open( filename, O_WRONLY|O_CREAT|O_TRUNC,
			FILE_PERMS )) == - 1 )
                return 1;

	while( p != ifile_end_ptr ) {
		if( *p == USED ) {
			if( *(p+2)==flags ) {
				sl=strlen(p+3);
				write( fd, p+3, sl );
				write( fd, start, 2 );
				write( fd, p+4+sl, strlen(p+4+sl) );
				write( fd, end, 2 );
			}
			p+=((int)*(p+1))*RECORD;
		}
		else
			p+=RECORD;
	}
	return IFILE_OKAY;
}


/* Interface functions (to be called from shell scripts) */


int ivar_set( char *name, char *value )
{
	return ifile_var_set( name, value, 0 );
}

int svar_set( char *name, char *value )
{
	return ifile_var_set( name, value, 1 );
}

char *ivar_get( char *name )
{
	return ifile_var_get( name, 0 );
}

char *svar_get( char *name )
{
	return ifile_var_get( name, 1 );
}

int ivar_dump( char *filename )
{
	return ifile_dump( filename, 0 );
}

int svar_dump( char *filename )
{
	return ifile_dump( filename, 1 );
}

char *long_mul(char *a, char *b)
{
	double m1=strtod(a, (char **)0) ,m2=strtod(b, (char **)0), result;
	char *retval;

	retval = (char *)malloc(sizeof(char)*256);
	result = m1 * m2;
	sprintf(retval, "%.1f", result);
	return retval;
}

char *long_div(char *a, char *b)
{
	double m1=strtod(a, (char **)0) ,m2=strtod(b, (char **)0), result;
	char *retval;

	retval = (char *)malloc(sizeof(char)*256);
	result = m1 / m2;
	sprintf(retval, "%.1f", result);
	return retval;
}

char *long_add(char *a, char *b)
{
	double m1=strtod(a, (char **)0) ,m2=strtod(b, (char **)0), result;
	char *retval;

	retval = (char *)malloc(sizeof(char)*256);
	result = m1 + m2;
	sprintf(retval, "%.1f", result);
	return retval;
}

char *long_sub(char *a, char *b)
{
	double m1=strtod(a, (char **)0) ,m2=strtod(b, (char **)0), result;
	char *retval;

	retval = (char *)malloc(sizeof(char)*256);
	result = m1 - m2;
	sprintf(retval, "%.1f", result);
	return retval;
}

int long_cmp(char *a, char *b)
{
	double l1, l2;

	l1=strtod(a, (char **)0);
	l2=strtod(b, (char **)0);

	if ( l1 == l2 )
		return 0;
	else if ( l1 < l2 )
		return -1;
	else
		return 1;
}

#ifdef TESTING

struct tests {
	char name[80];
	char value[80];
} test_list[] = {
	{ "LANG", "C" },
	{ "skip_license", "0" },
	{ "skip_license", "The rain in Spain falls mainly on the plain, I hear." },
	{ "skip_license", "0" },
	{ "EOF", "EOF" }
};


main( void )
{
	int i;
	char *joe;

DEBUG("ivar_init\n")
	ivar_init();

DEBUG("ivar_set\n")
	for( i=0; strcmp(test_list[i].name,"EOF") != 0; i++ ) {
		ivar_set( test_list[i].name, test_list[i].value );
	}

DEBUG("ivar_get\n")
	for( i=0; strcmp(test_list[i].name,"EOF") != 0; i++ ) {
		puts( test_list[i].name );
		joe=ivar_get( test_list[i].name );
		puts( joe );
		free( joe );
	}

	ivar_dump( "ifile" );
	ivar_shutdown();
}

#endif
