#ident	"@(#)util.c	1.3"

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <synch.h>
#include <time.h>
#include <sys/time.h>

#include "ppp_type.h"
#include "ppp_cfg.h"
#include "fsm.h"
#include "act.h"
#include "ppp_proto.h"
#include "ppp_util.h"
#include <sys/ppp.h>
#include <sys/ppp_psm.h>

int ppplog(int level, act_hdr_t *ah, char *fmt, ...);

db_t *
db_alloc(int size)
{
	db_t *db;

	db = (db_t *)malloc(sizeof(db_t));
	if (!db) {
		ppplog(MSG_WARN, 0, "db_alloc: No Memory");
		return db;
	}

	db->db_base = (unsigned char *)malloc(size + 1);
	if (!db->db_base) {
		free(db);
		ppplog(MSG_WARN, 0, "db_alloc: No Memory");
		return NULL;
	}
		
	db->db_lim = db->db_base + size;
	db->db_rptr = db->db_base;
	db->db_wptr = db->db_base;
	
	db->db_func = -1;
	db->db_lindex = -1;
	db->db_bindex = -1;
	db->db_ref = 1;
	db->db_band = 2; /* Default message priority */
	return db;
}

db_t *
db_dup(db_t *db)
{
	db->db_ref++;
	return db;
}

void	
db_free(db_t *db)
{
	db->db_ref--;
	if (db->db_ref > 0)
		return;

	if (db->db_base)
		free(db->db_base);

/* DEBUG */
	if (!db->db_base) {
		ppplog(MSG_WARN, 0,
		       "db_base = %x, lim %x, rptr %x, wptr %x\n", 
		       db->db_base, db->db_lim, db->db_rptr, db->db_wptr);

		ppplog(MSG_WARN, 0, "db_free: db %x, already free\n");
		ASSERT(0);
	} else
		db->db_base = 0;

	free(db);
}

/*
 * These mirror the kernel functions of the same name
 */
struct atomic_int_s *
ATOMIC_INT_ALLOC()
{
	atomic_int_t *at;

	at = (atomic_int_t *)malloc(sizeof(atomic_int_t));
	if (!at)
		return NULL;

	at->ref = 0;
	mutex_init(&at->lock, USYNC_PROCESS, NULL);
	return at;
}

void
ATOMIC_INT_DEALLOC(struct atomic_int_s *at)
{
	mutex_destroy(&at->lock);
	free(at);
}

void 
ATOMIC_INT_INCR(struct atomic_int_s *at)
{
	MUTEX_LOCK(&at->lock);
	at->ref++;
	MUTEX_UNLOCK(&at->lock);
}

void
ATOMIC_INT_DECR(struct atomic_int_s *at)
{
	MUTEX_LOCK(&at->lock);
	at->ref--;
	MUTEX_UNLOCK(&at->lock);
}

int
ATOMIC_INT_READ(struct atomic_int_s *at)
{
	int x;

	MUTEX_LOCK(&at->lock);
	x = at->ref;
	MUTEX_UNLOCK(&at->lock);
	return x;
}

display_transport(FILE *fp, act_hdr_t *ah)
{
	extern int pppd_debug;
	struct timeval tv;
	void *dummy;
	struct tm local_time;
	char tbuff[20];

	/* Get time */
	gettimeofday(&tv, dummy);

	/* Convert to local time */
	localtime_r(&tv.tv_sec, &local_time);

	/* Non debug, use MMM dd hh:mm:ss, debug use hh:mm:ss.hh */
	
	if (pppd_debug & DEBUG_PPPD) {
		strftime(tbuff, 20, "%T", &local_time);
		fprintf(fp, "%s.%2.2d : ", tbuff, tv.tv_usec/10000);
	} else {
		strftime(tbuff, 20, "%b %d %T : ", &local_time);
		fprintf(fp, "%s", tbuff);
	}

	if (!ah) {
		if (pppd_debug & DEBUG_PPPD)
			fprintf(fp, "          : ");
		return;
	}

	switch (ah->ah_type) {
	case DEF_BUNDLE:
		if (pppd_debug & DEBUG_PPPD)
			fprintf(fp, "B");
		else
			fprintf(fp, "Bundle");
		break;
	case DEF_LINK:
		if (pppd_debug & DEBUG_PPPD)
			fprintf(fp, "L");
		else
			fprintf(fp, "Link");
		break;
	}

	if (pppd_debug & DEBUG_PPPD)
		fprintf(fp, " %-7.7s : ", ah->ah_cfg->ch_id);
	else
		fprintf(fp, " %s : ", ah->ah_cfg->ch_id);
}

/*
 * Decide if a message should be 'displayed'
 */
display_msg(act_hdr_t *ah, int level)
{
	extern int user_debug, pppd_debug;

	if (pppd_debug & DEBUG_PPPD)
		return 1;

	if (user_debug && level != MSG_DEBUG)
		return 1;

	if (level >= MSG_WARN || level == MSG_INFO || level == MSG_AUDIT)
		return 1;

	switch(ah->ah_debug) {
	case DBG_NONE:	
		return 0;
	case DBG_LOW:
		if (level <= MSG_INFO_LOW)
			return 1;
		break;
	case DBG_MED:
		if (level <= MSG_INFO_MED)
			return 1;
		break;
	case DBG_HIGH:
	case DBG_WIRE:
		if (level <= MSG_INFO_HIGH)
			return 1;
		break;
	case DBG_DEBUG:
		return 1;
	}
	if (level == MSG_ULR)
		return 1;
	return 0;
}

int
ppplog(int level, act_hdr_t *ah, char *fmt, ...)
{
	extern FILE *log_fp;
	va_list ap;

	if (!display_msg(ah, level))
		return;

	display_transport(log_fp, ah);

	va_start(ap, fmt);
	PPPLOG_CMN(log_fp, level, fmt, ap);
	va_end(ap);	
}

STATIC 
adm_print(FILE *log_fp, act_hdr_t *ah, db_t *db, struct ppp_inf_dt_s *di)
{

	display_transport(log_fp, ah);
	switch (db->db_cmd) {
	case CMD_OPEN:
		fprintf(log_fp, "    CMD_OPEN ");
		switch (db->db_func) {
		case PCID_ADM_PSM:
		case PCID_ADM_NCP:
			break;
		case PCID_ADM_LINK:
		case PCID_ADM_BUNDLE:
			fprintf(log_fp, "(error = %d)", db->db_error);
			break;
		}
		fprintf(log_fp, "\n");
		break;
	case CMD_UP:
		fprintf(log_fp, "    CMD_UP ");
		fprintf(log_fp, "\n");
		break;
	case CMD_CLOSE:
		fprintf(log_fp, "    CMD_CLOSE ", db->db_error);
		switch (db->db_func) {
		case PCID_ADM_PSM:
		case PCID_ADM_NCP:
			break;
		case PCID_ADM_LINK:
		case PCID_ADM_BUNDLE:
			fprintf(log_fp, "(error = %d)", db->db_error);
			break;
		}
		fprintf(log_fp, "\n");
		break;
	case CMD_CFG:
		fprintf(log_fp, "    CMD_CFG\n");
		switch (db->db_func) {
		case PCID_ADM_LINK:
			display_transport(log_fp, ah);
			fprintf(log_fp, "    local mru = %d peer mru = %d\n",
				di->di_localmru, di->di_remotemru);
			display_transport(log_fp, ah);
			fprintf(log_fp,
				"    peer opts = 0x%4.4x bandwidth = %d\n",
				di->di_opts, di->di_bandwidth);
			display_transport(log_fp, ah);
			fprintf(log_fp,
				"    accm remote = 0x%x local = 0x%x\n",
				di->di_remoteaccm, di->di_localaccm);
 			break;
		case PCID_ADM_NCP:
			/*fprintf(log_fp, "\n",);*/
			break;
		case PCID_ADM_BUNDLE:
			display_transport(log_fp, ah);
			fprintf(log_fp, "    mtu = %d\n", di->di_mtu);
			break;
		case PCID_ADM_PSM:
			break;
		}
		break;
	case CMD_BIND:
		fprintf(log_fp, "    CMD_BIND (%s)\n",
			(PSM_FLAGS(db->db_proto) == PSM_RX ? "Rx" : "Tx"));
		display_transport(log_fp, ah);
		fprintf(log_fp, "    Id 0x%4.4x, Flags 0x%2.2x\n",
			PSM_PROTO(db->db_proto), PSM_FLAGS(db->db_proto));
		display_transport(log_fp, ah);
		fprintf(log_fp, "    Protocol 0x%4.4x, %d bytes of options\n",
			di->di_psm_proto, di->di_psm_optsz);
		break;
	case CMD_UNBIND:
		fprintf(log_fp, "    CMD_UNBIND (%s)\n",
			(PSM_FLAGS(db->db_proto) == PSM_RX ? "Rx" : "Tx"));
		display_transport(log_fp, ah);
		fprintf(log_fp, "    Id 0x%4.4x, Flags 0x%2.2x\n",
			PSM_PROTO(db->db_proto), PSM_FLAGS(db->db_proto));
		display_transport(log_fp, ah);
		fprintf(log_fp, "    Protocol 0x%4.4x\n", di->di_psm_proto);
		break;
	case CMD_ADDBW:
		fprintf(log_fp, "    CMD_ADDBW (error %d)\n", db->db_error);
		break;
	case CMD_REMBW:
		fprintf(log_fp, "    CMD_REMBW (error %d)\n", db->db_error);
		break;
	case CMD_STATS:
		fprintf(log_fp, "    CMD_STATS ");
		switch (db->db_func) {
		case PCID_ADM_NCP:
		case PCID_ADM_LINK:
		case PCID_ADM_BUNDLE:
			break;
		case PCID_ADM_PSM:
			fprintf(log_fp, "(protocol 0x%4.4x, flags %d)",
				PSM_PROTO(db->db_proto),
				PSM_FLAGS(db->db_proto));
			break;
		}
		fprintf(log_fp, "\n");
		break;
	case CMD_IDLE:
		fprintf(log_fp, "    CMD_IDLE\n");
		break;
	case CMD_ACTIVE:
		fprintf(log_fp, "    CMD_ACTIVE\n");
		break;
	case CMD_MSG:
		fprintf(log_fp, "    CMD_MSG\n");
		break;
	default:
		fprintf(log_fp, "    ?? CMD_ not known\n");
		break;
	}
}

int
ppplogdb(int level, act_hdr_t *ah, char *msg, db_t *db)
{
	extern FILE *log_fp;
	int i, cnt, j;
	uchar_t *p = db->db_base;
	struct ppp_inf_dt_s *di = (struct ppp_inf_dt_s *)db->db_base;

	if (!display_msg(ah, level))
		return;
	
	cnt = db->db_wptr - db->db_base;

	display_transport(log_fp, ah);

	switch (db->db_func) {
	case PCID_MSG:
		fprintf(log_fp, "%s PPP Frame %d bytes\n", msg, cnt);
		ppploghex(level, ah, db->db_base, cnt);
		break;

	case PCID_LOG:
		/*fprintf(log_fp, "DATA %d bytes\n", cnt);*/
		break;

	case PCID_ADD_L2B:
		fprintf(log_fp, "%s PCID_ADD_L2B (%d bytes)\n", msg, cnt);
		display_transport(log_fp, ah);
		fprintf(log_fp, "    Link index %d, Bundle index %d (error %d)\n",
			di->di_lindex, db->db_bindex, db->db_error);
		break;

	case PCID_ADM_LINK:
		fprintf(log_fp, "%s PCID_ADM_LINK (%d bytes)\n", msg, cnt);
		adm_print(log_fp, ah, db, di);
		break;

	case PCID_ADM_NCP:
		fprintf(log_fp, "%s PCID_ADM_NCP (%d bytes)\n", msg, cnt);
		adm_print(log_fp, ah, db, di);
		break;

	case PCID_ADM_BUNDLE:
		fprintf(log_fp, "%s PCID_ADM_BUNDLE (%d bytes)\n",
			msg, cnt);
		adm_print(log_fp, ah, db, di);
		break;
	case PCID_ADM_PSM:
		fprintf(log_fp, "%s PCID_ADM_PSM (%d bytes)\n",
			msg, cnt);
		adm_print(log_fp, ah, db, di);
		break;
	default:
		fprintf(log_fp, "%s CONTROL message %d\n", msg,
			db->db_func);
		break;
	}
	fflush(log_fp);
}


int
ppploghex(int level, act_hdr_t *ah, char *p, int cnt)
{
	extern FILE *log_fp;
	int i,  j;

	if (!display_msg(ah, level))
		return;
	
#define CHAR_LINE 16

	for (i = 0; i < cnt; i += CHAR_LINE) {

		display_transport(log_fp, ah);

		for (j = 0; j < CHAR_LINE; j++) {
			if (i + j < cnt) {
				fprintf(log_fp, "%2.2x",
					((int)*(p + j) & 0xff));
			} else {
				fprintf(log_fp, "..");
			}

			if (((j + 1) % 4) == 0)
				fprintf(log_fp, " ");
		}

		fprintf(log_fp, " ");

		for (j = 0; j < CHAR_LINE; j++) {
			if (i + j < cnt && isprint(*(p + j) & 0x7f)) {
				fprintf(log_fp, "%c", (*(p + j) & 0x7f));
			} else {
				fprintf(log_fp, ".");
			}
		}

		p += CHAR_LINE;
		fprintf(log_fp, "\n");
	}
	fflush(log_fp);
}

/* What value ??? should we use ???? */

#define MAX_RETRY 1800
int
MUTEX_LOCK(mutex_t *m)
{
	int i = 0, ret;

	while (i < MAX_RETRY) {
		ret = mutex_trylock(m);
		if (ret == 0)
			break;
		if (ret != EBUSY) {
			ppplog(MSG_INFO, 0, "mutex_trylock error %d\n", ret);
			ASSERT(0);
		}
		i++;
		nap(100);
	}

	if (ret) {
		ppplog(MSG_INFO, 0, "MUTEX_LOCK ... timeout\n");
		ASSERT(0);
	}
	return 0;
}

int
MUTEX_UNLOCK(mutex_t *m)
{
	mutex_unlock(m);
}

int
RW_WRLOCK(rwlock_t *l)
{
	int i = 0, ret;

	while (i < MAX_RETRY) {
		ret = rw_trywrlock(l);
		if (ret == 0)
			break;
		if (ret != EBUSY) {
			ppplog(MSG_INFO, 0, "rw_trywrlock error %d\n", ret);
			ASSERT(0);
		}
		i++;
		nap(100);
	}

	if (ret) {
		ppplog(MSG_INFO, 0, "RW_WRLOCK ... timeout\n");
		ASSERT(0);
	}
	return 0;
}

int
RW_RDLOCK(rwlock_t *l)
{
	int i = 0, ret;

	while (i < MAX_RETRY) {
		ret = rw_tryrdlock(l);
		if (ret == 0)
			break;
		if (ret != EBUSY) {
			ppplog(MSG_INFO, 0, "rw_tryrdlock error %d\n", ret);
			ASSERT(0);
		}
		i++;
		nap(100);
	}

	if (ret) {
		ppplog(MSG_INFO, 0, "RW_RDLOCK ... timeout\n");
		ASSERT(0);
	}
	return 0;
}

int
RW_UNLOCK(rwlock_t *l)
{
	rw_unlock(l);
}
