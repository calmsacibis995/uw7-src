#ident "@(#)filter.c	14.1"
#ident "$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */


/* this code is now part of the pf driver, owned and maintained by the UK
 * security team.  You can still include it as part of dlpi if you
 * compile all of dlpi with BUILTINFILTER defined in dlpi.mk
 * - Nathan
 *   5 June 97
 */
#ifdef BUILTINFILTER

#ident "$Id$"

#include <io/log/log.h>
#include <io/stream.h>
#include <io/strlog.h>
#include <io/stropts.h>
#include <mem/kmem.h>
#include <net/dlpi.h>

#include <net/inet/in.h>   /* for ntohl, ntohs(byteorder.h), msgblen define */

#if 0
#include <net/inet/in_systm.h>
#include <net/inet/ip.h>
#include <net/inet/tcp.h>
#include <net/inet/in_comp.h>
#include <net/inet/bpf.h>
#include <net/inet/memory.h>
#include <net/socket.h>
#include <net/sockio.h>
#endif

#include "bpf.h"

#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/mod/moddefs.h>

#include <io/ddi.h>		/* must come last */

#if defined(sparc) || defined(mips) || defined(ibm032)
#define BPF_ALIGN
#endif

#ifndef BPF_ALIGN
#define EXTRACT_SHORT(p)	((u_short)ntohs(*(u_short *)p))
#define EXTRACT_LONG(p)		(ntohl(*(u_long *)p))
#else
#define EXTRACT_SHORT(p)\
	((u_short)\
		((u_short)*((u_char *)p+0)<<8|\
		 (u_short)*((u_char *)p+1)<<0))
#define EXTRACT_LONG(p)\
		((u_long)*((u_char *)p+0)<<24|\
		 (u_long)*((u_char *)p+1)<<16|\
		 (u_long)*((u_char *)p+2)<<8|\
		 (u_long)*((u_char *)p+3)<<0)
#endif

#define MINDEX(m, k) \
{ \
	register int len = msgblen(m); \
 \
	while (k >= len) { \
		k -= len; \
		m = m->b_cont; \
		if (m == 0) \
			return 0; \
		len = msgblen(m); \
	} \
}

static int
m_xword(
	register mblk_t *m,
	register int k,
	int *err
	)
{
	register int len;
	register u_char *cp, *np;
	register mblk_t *m0;

	len = msgblen(m);
	while (k >= len) {
		k -= len;
		m = m->b_cont;
		if (m == 0) {
			goto bad;
		}
		len = msgblen(m);
	}
	cp = m->b_rptr + k;
	if (len - k >= 4) {
		*err = 0;
		return EXTRACT_LONG(cp);
	}
	m0 = m->b_cont;
	if (m0 == 0 || msgblen(m0) + len - k < 4){
		goto bad;
	}
	*err = 0;
	np = m0->b_rptr;
	switch (len - k) {

	case 1:
		return (cp[k] << 24) | (np[0] << 16) | (np[1] << 8) | np[2];

	case 2:
		return (cp[k] << 24) | (cp[k + 1] << 16) | (np[0] << 8) | 
			np[1];

	default:
		return (cp[k] << 24) | (cp[k + 1] << 16) | (cp[k + 2] << 8) |
			np[0];
	}
    bad:
	*err = 1;
	return 0;
}

static int
m_xhalf(
	register mblk_t *m,
	register int k, 
	int *err
	)
{
	register int len;
	register u_char *cp;
	register mblk_t *m0;

	len = msgblen(m);
	while (k >= len) {
		k -= len;
		m = m->b_cont;
		if (m == 0){
			goto bad;
		}
		len = msgblen(m);
	}
	cp = m->b_rptr + k;
	if (len - k >= 2) {
		*err = 0;
		return EXTRACT_SHORT(cp);
	}
	m0 = m->b_cont;
	if (m0 == 0) {
		goto bad;
	}
	*err = 0;
	return (cp[k] << 8) | m0->b_rptr[0];
 bad:
	*err = 1;
	return 0;
}

/*
 * Execute the filter program starting at pc on the packet p
 * wirelen is the length of the original packet
 * buflen is the amount of data present
 */
u_int
dlpi_filter(
	register struct bpf_insn *pc,
	register u_char *p,
	u_int wirelen,
	register u_int buflen
	)
{
	register u_long A, X;
	register int k;
	long mem[BPF_MEMWORDS];

	if (pc == 0)
		/*
		 * No filter means accept all.
		 */
		return (u_int)-1;
#ifdef lint
	A = 0;
	X = 0;
#endif
	--pc;
	while (1) {
		++pc;
		switch (pc->code) {

		default:
			return 0;
		case BPF_RET|BPF_K:
			return (u_int)pc->k;

		case BPF_RET|BPF_A:
			return (u_int)A;

		case BPF_LD|BPF_W|BPF_ABS:
			k = pc->k;
			if (k + sizeof(long) > buflen) {
				int merr;

				if (buflen != 0)
					return 0;
				A = m_xword((mblk_t *)p, k, &merr);
				if (merr != 0)
					return 0;
				continue;
			}
#ifdef BPF_ALIGN
			if (((int)(p + k) & 3) != 0)
				A = EXTRACT_LONG(&p[k]);
			else
#endif
				A = ntohl(*(long *)(p + k));
			continue;

		case BPF_LD|BPF_H|BPF_ABS:
			k = pc->k;
			if (k + sizeof(short) > buflen) {
				int merr;

				if (buflen != 0)
					return 0;
				A = m_xhalf((mblk_t *)p, k, &merr);
				continue;
			}
			A = EXTRACT_SHORT(&p[k]);
			continue;

		case BPF_LD|BPF_B|BPF_ABS:
			k = pc->k;
			if (k >= buflen) {
				register mblk_t *m;

				if (buflen != 0)
					return 0;
				m = (mblk_t *)p;
				MINDEX(m, k);
				A = m->b_rptr[k];
				continue;
			}
			A = p[k];
			continue;

		case BPF_LD|BPF_W|BPF_LEN:
			A = wirelen;
			continue;

		case BPF_LDX|BPF_W|BPF_LEN:
			X = wirelen;
			continue;

		case BPF_LD|BPF_W|BPF_IND:
			k = X + pc->k;
			if (k + sizeof(long) > buflen) {
				int merr;

				if (buflen != 0)
					return 0;
				A = m_xword((mblk_t *)p, k, &merr);
				if (merr != 0)
					return 0;
				continue;
			}
#ifdef BPF_ALIGN
			if (((int)(p + k) & 3) != 0)
				A = EXTRACT_LONG(&p[k]);
			else
#endif
				A = ntohl(*(long *)(p + k));
			continue;

		case BPF_LD|BPF_H|BPF_IND:
			k = X + pc->k;
			if (k + sizeof(short) > buflen) {
				int merr;

				if (buflen != 0)
					return 0;
				A = m_xhalf((mblk_t *)p, k, &merr);
				if (merr != 0)
					return 0;
				continue;
			}
			A = EXTRACT_SHORT(&p[k]);
			continue;

		case BPF_LD|BPF_B|BPF_IND:
			k = X + pc->k;
			if (k >= buflen) {
				register mblk_t *m;

				if (buflen != 0)
					return 0;
				m = (mblk_t *)p;
				MINDEX(m, k);
				A = m->b_rptr[k];
				continue;
			}
			A = p[k];
			continue;

		case BPF_LDX|BPF_MSH|BPF_B:
			k = pc->k;
			if (k >= buflen) {
				register mblk_t *m;

				if (buflen != 0)
					return 0;
				m = (mblk_t *)p;
				MINDEX(m, k);
				X = (m->b_rptr[k] & 0xf) << 2;
				continue;
			}
			X = (p[pc->k] & 0xf) << 2;
			continue;

		case BPF_LD|BPF_IMM:
			A = pc->k;
			continue;

		case BPF_LDX|BPF_IMM:
			X = pc->k;
			continue;

		case BPF_LD|BPF_MEM:
			A = mem[pc->k];
			continue;
			
		case BPF_LDX|BPF_MEM:
			X = mem[pc->k];
			continue;

		case BPF_ST:
			mem[pc->k] = A;
			continue;

		case BPF_STX:
			mem[pc->k] = X;
			continue;

		case BPF_JMP|BPF_JA:
			pc += pc->k;
			continue;

		case BPF_JMP|BPF_JGT|BPF_K:
			pc += (A > pc->k) ? pc->jt : pc->jf;
			continue;

		case BPF_JMP|BPF_JGE|BPF_K:
			pc += (A >= pc->k) ? pc->jt : pc->jf;
			continue;

		case BPF_JMP|BPF_JEQ|BPF_K:
			pc += (A == pc->k) ? pc->jt : pc->jf;
			continue;

		case BPF_JMP|BPF_JSET|BPF_K:
			pc += (A & pc->k) ? pc->jt : pc->jf;
			continue;

		case BPF_JMP|BPF_JGT|BPF_X:
			pc += (A > X) ? pc->jt : pc->jf;
			continue;

		case BPF_JMP|BPF_JGE|BPF_X:
			pc += (A >= X) ? pc->jt : pc->jf;
			continue;

		case BPF_JMP|BPF_JEQ|BPF_X:
			pc += (A == X) ? pc->jt : pc->jf;
			continue;

		case BPF_JMP|BPF_JSET|BPF_X:
			pc += (A & X) ? pc->jt : pc->jf;
			continue;

		case BPF_ALU|BPF_ADD|BPF_X:
			A += X;
			continue;
			
		case BPF_ALU|BPF_SUB|BPF_X:
			A -= X;
			continue;
			
		case BPF_ALU|BPF_MUL|BPF_X:
			A *= X;
			continue;
			
		case BPF_ALU|BPF_DIV|BPF_X:
			if (X == 0){
				return 0;
			}
			A /= X;
			continue;
			
		case BPF_ALU|BPF_AND|BPF_X:
			A &= X;
			continue;
			
		case BPF_ALU|BPF_OR|BPF_X:
			A |= X;
			continue;

		case BPF_ALU|BPF_LSH|BPF_X:
			A <<= X;
			continue;

		case BPF_ALU|BPF_RSH|BPF_X:
			A >>= X;
			continue;

		case BPF_ALU|BPF_ADD|BPF_K:
			A += pc->k;
			continue;
			
		case BPF_ALU|BPF_SUB|BPF_K:
			A -= pc->k;
			continue;
			
		case BPF_ALU|BPF_MUL|BPF_K:
			A *= pc->k;
			continue;
			
		case BPF_ALU|BPF_DIV|BPF_K:
			A /= pc->k;
			continue;
			
		case BPF_ALU|BPF_AND|BPF_K:
			A &= pc->k;
			continue;
			
		case BPF_ALU|BPF_OR|BPF_K:
			A |= pc->k;
			continue;

		case BPF_ALU|BPF_LSH|BPF_K:
			A <<= pc->k;
			continue;

		case BPF_ALU|BPF_RSH|BPF_K:
			A >>= pc->k;
			continue;

		case BPF_ALU|BPF_NEG:
			A = -A;
			continue;

		case BPF_MISC|BPF_TAX:
			X = A;
			continue;

		case BPF_MISC|BPF_TXA:
			A = X;
			continue;
		}
	}
}

/*
 * Return true if the 'fcode' is a valid filter program.
 * The constraints are that each jump be forward and to a valid
 * code.  The code must terminate with either an accept or reject. 
 * 'valid' is an array for use by the routine (it must be at least
 * 'len' bytes long).  
 *
 * The kernel needs to be able to verify an application's filter code.
 * Otherwise, a bogus program could easily crash the system.
 */
int
dlpi_validate(
	struct bpf_insn *f,
	int len
	)
{
	register int i;
	register struct bpf_insn *p;

	for (i = 0; i < len; ++i) {
		/*
		 * Check that that jumps are forward, and within 
		 * the code block.
		 */
		p = &f[i];
		if (BPF_CLASS(p->code) == BPF_JMP) {
			register int from = i + 1;

			if (BPF_OP(p->code) == BPF_JA) {
				if (from + p->k >= len)
					return 0;
			}
			else if ((int )(from + p->jt) >= len || (int)(from + p->jf) >= len)
				return 0;
		}
		/*
		 * Check that memory operations use valid addresses.
		 */
		if ((BPF_CLASS(p->code) == BPF_ST ||
		     (BPF_CLASS(p->code) == BPF_LD && 
		      (p->code & 0xe0) == BPF_MEM)) &&
		    (p->k >= BPF_MEMWORDS || p->k < 0))
			return 0;
		/*
		 * Check for constant division by 0.
		 */
		if (p->code == (BPF_ALU|BPF_DIV|BPF_K) && p->k == 0)
			return 0;
	}
	return BPF_CLASS(f[len - 1].code) == BPF_RET;
}

#endif /* BUILTINFILTER */
