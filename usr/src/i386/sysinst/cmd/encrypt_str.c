#ident  "@(#)encrypt_str.c	15.1	98/03/04"

#include <stdio.h>
#include <unistd.h>
#include <crypt.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

main(int argc, char **argv)
{
	int i, len;
	time_t salt;
	char c, *pwbuf, saltc[2];
	/* Old code - char pwbuf[10] */

	if(argc != 2) exit(1);
	len = strlen(argv[1]);
	pwbuf = (char *)malloc(len+1);
	strncpy(pwbuf, argv[1], len+1);

	/*
	 * Construct salt, then encrypt the new password.
	 */
	(void) time((time_t *)&salt);
	salt += (long)getpid();
/* salt=0; */

	saltc[0] = salt & 077;
	saltc[1] = (salt >> 6) & 077;
	for (i = 0; i < 2; i++) {
		c = saltc[i] + '.';
		if (c>'9') c += 7;
		if (c>'Z') c += 6;
		saltc[i] = (char) c;
	}
	printf("%s\n", bigcrypt(pwbuf, saltc));
}
