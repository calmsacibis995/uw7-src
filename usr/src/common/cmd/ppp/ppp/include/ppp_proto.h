#ifndef _PPP_PROTO_H
#define _PPP_PROTO_H

#ident	"@(#)ppp_proto.h	1.2"

#include "act.h"
#include "psm.h"

void *timeout(int, int (*)(), caddr_t, caddr_t);

db_t *db_alloc(int);
db_t *db_dup(db_t *);
void db_free(db_t *);

struct db_s *mk_cfg_req(struct proto_hdr_s *, uchar_t, db_t *);
struct db_s *mk_trm(struct proto_hdr_s *, uchar_t, uchar_t, db_t *);
struct db_s *mk_cd_rej(struct proto_hdr_s *, uchar_t, db_t *);

char *ucfg_str(struct cfg_hdr *, unsigned int);
char *ucfg_get_element(char *s, char *d);

uchar_t *MD5String(uchar_t *, uchar_t);

/*struct act_hdr_s *act_findout(char *outId);*/
int act_find_id(char *Id, int type, act_hdr_t **ahp);

struct proto_hdr_s *act_findproto(act_hdr_t *ah, ushort_t proto, int datagram);

struct atomic_int_s * ATOMIC_INT_ALLOC();
void ATOMIC_INT_DEALLOC(struct atomic_int_s *at);
void ATOMIC_INT_INCR(struct atomic_int_s *at);
void ATOMIC_INT_DECR(struct atomic_int_s *at);
int ATOMIC_INT_READ(struct atomic_int_s *at);

#endif /* _PPP_PROTO_H */
