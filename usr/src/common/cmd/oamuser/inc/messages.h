#ident  "@(#)messages.h	1.5"
#ident  "$Header$"
/* Error returned by getdate(3C)	*/
#define GETDATE_ERR		7

/* WARNING: uid %d is reserved. */
#define M_RESERVED		0

/* WARNING: more than NGROUPS_MAX(%d) groups specified. */
#define M_MAXGROUPS	1

/* ERROR:invalid syntax.\nusage: useradd [-u uid [-o] [-i] ] [-g group] [-G group[[,group]...]] [-d dir] \n               [-s shell]  [-c comment]  [-m [-k skel_dir]] [-f inactive] \n               [-e expire] [-p passgen%s%s] login\n */
#define M_AUSAGE		2

/* ERROR: Invalid syntax.\nusage:  userdel [-r] [-n months] login */
#define M_DUSAGE		3

/* ERROR: Invalid syntax.\nusage:   usermod [-u uid [-o][-U]] [-g group] [-G group[[,group]...]] [-d dir] [-m] \n               [-s shell]  [-c comment]  [-l new_logname] login */
#define M_MUSAGE		4

/* ERROR: Unexpected failure.  Defaults unchanged. */
#define M_FAILED	5

/* ERROR: Unable to remove files from home directory. */
#define M_RMFILES	6

/* ERROR: Unable to remove home directory. */
#define M_RMHOME		7

/* ERROR: Cannot update system files - login cannot be %s. */
#define M_UPDATE		8

/* ERROR: uid %d is already in use.  Choose another. */
#define M_UID_USED	9

/* ERROR: %s is already in use.  Choose another. */
#define M_USED	10

/* ERROR: %s does not exist. */
#define M_EXIST	11

/* ERROR: %s is not a valid %s.  Choose another. */
#define M_INVALID		12

/* ERROR: %s is in use.  Cannot %s it. */
#define M_BUSY	13

/* WARNING: %s has no permissions to use %s. */
#define M_NO_PERM	14

/* ERROR: There is not sufficient space to move %s home directory to %s */
#define M_NOSPACE		15

/* ERROR: %s %d is too big.  Choose another. */
#define	M_TOOBIG	16

/* ERROR: group %s does not exist.  Choose another. */
#define	M_GRP_NOTUSED	17

/* ERROR: Unable to %s: %s */
#define	M_OOPS	18

/* ERROR: %s is not a full path name.  Choose another. */
#define	M_RELPATH	19

/* "ERROR: invalid argument specified with -p flag\n" */
#define M_BAD_PFLG	20

/* "ERROR: invalid audit event type or class specified.\n"	*/
#define	M_BADMASK	21

/* "ERROR: invalid security level specified.\n", */
#define	M_INVALID_LVL	22

/* "ERROR: invalid default security level specified.\n", */
#define	M_INVALID_DLVL	23

/* "ERROR: invalid option -a\n", */
#define	M_NO_AUDIT	24

/* "ERROR: invalid options -h\n", */
#define	M_NO_MAC_H	25

/* "ERROR: system service not installed.\n", */
#define	M_NO_SERVICE	26

/* "ERROR: cannot delete security level %s.\n                Current default security level will become invalid.\n"
*/
#define	M_NO_DEFLVL	27
/* "ERROR: Invalid syntax.\nusage:   usermod [-u uid [-o][-U]] [-g group] [-G group[[,group]...]]\n   	 [-d dir] [-m] [-s shell]  [-c comment] \n  
          [-l new_logname] [-f inactive] [-e expire]\n               [-h [operator]]
level[,..]] [-v def_level] [-a[operator]event[,..]]   login\n"    */

#define	M_MUSAGE1	28

/* "ERROR: %s is the primary group name.  Choose another.\n" */
#define	M_SAME_GRP	29

/* "ERROR: invalid security level specified for user's home directory.\n", */
#define	M_INVALID_WLVL	30

/* "WARN: user's home directory already exists, -w ignored.\n", */
#define M_INVALID_WOPT  31

/* "ERROR: invalid months value specified for uid aging.\n", */
#define M_INVALID_AGE  	32

/* "ERROR: uid %d not aged sufficiently. Choose another.\n" */
#define M_UID_AGED	33

/* "ERROR: unable to access ``%s''\n"	*/
#define M_NOACCESS	34

/* "ERROR: The DATEMSK environment variable is null or undefined.\n" */
#define M_GETDATE	35

/* Error numbers 36-40 are reserved for getdate(3C) errors. 	*/

/* "WARNING: A logname of %s can cause produce unexepected results when used with other commands on the system\n" */
#define	M_NUMERIC	41

/* "ERROR: invalid options -v\n", */
#define	M_NO_MAC_V	42

/* "ERROR: invalid options -w\n", */
#define	M_NO_MAC_W	43

/* WARNING: more than NGROUPS_MAX(%d including basegid) groups specified. */
#define M_NGROUPS_MAX  	44

 /* "ERROR: invalid option usage for NIS user\n" */
#define M_NIS_INVALID 45

 /* "ERROR: unable to contact NIS \n" */
#define M_NO_NIS 46

 /* "ERROR: unable to find user in NIS map \n" */
#define M_NO_NISMATCH 47

 /* "ERROR: unknown NIS error \n" */
#define M_UNK_NIS 48

 /* "WARNING: unable to chown all files to new uid\n" */
#define M_CHOWN_FAIL 49

/* WARNING: gid %d is reserved.\n */
#define M_GID_RESERVED     50

/* ERROR: invalid syntax.\nusage:  groupadd [-g gid [-o]] [-K path] group\n */
#define M_GROUPADD_USAGE        51

/* ERROR: invalid syntax.\nusage:  groupdel [-K path] group\n */
#define M_GROUPDEL_USAGE        52

/* ERROR: invalid syntax.\nusage:  groupmod [-g gid [-o]] [-n name] [-K path] group\n */
#define M_GROUPMOD_USAGE        53

/* ERROR: Cannot update system files - group cannot be modified.\n */
#define M_GROUP_UPDATE      54

/* ERROR: %s is not a valid group id.  Choose another.\n */
#define M_GID_INVALID   55

/* ERROR: %s is already in use.  Choose another.\n */
#define M_GRP_USED  56

/* ERROR: %s is not a valid group name.  Choose another.\n */
#define M_GRP_INVALID   57

/* ERROR: %s does not exist.\n */
#define M_NO_GROUP  58

/* ERROR: Group id %d is too big.  Choose another.\n */
#define M_GROUP_TOOBIG    59

/* ******************************************************************** */

/* ERROR: invalid syntax.\nusage:  userls -ggroup,...|-a|-d [-o] [-S system_name] [-x extendedOptionString] [-X optionsFile] \n */
#define M_USERLS_USAGE	60

/* ERROR: invalid syntax.\nusage:  groupls -ggroup,...|-a|-d [-o] [-S system_name] [-x extendedOptionString] [-X optionsFile] \n */
#define M_GROUPLS_USAGE  61

/* Duplicated user Id's (pw_uid) are not allowed.\n */
#define M_DUPUID	62

/* Obtaining information from other systems is not supported.\n */
#define M_SYSNAME	63

/* Duplicated group Id's (gr_gid) are not allowed.\nSee group(4) for further details.\n */
#define M_DUPGRP    64

/* %s is not a valid user name.  Choose another.\n */
#define M_USER_INVALID	65

/* usage: addgrpmem -g group login [login ...]\n */
#define M_ADDGRPMEM_USAGE 66

/* unable to stat /etc/group\n */
#define M_UNABLE_TO_STAT 67

/* unable to open /etc/group\n */
#define M_UNABLE_TO_OPEN_GROUP 68

/* unable to open tmp file needed to modify /etc/group\n */
#define M_UNABLE_TO_OPEN_TMP 69

/* %s is not a valid login\n */
#define M_INVALID_LOGIN 69

/* /etc/group contains bad entries -- it was not modified.\n */
#define M_BAD_GROUP_ENTERIES 70

/* problem renaming temporary file \n */
#define M_RENAME_TMP_FILE 71

/* Group file busy.  Try again later\n */
#define M_GROUP_BUSY 72

/*invalid syntax:\n usage: getprimary group\n */
#define M_GETPRIMARY_USAGE 73

/* failed argvtostr()\n */
#define M_FAILED_ARGVTOSTR 74

/* NIS not available\n */
#define M_NIS_UNAVAILABLE 75

/* %s not found in NIS group map\n */
#define M_NIS_GROUPMAP 76

/* Unknown NIS error\n */
#define M_NIS_UNKNOWN 77

/* synopsis: expdate date\n */
#define M_EXP_DATE 78

/* Unable to change ownership of home. directory %s \n */
#define M_OWNERSHIP 79

/* Unable to create the home directory. %s \n */
#define M_HOME_DIR 80

/* Unable to fork.  Try again later. %s \n */
#define M_NO_FORK 81

/* Unable to find source directory. %s \n */
#define M_NO_SOURCE_DIR 82

/* Unable to change directory (cd) to source directory. %s \n */
#define M_UNABLE_TO_CD 83

/* Unable to chlvl new files \n */
#define M_UNABLE_TO_CHLVL 84

/* Unable to chown new home directory. %s \n */
#define M_CHOWN_NEW 85

/* Unable to chgrp new home directory. %s \n */
#define M_CHGRP_NEW 86

/* Unable to chmod home directory. %s \n */
#define M_CHMOD_NEW 87

/* New home directory is a sub-directory of the old one. Please clean up the old home directory\n */
#define M_NEW_HOME 88

/* Unexpected failure.  Uid aging file(s) missing\n */
#define M_UID_AGING_MISSING 89

/* Unexpected failure.  Uid aging files unchanged\n */
#define M_UID_AGING_UNCHANGED 90

/* bad entry found in /etc/security/ia/ageduid.\n */
#define M_BADENTRY_IN_AGEDUID 91

/* Unexpected failure.  Password file(s) missing\n */
#define M_PASSWD_MISSING 92

/* Password file(s) busy.  Try again later\n */
#define M_PASSWD_BUSY 93

/* Unexpected failure.  Password files unchanged\n */
#define M_PASSWD_UNCHANGED 94

/* Bad entry found in \"%s\".  Run pwconv.\n */
#define M_BADENTRY 95

/* Bad entry found in audit file.  Run auditcnv.\n */
#define M_AUDIT_BADENTRY 96

/* New password entry too long */
#define M_NEW_PASSWD_ENT_TO_LONG 97

/* %s not found in NIS passwd map\n */
#define M_NIS_PASSWDMAP 98

/* Unexpected failure, errno = %d.  Password files unchanged\n */
#define M_PASSWD_UNCHANGED_WITH_ERRNO 99

/* Bad entry found in \"%s\".\n" */
#define M_NW_BADENTRY 100

/* Inconsistent password files\n */
#define M_PASSWD_INCON 101

/* \"%s\" name does not exist\n */
#define M_NAME_NOT_EXIST 102

/* cannot unlink %s  \n*/
#define M_CANNOT_UNLINK 103

/* Unable to remove home directory. %s \n */
#define M_UNABLE_TO_REMOVE_HOME 104

/* Unable to allocate space for uid list. \n */
#define M_UNABLE_TO_ALLOC_UID_LIST 105

/* Unable to contact NIS, Reason %s \n */
#define M_UNABLE_TO_CONTACT_NIS 106

/* Unable to get NIS Domain \n */
#define M_UNABLE_TO_GET_NIS_DOMAIN 107

/* Extended options file %s does not exist.\n */
#define M_XTENDED_OPT_FILE_DOES_NOT_EXIST 108

/* Invalid extended option %s.\n */
#define M_INVALID_XTENDED_OPT	109

/* Extended options processing - Cannot find the operator.\n */
#define M_EXTENDED_OPT_ERROR_2 110

/* Extended options processing - Memory allocation error.\n */
#define M_EXTENDED_OPT_ERROR_3 111

/* Extended options processing - Cannot find end of brace for structure value.\n */
#define M_EXTENDED_OPT_ERROR_4 112

/* Extended options processing - Internal error.\n */
#define M_EXTENDED_OPT_ERROR_5 113

/* Extended options processing - Unmatched single quote (\').\n */
#define M_EXTENDED_OPT_ERROR_6 114

/* Extended options processing - Unmatched double quote (\").\n */
#define M_EXTENDED_OPT_ERROR_7 115

/* Internal Error */
#define M_INTERNAL_ERROR 116

/* Cells are not supported \n */
#define M_CELLNAME 117

/* This option is not supported at this time \n */
#define M_UNSUPPORTED_OPTN 118

/* Unable to exec script\n */
#define M_NO_EXEC 119

/* Path name too long\n */
#define	M_PATH_TOO_LONG 120

/* ERROR: Cannot update system files - group cannot be created\n */
#define M_GROUP_CREATE      121

/* ERROR: Cannot update system files - group cannot be deleted\n */
#define M_GROUP_DELETE      122

/* ERROR: Cannot copy `skeldir' to `homedir'\n */
#define M_COPY_SKELDIR      123

/* ERROR: login name contains disallowed characters\n */
#define M_LOGIN_SYNTAX		124
