#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
main()
{
	int fd,result;
	extern char 	*getenv();
	char *ptr;
	char file_name[1024];
	pid_t ipid ;

	ipid = getpid() ;
	setpgid(ipid,ipid) ;

	ptr = getenv("TET_SUITE_ROOT");
	if(ptr){
		/* get TET_SUITE_ROOT from environment */
		strcpy(file_name,ptr);
		strcat(file_name,"/bin/Uwcert_Check");
	}
	else {
		printf("Error: TET_SUITE_ROOT not defined in environment\n");
		exit(1);
	}
	fd=open(file_name,O_RDWR|O_CREAT,0755);
	if (fd < 0 ) {
		(void)printf("Creation of %s Failed\n",file_name);
		exit(1);
	}
	else { 
		result=lockf(fd,F_TLOCK,0);
		if (result < 0) {
			(void)fprintf(stderr,"Another Invocation Of UnixWare Certification Tests in Progress.\n Cannot Start This Session\n Press Enter To Continue");
			getchar();	
			exit(2);
		}
		else {
			(void)system("mainmenu");
		}
	}
	exit(0);
}
