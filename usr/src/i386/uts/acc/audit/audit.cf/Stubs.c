#ident	"@(#)Stubs.c	1.2"
#ident	"$Header$"

#include <sys/audit.h>
#include <sys/ksynch.h>
#include <sys/time.h>

actlctl_t adt_ctl;
event_t	  adtd_event;
crseq_t   *crseqp;
fspin_t   crseq_mutex;

void adt_admin(unsigned short event, int status, int nentries, void *tbl) {}
void adt_allocaproc(aproc_t *padtp, proc_t *prp) {}
void adt_attrupdate(void) {}
void adt_auditchk(int scall, int *argp) {}
void adt_exit(int why) {}
void adt_filenm(struct pathname *apnp, struct vnode *vp, int error, uint_t cnt){}
void adt_free(alwp_t *alwp) {}
void adt_freeaproc(struct proc *pp) {}
void adt_getbuf(struct alwp *alwp, size_t size) {}
void adt_logoff(void) {}
void adt_lwp_exit(void) {}
void adt_modload(int status, struct modctl *mcp) {}
void adt_ola(struct vnode *fdvp, struct vnode *sdvp, struct vnode *vp) {}
void adt_parmsset(unsigned short event, int status, long upri, long uprisecs) {}
void adt_record(int scall, int status, int *uap, union rval *rvp) {}
void adt_recvfd(struct adtrecvfd *recvfdp, struct vnode *vp, struct vattr *vap) {}
void adt_stime(int status, timestruc_t *timep) {}
void adt_symlink(struct vnode *dvp, struct pathname *lpn, caddr_t path, int error) {}
void adt_pathupdate(struct proc *pp, struct vnode *vp, struct vnode **opvp, struct vnode **olvp, uint_t flags) {}
void adt_wrfilerec(struct arecbuf *recp, struct vnode *vp) {}

struct aproc *adt_p0aproc() { return(NULL);}
int  adt_chk_ola(struct alwp *alwp, lid_t lid) { return(0); }
void adt_pckill(pid_t pid, int sig, int err) {}
int  adt_installed() { return(0); }
int  auditbuf() { return nopkg(); }
int  auditlog() { return nopkg(); }
int  auditdmp() { return nopkg(); }
int  auditctl() { return nopkg(); }
int  auditevt() { return nopkg(); }
