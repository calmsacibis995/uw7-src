#ident	"@(#)ucfg.c	1.6"

#include <stdlib.h>
#include <locale.h>
#include <sys/param.h>
#include <synch.h>
#include <thread.h>
#include <errno.h>

#include "pathnames.h"
#include "psm.h"
#include "ppp_type.h"
#include "ppp_param.h"
#include "ppp_cfg.h"
#include "act.h"
#include "ppp_proto.h"

/*
 * User confuguration
 *
 * This module contains routines to manage the users PPP configuration.
 * It is called from the ulr_ routines to get, set, delete and list
 * the configuration,
 *
 * The configuration is stored on several lists, a list per definition type.
 * The configuration is protected by a single rw lock. The lock must be held in
 * at least read mode to traverse any elements. It must be held in write mode
 * to add/remove list elements. A list element must have a zero reference count
 * if it is to be modified. The rw lock must be held for writing if a
 * reference count is to be modified.
 */
rwlock_t ucfg_rwlock;
struct cfg_hdr *ucfg[DEF_MAX];

/*
 * This is a list of configurations that are old ... that
 * have been superceeded by a new config ... they may be removed
 * when their refcnt == 0 ...
 */
struct cfg_hdr *ucfg_old[DEF_MAX];

/*
 * Pointers to global configurations
 */
struct cfg_global *global[DEF_MAX];

/*
 * Clobal config state flags
 */
int ucfg_state = 0;

void
ucfg_init()
{
	int i;

	rwlock_init(&ucfg_rwlock, USYNC_PROCESS, NULL);

	for (i = 0; i < DEF_MAX; i++) {
		ucfg[i] = NULL;
		ucfg_old[i] = NULL;
	}

	default_init();
}

/*
 * Check on the ucfg_old lists to see if we have any unused entries
 * if we do then remove them
 */
ucfg_garbage()
{
	cfg_hdr_t *c, *prev;
	int i;

	RW_WRLOCK(&ucfg_rwlock);
	
	for (i = 0; i < DEF_MAX; i++) {

		c = ucfg_old[i];
		prev = NULL;

		while (c) {

			if (c->ch_refcnt == 0) {

				if (prev)
					prev->ch_next = c->ch_next;
				else
					ucfg_old[i] = c->ch_next;

				ppplog(MSG_DEBUG, 0,
				       "ucfg_garbage: type %d, free %s\n",
				       i, c->ch_id);

				free(c);

				if (prev)
					c = prev->ch_next;
				else
					c = NULL;

			} else {
				prev = c;
				c = c->ch_next;
			}
		}
	}

	/* 
	 * We have found our entry - remove it
	 */

	RW_UNLOCK(&ucfg_rwlock);

	timeout(10 * HZ, ucfg_garbage, (caddr_t)0, (caddr_t)0);
}

/*
 *  Create or mofify a configuration entry
 *	type - specifies the definition type (link, bundle ...)
 *	ch - points to the users new config.
 *
 *  Return 0 on sucess
 * Return errno on failure.
 */
int
ucfg_set(int type, struct cfg_hdr *ch)
{
	cfg_hdr_t *uc, *prev = NULL;
	int ret;
	cfg_hdr_t *new;

	RW_WRLOCK(&ucfg_rwlock);

	/*
	 * Check if a definition with the same ID already exists,
	 * if so update it otherwise, it's a new definition - create
	 * an entry.
	 */
	uc = ucfg[type];

	while (uc && strcmp(uc->ch_id, ch->ch_id) != 0) {
		prev = uc;
		uc = uc->ch_next;
	}

	if (!uc) {
		/* It's a new definition */

		uc = (cfg_hdr_t *)malloc(ch->ch_len);
		if (!uc) {
			RW_UNLOCK(&ucfg_rwlock);
			return(ENOMEM);
		}
			
		memcpy(uc, ch, ch->ch_len);

		/* Initialise link structure */
		uc->ch_refcnt = 0;

		/* Link int the new structure */
		uc->ch_next = ucfg[type];
		ucfg[type] = new = uc;

	} else {

		if (ch->ch_flags & CHF_RONLY) {
			RW_UNLOCK(&ucfg_rwlock);
			return EPERM;
		}

		new = (cfg_hdr_t *)malloc(ch->ch_len);
		if (!new) {
			RW_UNLOCK(&ucfg_rwlock);
			return ENOMEM;
		}

		memcpy(new, ch, ch->ch_len);
		new->ch_refcnt = 0;
		new->ch_next = uc->ch_next;

		if (prev)
			prev->ch_next = new;
		else
			ucfg[type] = new;

		/* 
		 * To modify the contents of this element we must 
		 * have a zero ref cnt
		 */
		if (uc->ch_refcnt > 0) {
			uc->ch_next = ucfg_old[type];
			ucfg_old[type] = uc;
		} else
			free(uc);

	}

	if (type == DEF_GLOBAL) {
		struct cfg_global *cg = (struct cfg_global *)new;
		global[cg->gi_type] = cg;
	}

	ucfg_state |= UCFG_MODIFIED;
	RW_UNLOCK(&ucfg_rwlock);
	return 0;
}

/*
 * Return a pointer to the specified config definition
 */
int
ucfg_findid(char *id, int type, cfg_hdr_t **c)
{
	cfg_hdr_t *uc;
	int ret;

	RW_WRLOCK(&ucfg_rwlock);

	/*
	 * Check if a definition with the same ID already exists, 
	 * if so update it otherwise, it's a new 
	 * definition - create an entry.
	 */
	uc = ucfg[type];

	while (uc && strcmp(uc->ch_id, id) != 0)
		uc = uc->ch_next;

	if (uc) {
		*c = uc;
		return 0;
	}

	RW_UNLOCK(&ucfg_rwlock);
	return ENOENT;
}

ucfg_lock()
{
	RW_WRLOCK(&ucfg_rwlock);
}

/*
 * Release the user config lock
 */
ucfg_release()
{
	RW_UNLOCK(&ucfg_rwlock);
}


int
ucfg_del(char *id, int type)
{
	cfg_hdr_t *c, *prev = NULL;

	/*
	 * Lock the definition list for WRITING
	 */
	RW_WRLOCK(&ucfg_rwlock);

	/*
	 * Find the element to remove.
	 */
	c = ucfg[type];
	while (c && strcmp(c->ch_id, id) != 0) {
		prev = c;
		c = c->ch_next;
	}

	if (!c) {
		RW_UNLOCK(&ucfg_rwlock);
		return ENOENT;
	}


	/* 
	 * We have found our entry - remove it
	 */
	if (prev)
		prev->ch_next = c->ch_next;
	else
		ucfg[type] = c->ch_next;

	if (type == DEF_GLOBAL) {
		struct cfg_global *cg = (struct cfg_global *)c;
		global[cg->gi_type] = NULL;
	}

	if (c->ch_refcnt > 0) {
		c->ch_next = ucfg_old[type];
		ucfg_old[type] = c;
	} else
		free(c);

	ucfg_state |= UCFG_MODIFIED;
	RW_UNLOCK(&ucfg_rwlock);
	return 0;
}

/*
 * Return a pointer to a sting in a configuration structure
 */
char *
ucfg_str(struct cfg_hdr *ch, unsigned int off)
{
	return((char *)ch + off);
}

/*
 * Copy next element from s to d (it is assumed that the
 * buffer we are given is large enough to hold the element, MAXID).
 * Return a pointer to character after next element
 */
char *
ucfg_get_element(char *s, char *d)
{
	int len = 0;

	while (*s && !isspace(*s) && len < MAXID)
		d[len++] = *s++;

	d[len] = 0;

	if (len == MAXID) {
		if (*s && !isspace(*s)) {
			ppplog(MSG_WARN, 0,
			       "Truncated element in list to %s\n", d);
		}

		/* Too long */
		while (*s && !isspace(*s))
			s++;
	}

	while(*s && isspace(*s))
		s++;

	return s;
}

void
ucfg_insert_str(struct cfg_hdr *ch, unsigned int *off, char *src)
{
	char *p;

	p = (char *)ch + ch->ch_len;
	*off = ch->ch_len;
	strcpy(p, src);

	ch->ch_len += strlen(src) + 1;
}

/*
 * Return the index number (first is 1) .. of the given string in the
 * specified list of elements.
 */
int
ucfg_find_element(char *find, char *list)
{
	char buf[MAXID];
	int index = 0;

	while (*list) {
		index++;
		list = ucfg_get_element(list, buf);
		if (strcmp(buf, find) == 0)
			return index;
	}
	return 0;
}

/*
 * The following routines are provided so that a user may
 * traverse all the config definitions by type
 */
int
ucfg_open(int type, cfg_hdr_t **first)
{
	RW_WRLOCK(&ucfg_rwlock);
	*first = ucfg[type];
}

int
ucfg_next(cfg_hdr_t **entry)
{
	*entry = (*entry)->ch_next;
	return *entry != NULL;
}

void
ucfg_close()
{
	RW_UNLOCK(&ucfg_rwlock);
}
