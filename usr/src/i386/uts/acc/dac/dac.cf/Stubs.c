#ident	"@(#)Stubs.c	1.2"
#ident	"$Header$"

int dac_installed = 0;

int aclipc() { return nopkg(); }
int ipcaclck() { return noreach(); }
int acl() { return nopkg(); }
int acl_getmax() { return 0; }
int acl_valid() { return nopkg(); }
