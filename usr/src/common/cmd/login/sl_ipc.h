/*		copyright	"%c%" 	*/

#ident	"@(#)sl_ipc.h	1.2"
#ident  "$Header$"

typedef unsigned char			uint8;

#define NWMAX_OBJECT_NAME_LENGTH		0x30
#define NWMAX_PASSWORD_LENGTH			0x80
#define MAX_MESSAGE_SIZE				256

#define DEAMON_NOT_RUNNING				-2
#define	MUNGE_KEY						'H'

#define REGISTER_USER					1
#define UNREGISTER_USER					2
#define AUTHENTICATE_USER				3


#define Munge( _a, _len )	do{ \
								int i; \
								for( i=0; i<(int)_len; i++ ){ \
									((uint8*)(_a))[i] ^= MUNGE_KEY; \
								} \
							}while( FALSE )


typedef struct{
	uid_t				uid;
	gid_t				gid;
	char				userName[NWMAX_OBJECT_NAME_LENGTH];
	char				password[NWMAX_PASSWORD_LENGTH];
} SL_USER_INFO_T;

typedef struct{
	int					cmd;
	int					rc;
	pid_t				pid;
	SL_USER_INFO_T		userInfo;
} SL_CMD_T;

typedef struct{
	long				type;
	uint8				data[MAX_MESSAGE_SIZE];
} MSG_T;

int SLGetQID( int Queue, int Flag );
int SLRegisterUser( SL_USER_INFO_T* UserInfo );
int SLUnRegisterUser( uid_t uid );
int SLAuthenticateRequest();
int SLGetRequest( SL_CMD_T* Cmd );
int SLSendReply( SL_CMD_T* Cmd );
