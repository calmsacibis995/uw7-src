#ident	"@(#)dl_sap.c	1.2"
/*	
 * Copyright (c) 1994 Novell, Inc. All Rights Reserved.	
 *
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
 * The copyright notice above does not evidence any 
 * actual or intended publication of such source code.
 *
 * This work is subject to U.S. and International copyright laws and
 * treaties.  No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */
/*********************************************************************
 * File Description:  
 *      This file contains routines to allow an application to 
 *      set sapping on/off for the following sap types.
 *
 *         Remote Applications
 *         Install Server 
 *         UnixWare
 *
 *      Also routines are supplied that interface with the
 *      Sap API's to allow an application to determine if
 *      the system is sapping, determine if a certain sap is
 *      being sent and to get a list of servers for a given
 *      sap type;
 *
 *      Functions to enable/disable sapping:
 *
 *          enableInstallSAP(),    disableInstallSAP();
 *          enableUnixWareSAP(),   disableUnixWareSAP();
 *          enableRemoteAppsSAP(), disableRemoteAppsSAP();
 *
 *      Functions to do sap queries:
 * 
 *          isSystemSapping(void);
 *          isSystemSappingAType(const int sapType);
 *          getSappingServerList(const int sapType, char ** list, 
 *                                int * listCount);
 *
 *     All functions return NULL on success and a character message
 *     indicating the error condition on failure.
 *
 *     Use -DSTAND_ALONE to compile this file for stand alone testing.
 *
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>     /* strdup etc. */
#include <sys/utsname.h>
#include "dl_msgs.h"

#ifndef STAND_ALONE

#include <sys/sap_app.h>
#include "nwconfig.h"

#else

typedef struct sapi {
	unsigned int serverType;
	unsigned char serverName[48];
}SAPI;
#define NWCP_BOOLEAN "int"
#define NWCM_SUCCESS 0
#define ALL_SERVER_TYPE 0
int SAPGetServerByName(char * systemName,int type,
	unsigned int *ServerEntry,SAPI *ServerBuf, int MaxEntries);
int SAPGetAllServers(int sapType, unsigned int *ServerEntry,
		SAPI *ServerBuf,int MaxEntries);
int NWCMSetParam(char *tokenName,char * tokenType,void *i);
int NWCMGetParam(char *tokenName,char *tokenType,void *result );

#endif

/* 
 * Public Functions to enable a sap service 
 */
char * enableInstallSAP();
char * enableUnixWareSAP();
char * enableRemoteAppsSAP();
/* 
 * Public Functions to disable a sap service 
 */
char * disableInstallSAP();
char * disableUnixWareSAP();
char * disableRemoteAppsSAP();
/* 
 * Public Functions for sap service queries
 */
char * isSystemSapping(void);
char * isSystemSappingAType( const int sapType);
char * getSappingServerList(int sapType, char ** list, int * listCount);

/* 
 * Private Functions prototypes to do sap service manipulation 
 */
static char * getTokenFromConfig(const char *tokenName,int *result);
static char * setTokenInConfig(const char *tokenName);
static char * clearTokenInConfig(const char *tokenName);
static int    isDaemonActive(void);
static char * advertiseMyType(int sapType );
static char * unAdvertiseMyType(int sapType );
static char * GetStr (char *idstr);
static char * GetSystemName(char **name);

/*
 * Local Constants
 */
static const char *REMOTE_APPS    = "sap_remote_apps";
static const char *INSTALL_SERVER = "sap_install_server";
static const char *UNIXWARE       = "sap_unixware";

enum {TYPE_REMOTE_APPS    = 0x3e1, 
      TYPE_INSTALL_SERVER = 0x3e2,
      TYPE_UNIXWARE       = 0x3e4 };

#define  MAX_SERVERS_AT_ONCE   256



/*---------------------------------------------------------------*/
/* Public Functions to enable a sap service                      */
/*---------------------------------------------------------------*/
char * 
enableInstallSAP()
{
	char *errMsg;
	int ret;
	uid_t uid;

	if ( isSystemSappingAType(TYPE_UNIXWARE) != NULL )
	{
		return(GetStr( TXT_peerToPeerNotEnabled ));
	}
	uid = getuid();
	setuid(0);
	if ((errMsg = setTokenInConfig(INSTALL_SERVER)) == NULL)
		errMsg = advertiseMyType(TYPE_INSTALL_SERVER);
	setuid(uid);
	return(errMsg);
}
char *
enableUnixWareSAP()
{
	char *errMsg;
	int ret;
	uid_t uid;

	uid = getuid();
	setuid(0);
	errMsg = setTokenInConfig(UNIXWARE);
	if ((ret =  EnableIpxServerMode()) != 0 )
	{
		setuid(uid);
		return(GetStr( TXT_enableSapServiceErr ));
	}
	errMsg = advertiseMyType(TYPE_UNIXWARE);
	setuid(uid);
	return(errMsg);
}
char * 
enableRemoteAppsSAP()
{
	char *errMsg;
	int ret;
	uid_t uid;

	if ( isSystemSappingAType(TYPE_UNIXWARE) != NULL )
	{
		return(GetStr( TXT_peerToPeerNotEnabled ));
	}
	uid = getuid();
	setuid(0);
	if ((errMsg = setTokenInConfig(REMOTE_APPS)) == NULL)
		errMsg = advertiseMyType(TYPE_REMOTE_APPS);
	setuid(uid);
	return(errMsg);
}
/*---------------------------------------------------------------*/
/* Public Functions to disable a sap service                     */
/*---------------------------------------------------------------*/
char * 
disableInstallSAP()
{
	char *errMsg;
	uid_t uid;

	uid = getuid();
	setuid(0);
	errMsg = clearTokenInConfig(INSTALL_SERVER);
	if ( isDaemonActive() == TRUE )
	{
		errMsg = unAdvertiseMyType(TYPE_INSTALL_SERVER);
	}
	setuid(uid);
	return(errMsg);
}
char *
disableUnixWareSAP()
{
	char *errMsg;
	uid_t uid;

	disableRemoteAppsSAP();

	uid = getuid();
	setuid(0);
	errMsg = clearTokenInConfig(UNIXWARE);
	if ( isDaemonActive() == TRUE )
	{
		errMsg = unAdvertiseMyType(TYPE_UNIXWARE);
	}
	setuid(uid);
	return(errMsg);
}
char *
disableRemoteAppsSAP()
{
	char *errMsg;
	uid_t uid;

	uid = getuid();
	setuid(0);
	errMsg = clearTokenInConfig(REMOTE_APPS);
	if ( isDaemonActive() == TRUE )
	{
		errMsg = unAdvertiseMyType(TYPE_REMOTE_APPS);
	}
	setuid(uid);
	return(errMsg);
}

/*
 * Description:
 *
 *    The system name must be found in the SAP table
 *    and the nwconfig file must have the token
 *    UNIXWARE set to active for a successful return.
 */
char * 
isSystemSapping()
{
	int i;
	int ServerEntry = NULL;
	SAPI ServerBuf;
	int MaxEntries = 1;
	char *name;
	char *ret;

	if ( (ret = GetSystemName(&name)) != NULL )
	{
		return(ret);
	}
	i = SAPGetServerByName((uint8 *)name,ALL_SERVER_TYPE,
	                   &ServerEntry,&ServerBuf,MaxEntries);
	free(name);
	if ( i > 0 )
	{
		if ( ServerBuf.serverHops != 0 )
			return(GetStr( TXT_sysNotSapping ));
		getTokenFromConfig(UNIXWARE,&i);
		if ( i )             /* sap active */
		{
			return(NULL);
		}
	}
	return(GetStr( TXT_sysNotSapping ));
}
/*
 * Description:
 *
 *    The system name must be found in the SAP table
 *    and the nwconfig file must have the cooresponding
 *    token set to active for a successful return.
 */
char * 
isSystemSappingAType( const int sapType)
{
	int i;
	int ServerEntry = NULL;
	SAPI ServerBuf;
	int MaxEntries = 1;
	char *name;
	const char *token;
	char *ret;

	if ( (ret = GetSystemName(&name)) != NULL )
	{
		return(ret);
	}
	i = SAPGetServerByName((uint8 *)name,sapType,
	                   &ServerEntry,&ServerBuf,MaxEntries);
	free(name);
	if ( i > 0 )
	{
		if ( ServerBuf.serverHops != 0 )
			return(GetStr( TXT_sysNotSappingType ));
		switch(sapType)
		{
			case TYPE_REMOTE_APPS:
				token = REMOTE_APPS;
				break;
			case TYPE_INSTALL_SERVER:
				token = INSTALL_SERVER;
				break;
			case TYPE_UNIXWARE:
				token = UNIXWARE;
				break;
			default:
				return(GetStr( TXT_sysNotSappingType ));
		}
		getTokenFromConfig(token,&i);
		if ( i )             /* sap active */
		{
			return(NULL);
		}
	}
	return(GetStr( TXT_sysNotSappingType ));
}
/*
 * Description:
 *
 *    Get a list of servers from the  SAP table
 *    that match the argumented sapType.
 *
 *
 *    listCount     load with the amount of servers found
 *    list          load with names of servers found
 *
 *    Callee is responsible for freeing the server list space.
 */
char * 
getSappingServerList( const int sapType, char ** list, int * listCount)
{
	int i,x;
	int ServerEntry = NULL;
	int MaxEntries = MAX_SERVERS_AT_ONCE;
	SAPI ServerBuf[MAX_SERVERS_AT_ONCE];
	int ServerBufSize;
	char **temp;

	ServerBufSize = MaxEntries;
	*listCount = 0;
	temp = (char **)calloc((sizeof(char *)) * MaxEntries,1);
	*list = (char *)temp;
	do
	{
		i = SAPGetAllServers(sapType, &ServerEntry,ServerBuf,MaxEntries);
		if ( i <= 0 )
		{
			break;
		}
		for( x = 0 ; x < i ; x++ )
		{
			temp[*listCount] = (char *)
						strdup((const char *)ServerBuf[x].serverName);
			*listCount += 1;
		}
		if ( i == MaxEntries )
		{
			ServerBufSize += MaxEntries;
			temp =  (char **)realloc
						((char *)temp,sizeof (char *) * ServerBufSize);
			*list = (char *)temp;
		}
	}while ( i == MaxEntries );
	if ( *listCount > 0 )
		return(NULL);
	return(GetStr( TXT_noSappingServers ));
}
/*---------------------------------------------------------------*/
/* Start Private Functions to enable a sap service               */
/*---------------------------------------------------------------*/
/*
 * Description:
 *    Determine if the sapd daemon is active 
 */
static int    
isDaemonActive()
{
	struct  utsname  machine;
	char *pidLogDir;

	if (uname(&machine) < 0)
    {
		return(FALSE);
    }
	if( (pidLogDir = (char *)NWCMGetConfigDirPath()) == NULL)
	{
        return(FALSE);
	}
	if ( LogPidKill(pidLogDir,"sapd",0)  != 0 )
	{
		return(FALSE);
	}
	return(TRUE);
}
/*
 * Description:
 *    Inform the sapd daemon that it needs to start
 *    advertising a sap type
 */
static char *
advertiseMyType(int sapType )
{
	char *name;
	char *ret = NULL;

	if ( (ret = GetSystemName(&name)) != NULL )
	{
		return(ret);
	}
	if (( SAPAdvertiseMyServer(sapType,(uint8 *)name,
									0, SAP_ADVERTISE_FOREVER)) != 0 )
	{
		ret = GetStr( TXT_advertiseErrr );
	}
	free(name);
	return( ret );
}
/*
 * Description:
 *    Inform the npsd daemon that it needs to stop
 *    advertising a sap type
 */
static char *
unAdvertiseMyType(int sapType )
{
	char *name;
	char *ret = NULL;

	if ( (ret = GetSystemName(&name)) != NULL )
	{
		return(ret);
	}
	if (( SAPAdvertiseMyServer(sapType,(uint8 *)name, 
									0, SAP_STOP_ADVERTISING)) != 0 )
	{
		ret = GetStr( TXT_unAdvertiseErr );
	}
	free(name);
	return(ret );
}
/*
 * Description:
 *    Get the token's state
 */
static char * 
getTokenFromConfig(const char *tokenName,int *result)
{
	int i;
	
	i = NWCMGetParam((char *)tokenName,NWCP_BOOLEAN,result);
	if ( i != NWCM_SUCCESS )
		return(GetStr( TXT_tokenGetErr));
	return(NULL);
}
/*
 * Description:
 *    Set the token's state to the argumented value
 */
static char * 
setTokenInConfig(const char *tokenName)
{
	int i = 1;

	i = NWCMSetParam((char *)tokenName,NWCP_BOOLEAN,&i);
	if ( i != NWCM_SUCCESS )
		return(GetStr( TXT_tokenSetErr));
	return(NULL);
}
/*
 * Description:
 *    Clear the token's state to the argumented value
 */
static char * 
clearTokenInConfig(const char *tokenName)
{
	int i = 0;
	
	i = NWCMSetParam((char *)tokenName,NWCP_BOOLEAN,&i);
	if ( i != NWCM_SUCCESS )
		return(GetStr( TXT_tokenClearErr));
	return(NULL);
}
/* GetStr
 *
 * Get an internationalized string.  
 *
 * String id's contain both the filename:id
 * and default string, separated by the FS_CHR character.
 * ( Currently gettxt uses __gtxt which mmaps the locale message file so
 *   the returned pointer does not need to be freed and is non-volital 
 *   believe it or not. )
 */
static char *
GetStr (char *idstr)
{
    char    *sep;
    char    *str;

    sep = (char *)strchr (idstr, FS_CHR);
    *sep = 0;
    str = (char *)gettxt (idstr, sep + 1);
    *sep = FS_CHR;

    return (str);
}   /* End of GetStr () */

/* GetSystemName
 *
 * Get the system name, convert to uppercase and return.
 *
 */

static char * 
GetSystemName(char **name)
{
	struct  utsname  machine;
	int i;
	char *cp;

	if (uname(&machine) < 0)
    {
		return(GetStr( TXT_machineNameReadErr ));
    }
	cp = strdup(machine.nodename);
	/*
	 *  Upper case the name
	 */
    for(i=0; i < strlen(machine.nodename); i++)
        cp[i] = toupper(machine.nodename[i]);
	*name = cp;
	return(NULL);
}

#ifdef STAND_ALONE

/***********************************************************************/
/* test routines for STAND_ALONE 
/***********************************************************************/

/*
 * List of servers
 */
char *serverNames[] = {
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten",
" one","two","three","four","five","six","seven","eight","nine","ten"
};

main(int argc,char **argv)
{
	char **serverList = NULL;
	int serverListCount;
	int i;

	enableInstallSAP();
	enableUnixWareSAP();
	enableRemoteAppsSAP();
	enableInstallSAP();
	

	disableInstallSAP();
	disableUnixWareSAP();
	disableInstallSAP();
	disableRemoteAppsSAP();
	isSystemSapping();
	isSystemSappingAType(TYPE_UNIXWARE);
	getSappingServerList(TYPE_INSTALL_SERVER,
					(char **)&serverList,&serverListCount);
	for(i=0;i<10;i++)
	{
		printf("%s\n",*serverList);
		serverList++;
	}
}
/*
 *                    Stub of the real thing
 */
int 
SAPGetServerByName(char * systemName,int type,
	unsigned int *ServerEntry,SAPI *ServerBuf, int MaxEntries)
{
	strcpy((char *)ServerBuf->serverName,"WHITEHARE");
	return(0);
}
int 
SAPGetAllServers(int sapType, unsigned int *ServerEntry,
		SAPI *ServerBuf,int MaxEntries)
{
	static int pos = 0;
	int count;
	int sendCount;
	int i;

	count = sizeof(serverNames)/sizeof(char *);
	sendCount = count - pos;
	if ( sendCount > MaxEntries )
		sendCount = MaxEntries;
	for(i=0;i<sendCount;i++)
	{
		strcpy((char *)ServerBuf[i].serverName,serverNames[pos]);
		pos++;
	}
	if ( sendCount != MaxEntries )
		pos = 0;
	return(sendCount);
}
int
NWCMSetParam(char *tokenName,char * tokenType,void *i)
{
	return(0);
}
int
NWCMGetParam(char *tokenName,char *tokenType,void *result )
{
	*(int *)result = 1;
	return(0);
}
#endif
