

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/systeminfo.h>

#include <signal.h>

#include <sys/errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

#include <syslog.h>
#include <stdlib.h>


#define SOCKNAME "imap-4"
#define PROTOCOL "tcp"
#define SOCKTYPE SOCK_STREAM

#define IMAP_PORT 143


int             FileDescriptor; /* Value returned from "socket()" */

int		acceptID;

struct sockaddr_in SocketID;    /* Used to get remote socket info */
struct sockaddr From;
struct servent *ServiceNum;
int len;

int	userCount = 0;

void catch_child ()
{

  int             wait_stat;

/*

  pid_t pid;
  register struct servtab *sep;
  extern char *sys_siglist[];

        for (;;) {
                union wait status;
                pid = wait3(&status, WNOHANG, (struct rusage *)0);
                if (pid <= 0)
                        break;
       }


*/

  waitpid (0, &wait_stat, WNOHANG);
  syslog (LOG_ALERT,"Caught my child");
  signal (SIGCHLD, catch_child);

  userCount--;


}




check_license () 
{

   int port;
   int Errval ;
   int status;
   int errno;
   int result;
   int usersConfiged;

   char * userArray;
/*
   struct sigvec sv;
*/


  /*
   *Create binding
   */


   if ((ServiceNum = getservbyname (SOCKNAME, PROTOCOL)) == NULL)
      port = IMAP_PORT;
   else
      port = ntohs (ServiceNum -> s_port);


  FileDescriptor = socket (AF_INET, SOCKTYPE, 0); /* select auto configure */

  if (FileDescriptor == -1)
    {
      syslog(LOG_ALERT,"Error Creating Socket");
      exit (3);
    }

  /* Set the Values to bind to specific <host.port> entity */

  bzero (&SocketID, sizeof (struct sockaddr_in));


  SocketID.sin_family = AF_INET;/* This values is ALWAYS the same */
  SocketID.sin_port = htons (port);     /* Bind to a specific Port */
  /* The port must be in network byte order */
  SocketID.sin_addr.s_addr = INADDR_ANY;        /* any host is ok */

  Errval = bind (FileDescriptor, (char *) &SocketID, sizeof (SocketID));

  if (Errval != 0 )
      {
      perror ("Error Creating Socket\n");

      syslog (LOG_ALERT,"bind has failed: %d",errno);

      exit (3);
      }

  /*
   * End of binding
   */



  /*
   * Lets look like a deamon...
   */ 
  if (fork ())
     exit (0);


  /*
   * Set up signal handler for our children...
   */

  signal (SIGINT, SIG_IGN);
  signal (SIGHUP, SIG_IGN);
  signal (SIGQUIT, SIG_IGN);
  signal (SIGCHLD, catch_child);

/*
  sv.sv_handler = catch_child;
  sigvec(SIGCHLD, &sv, (struct sigvec *)0);
*/

  if (listen (FileDescriptor, 5) < 0)
    syslog(LOG_INFO,"listen failed:");


  for (;;)
    {

    while (waitpid (0, &status, WNOHANG) > 0)
      ;

    acceptID = accept (FileDescriptor, &From, &len);

    if (acceptID  < 0)
        continue;

   if (sysinfo(SI_USER_LIMIT,userArray, sizeof(userArray)) < 0 )
     syslog(LOG_INFO,"Could not determine users configured into system");
   else {

	if ( sscanf(userArray,"%d",&usersConfiged) == 1 ) {
	  if (userCount == usersConfiged )
             syslog(LOG_INFO,"User count exceeded");
	    }

	 }

   userCount++;

   if (fork () == 0)
     {
       close (FileDescriptor);
       close (2);
       dup(acceptID);
       close(1);
       dup(acceptID);
       close(0);
       dup(acceptID);
       return; /* Do the work */
     }                   /* End of if (fork) statement  */

     close (acceptID);

   }                       /* End of for (;;) statement  */

}


