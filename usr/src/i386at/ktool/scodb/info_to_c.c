#ident	"@(#)ktool:i386at/ktool/scodb/info_to_c.c	1.2"
#include <stdio.h>

#define SYMBUFSIZE 64
int
main () {
FILE	*f;
unsigned long	buf[SYMBUFSIZE];
long	c,got;

	
	f = fopen("kstruct.info","r");
	printf("unsigned long scodb_kstruct_info[] = {");
	while((got = fread(buf,sizeof(long),SYMBUFSIZE,f)) != 0) {
		for(c=0;c <got;c++)
			printf("0x%lx,",buf[c]);
		printf("\n");
	}
	fclose(f);
	printf("};\n");
}
		

