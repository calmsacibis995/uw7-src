#ident	"@(#)Stubs.c	1.2"
#ident	"$Header$"

int mac_installed = 0;

int mac_init() { return 0; }
int mac_vaccess() { return 0; }
int mac_liddom() { return 0; }
int mac_lvl_ops() { return 0; }
int mac_lid_ops() { return 0; }
int mac_checks() { return 0; }
void lvls_clnr() {}
int mac_openlid() { return nopkg(); }
int mac_rootlid() { return nopkg(); }
int mac_adtlid() { return nopkg(); }

void cc_limit_all() {}
int cc_getinfo(){ return 0; }

int lvldom() { return nopkg(); }
int lvlequal() { return nopkg(); }
int lvlipc() { return nopkg(); }
int lvlproc() { return nopkg(); }
int lvlvfs() { return nopkg(); }

int mldmode() { return nopkg(); }
int mkmld() { return nopkg(); }
int mld_deflect() { return nopkg(); }

int devstat() { return nopkg(); }
int fdevstat() { return nopkg(); }
