#ident	"@(#)option.c	1.4"
#ident	"Header: $"

#include <sys/types.h>
#include <sys/socket.h>
#include "lber.h"
#include "ldap.h"
#include "ldap-int.h"
#include "ldaplog.h"

int
ldap_get_option(
	LDAP *ld,
	int option,
	void *outvalue)
{
	int ret = LDAP_SUCCESS;

	if ((ld == NULL) || (outvalue == NULL))
		return LDAP_PARAM_ERROR;

	switch (option) {
	case LDAP_OPT_DESC:
		*((int *)outvalue) = ld->ld_sb.sb_sd;
		break;
	case LDAP_OPT_DEREF:
		*((int *)outvalue) = ld->ld_deref;
		break;
	case LDAP_OPT_SIZELIMIT:
		*((int *)outvalue) = ld->ld_sizelimit;
		break;
	case LDAP_OPT_TIMELIMIT:
		*((int *)outvalue) = ld->ld_timelimit;
		break;
	case LDAP_OPT_REFERRALS:
		*((int *)outvalue) = (ld->ld_options & LDAP_FLAG_REFERRALS)?
			LDAP_OPT_ON: LDAP_OPT_OFF;
		break;
	case LDAP_OPT_VERSION:
		*((int *)outvalue) = LDAP_VERSION2;
		break;
	case LDAP_OPT_RESTART:
		*((int *)outvalue) = (ld->ld_options & LDAP_FLAG_RESTART)?
			LDAP_OPT_ON: LDAP_OPT_OFF;
		break;
	case LDAP_OPT_REBIND_FN:
#ifdef LDAP_REFERRALS
		*((ldap_rebind_fn_t *)outvalue) = ld->ld_rebindproc;
#else
		ret = LDAP_PARAM_ERROR;
#endif /* LDAP_REFERRALS */
		break;
	case LDAP_OPT_SSL:
	case LDAP_OPT_SORTKEYS:
	case LDAP_OPT_IO_FN_PTRS:
	case LDAP_OPT_THREAD_FN_PTRS:
	case LDAP_OPT_REBIND_ARG:
	case LDAP_OPT_CACHE_FN_PTRS:
	case LDAP_OPT_CACHE_STRATEGY:
	case LDAP_OPT_CACHE_ENABLE:

	default:
		ret = LDAP_PARAM_ERROR;
		break;
	}
	return ret;
}


int
ldap_set_option(
	LDAP *ld,
	int option,
	void *invalue)
{
	int ret = LDAP_SUCCESS;

	if ((ld == NULL) || (invalue == NULL))
		return LDAP_PARAM_ERROR;

	switch (option) {
	case LDAP_OPT_DESC:
		ld->ld_sb.sb_sd = *((int *)invalue);
		break;
	case LDAP_OPT_DEREF:
		ld->ld_deref = *((int *)invalue);
		break;
	case LDAP_OPT_SIZELIMIT:
		ld->ld_sizelimit = *((int *)invalue);
		break;
	case LDAP_OPT_TIMELIMIT:
		ld->ld_timelimit = *((int *)invalue);
		break;
	case LDAP_OPT_REFERRALS:
		if (*((int *)invalue) == LDAP_OPT_ON) {
			ld->ld_options |= LDAP_FLAG_REFERRALS;
		} else if (*((int *)invalue) == LDAP_OPT_OFF) {
			ld->ld_options &= ~LDAP_FLAG_REFERRALS;
		} else {
			ret = LDAP_PARAM_ERROR;
		}
		break;
	case LDAP_OPT_VERSION:
		if (*((int *)invalue) != LDAP_VERSION2)
			ret = LDAP_PARAM_ERROR;
		break;
	case LDAP_OPT_RESTART:
		if (*((int *)invalue) == LDAP_OPT_ON) {
			ld->ld_options |= LDAP_FLAG_RESTART;
		} else if (*((int *)invalue) == LDAP_OPT_OFF) {
			ld->ld_options &= ~LDAP_FLAG_RESTART;
		} else {
			ret = LDAP_PARAM_ERROR;
		}
		break;
	case LDAP_OPT_REBIND_FN:
#ifdef LDAP_REFERRALS
		ld->ld_rebindproc = *((ldap_rebind_fn_t *) invalue);
#else
		ret = LDAP_PARAM_ERROR;
#endif /* LDAP_REFERRALS */
		break;

	case LDAP_OPT_SSL:
	case LDAP_OPT_SORTKEYS:
	case LDAP_OPT_IO_FN_PTRS:
	case LDAP_OPT_THREAD_FN_PTRS:
	case LDAP_OPT_REBIND_ARG:
	case LDAP_OPT_CACHE_FN_PTRS:
	case LDAP_OPT_CACHE_STRATEGY:
	case LDAP_OPT_CACHE_ENABLE:

	default:
		ret = LDAP_PARAM_ERROR;
		break;
	}
	return ret;
}
