/*		copyright	"%c%" 	*/

#ident	"@(#)auditmap.h	1.2"
#ident  "$Header$"
/*maximum number of digits in a number*/
#define MAXNUM	20	
#define LVLSZ	32
#define NAMESZ	1024
#define MAX_ATTEMPTS 5

#define MAP_FILE	"/auditmap"
#define ALIAS_FILE	"/ltf.alias"
#define CLASS_FILE	"/ltf.class"
#define CAT_FILE	"/ltf.cat"
#define LID_FILE	"/lid.internal"

#define O_MAPFILE	"/oauditmap"
#define O_ALIASFILE	"/oltf.alias"
#define O_CLASSFILE	"/oltf.class"
#define O_CATFILE	"/oltf.cat"
#define O_LIDFILE	"/olid.internal"

#define NOPERM	 	 ":17:Permission denied\n"
#define MSG_ARGV	 ":18:argvtostr() failed\n"
#define MSG_USAGE  	 ":51:usage: auditmap [-m dirname]\n"
#define MSG_NO_MAPFILE   ":52:Unable to create the auditmap file\n"
#define MSG_NO_DIR       ":53:Invalid full path or pathname %s specified\n"
#define MSG_NO_WRITE     ":54:%s is not writable\n"
#define MSG_AUDITCTL     ":55:auditctl() failed ASTATUS\n"
#define MSG_MALLOC       ":56:malloc() failed\n"
#define MSG_FILEBUSY     ":57:%s file busy\n"
#define MSG_FCNTL        ":58:fcntl() failed\n"
#define MSG_INCOMPLETE   ":59:\"%s\" not written to audit map file \"%s\"\n"
#define MSG_BADSTAT	 ":60:stat() failed\n"
#define NOPKG		 ":34:system service not installed\n"
#define MSG_NO_READ	 ":172:%s is not readable\n"
#define MSG_RENAME	 ":173:unable to rename file %s to %s\n"
#define LVLOPER		 ":35:%s() failed, errno = %d\n"

