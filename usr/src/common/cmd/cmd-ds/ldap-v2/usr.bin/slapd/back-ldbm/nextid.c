/* @(#)nextid.c	1.3 
 * 
 * nextid.c - keep track of the next id to be given out
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include "slap.h"
#include "back-ldbm.h"

/* Messages */
#define MSG_NOOPENR_NEXTID \
    1,94,"Could not open file \"%s\" to read\n"
#define MSG_NOOPENW_NEXTID \
    1,95,"Could not open file \"%s\" to write\n"
#define MSG_NEXTID_NOWRITE \
    1,96,"Cannot write id %d to \"%s\"\n"
#define MSG_NEXTID_FILECLOSE \
    1,97,"Cannot close file \"%s\"\n"
#define MSG_NEXTID_GET \
    1,98,"Cannot read next id from file \"%s\"\n"


ID
next_id( Backend *be )
{
	struct ldbminfo	*li = (struct ldbminfo *) be->be_private;
	char		buf[MAXPATHLEN];
	char		buf2[20];
	FILE		*fp;
	ID		id;

	sprintf( buf, "%s/NEXTID", li->li_directory );

	pthread_mutex_lock( &li->li_nextid_mutex );
	/* first time in here since startup - try to read the nexid */
	if ( li->li_nextid == -1 ) {
		if ( (fp = fopen( buf, "r" )) == NULL ) {

			logError( get_ldap_message(MSG_NOOPENR_NEXTID,
			    buf));

			li->li_nextid = 1;
		} else {
			if ( fgets( buf2, sizeof(buf2), fp ) != NULL ) {
				li->li_nextid = atol( buf2 );
			} else {

				logDebug( LDAP_LOG_LDBMBE,
		    "(next_id) %d could not fgets nextid from \"%s\"\n",
				    li->li_nextid, buf2, 0 );

				li->li_nextid = 1;
			}
			fclose( fp );
		}
	}

	li->li_nextid++;
	if ( (fp = fopen( buf, "w" )) == NULL ) {

		logError( get_ldap_message( MSG_NOOPENW_NEXTID, buf ));

	} else {
		if ( fprintf( fp, "%ld\n", li->li_nextid ) == EOF ) {

			logError(get_ldap_message(MSG_NEXTID_NOWRITE,
			    li->li_nextid, buf));

		}
		if( fclose( fp ) != 0 ) {

			logError(get_ldap_message(MSG_NEXTID_FILECLOSE,
			     buf));
		}
	}
	id = li->li_nextid - 1;
	pthread_mutex_unlock( &li->li_nextid_mutex );

	return( id );
}

void
next_id_return( Backend *be, ID id )
{
	struct ldbminfo	*li = (struct ldbminfo *) be->be_private;
	char		buf[MAXPATHLEN];
	FILE		*fp;

	pthread_mutex_lock( &li->li_nextid_mutex );
	if ( id != li->li_nextid - 1 ) {
		pthread_mutex_unlock( &li->li_nextid_mutex );
		return;
	}

	sprintf( buf, "%s/NEXTID", li->li_directory );

	li->li_nextid--;
	if ( (fp = fopen( buf, "w" )) == NULL ) {
		logError( get_ldap_message( MSG_NOOPENW_NEXTID, buf ));
	} else {
		if ( fprintf( fp, "%ld\n", li->li_nextid ) == EOF ) {

			logError(get_ldap_message(MSG_NEXTID_NOWRITE,
			    li->li_nextid, buf));

		}
		if( fclose( fp ) != 0 ) {

			logError(get_ldap_message(MSG_NEXTID_FILECLOSE,
			    buf));

		}
	}
	pthread_mutex_unlock( &li->li_nextid_mutex );
}

ID
next_id_get( Backend *be )
{
	struct ldbminfo	*li = (struct ldbminfo *) be->be_private;
	char		buf[MAXPATHLEN];
	char		buf2[20];
	FILE		*fp;
	ID		id;

	sprintf( buf, "%s/NEXTID", li->li_directory );

	pthread_mutex_lock( &li->li_nextid_mutex );
	/* first time in here since startup - try to read the nexid */
	if ( li->li_nextid == -1 ) {
		if ( (fp = fopen( buf, "r" )) == NULL ) {

			logError( get_ldap_message(MSG_NOOPENR_NEXTID,
			    buf));

			li->li_nextid = 1;
		} else {
			if ( fgets( buf2, sizeof(buf2), fp ) != NULL ) {
				li->li_nextid = atol( buf2 );
			} else {

				logError(get_ldap_message(MSG_NEXTID_GET,
				    buf));

				li->li_nextid = 1;
			}
			fclose( fp );
		}
	}
	id = li->li_nextid;
	pthread_mutex_unlock( &li->li_nextid_mutex );

	return( id );
}
