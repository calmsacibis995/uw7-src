#include <stdio.h>
#include <tet_api.h>

void (*tet_startup)()=NULL;
void (*tet_cleanup)()=NULL;

void tp1();

struct tet_testlist tet_testlist []=
{{tp1,1},
{NULL,NULL}};

static char null[]={"(null)"};

char * null_chk(s)
	char *s;
{
	if (s) return s;
	else return null;
}


char * getvar(s)
	char * s;
{
	char * t;
	t=tet_getvar(s);

	if (t) return t;
	else return null;
}


void tp1()
{
	char buff[512];
	extern void sleep();
	
	printf("ctest7: ic1 tp1\n");
	tet_infoline("ctest7: ic1 tp1");
	tet_result(TET_UNRESOLVED);
	while (1) {
		sleep(1);
	}
}

