#include <stdio.h>
#include <tet_api.h>

void startup();
void cleanup();
void (*tet_startup)()=startup;
void (*tet_cleanup)()=cleanup;

void tp1();
void tp2();
void tp3();

struct tet_testlist tet_testlist []=
{{tp1,1},
{tp2,2},
{tp3,2},
{NULL,NULL}};

void startup(){
	printf("ctest1: startup\n");
	tet_infoline("ctest1: startup");
}

void cleanup(){
	printf("ctest1: cleanup\n");
	tet_infoline("ctest1: cleanup");
}


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
	printf("ctest1: ic1 tp1\n");
	tet_infoline("ctest1: ic1 tp1");
	sprintf(buff,"ctest1: This tp is tp%d, test name is %s",
		tet_thistest,null_chk(tet_pname));
	printf("%s\n",buff);
	tet_infoline(buff);
	tet_result(TET_PASS);
}

void tp2()
{
	printf("ctest1: ic2 tp2\n");
	tet_infoline("ctest1: ic2 tp2");
	tet_result(TET_FAIL);
}

void tp3(){
	printf("ctest1: ic2 tp3\n");
	tet_infoline("ctest1: ic2 tp3");
	tet_result(TET_UNRESOLVED);
}

