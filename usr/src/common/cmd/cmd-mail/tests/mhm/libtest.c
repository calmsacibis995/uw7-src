/*
 * test program for libmultihome.a
 * used by the test suite script (lib)
 */

#include <stdio.h>
#include <multihome.h>

main(argc, argv)
char **argv;
{
	char *cp;
	char *retcp;
	int ret;
	char buf[1024];
	int flag;

	argc--;
	argv++;
	cp = *argv;
	if (*cp != '-') {
		usage();
	}
	if (cp[1] == 'u')
		flag = 0;
	else if (cp[1] == 'd')
		flag = 1;
	else if (cp[1] == 'n')
		flag = 2;
	else
		usage();
	argc--;
	argv++;

	while (argc > 0) {
		if (flag == 0) {
			/* virtual users */
			strcpy(buf, *argv);
			cp = (char *)strchr(buf, '@');
			if (cp == 0) {
				printf("Invalid address %s\n", buf);
				exit(1);
			}
			*cp++ = 0;
			retcp = mhome_user_map(buf, cp);
			if (retcp) {
				printf("%s@%s %s\n", buf, cp, retcp);
			}
			else {
				printf("%s@%s <nofind>\n", buf, cp);
			}
		}
		else if (flag == 1) {
			/* virtual domains */
			strcpy(buf, *argv);
			cp = mhome_virtual_domain_ip(buf);
			if (cp) {
				printf("%s %s\n", buf, cp);
			}
			else {
				printf("%s <nofind>\n", buf);
			}
		}
		else if (flag == 2) {
			/* virtual domain names from ip */
			strcpy(buf, *argv);
			cp = mhome_ip_virtual_name(buf);
			if (cp) {
				printf("%s %s\n", buf, cp);
			}
			else {
				printf("%s <nofind>\n", buf);
			}
		}
		argc--;
		argv++;
	}
	if (flag == 0)
		mhome_user_map_close();
	else if (flag == 1)
		mhome_virtual_domain_close();
	exit(0);
}

usage()
{
	printf("libtest -u/-d/-n key...\n");
	printf("    look up either users or domains (ip's) or domain names\n");
	exit(1);
}
