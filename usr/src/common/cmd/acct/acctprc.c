/*		copyright	"%c%" 	*/


#ident	"@(#)acct:common/cmd/acct/acctprc.c	1.3.7.4"
#ident "$Header$"
/*
 *      acctprc
 *      reads std. input (acct.h format), 
 *      writes std. output (tacct format)
 *      sorted by uid
 *      adds login names
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include "acctdef.h"
#include <sys/acct.h>

struct  acct    ab;
struct  ptmp    pb;
struct  tacct   tb;

struct  utab    {
        uid_t   ut_uid;
        char    ut_name[NSZ];
        float   ut_cpu[2];      /* cpu time (mins) */
        float   ut_kcore[2];    /* kcore-mins */
        long    ut_pc;          /* # processes */
} ub[A_USIZE]; 
static  usize;
int     ucmp();
char	*strncpy();


main()
{
        long    elaps[2];
        long    etime, stime, mem;
	time_t  expand();

        while (fread(&ab, sizeof(ab), 1, stdin) == 1) {
                if (!MYKIND(ab.ac_flag))
                        continue;
                pb.pt_uid = ab.ac_uid;
		pb.pt_name[0] = '\0';
                /*
                 * approximate cpu P/NP split as same as elapsed time
                 */
                if ((etime = SECS(expand(ab.ac_etime))) == 0)
                        etime = 1;
                stime = expand(ab.ac_stime) + expand(ab.ac_utime);
                mem = expand(ab.ac_mem);
                pnpsplit(ab.ac_btime, etime, elaps);
                pb.pt_cpu[0] = (double)stime * (double)elaps[0] / etime;
                pb.pt_cpu[1] = (stime > pb.pt_cpu[0])? stime - pb.pt_cpu[0] : 0;
                pb.pt_cpu[1] = stime - pb.pt_cpu[0];
                if (stime)
                        pb.pt_mem = (mem + stime - 1) / stime;
                else
                        pb.pt_mem = 0;  /* unlikely */
                enter(&pb);
        }
        squeeze();
        qsort(ub, usize, sizeof(ub[0]), ucmp);
        output();
}

enter(p)
register struct ptmp *p;
{
        register unsigned i;
        int j;
        double memk;

        i=(unsigned)p->pt_uid;
        j=0;
        for (i %= A_USIZE; !UBEMPTY && j++ < A_USIZE; i = (i+1) % A_USIZE)
                if (p->pt_uid == ub[i].ut_uid) 
                        break;
        if (j >= A_USIZE) {
                fprintf(stderr, "acctprc: INCREASE A_USIZE\n");
                exit(1);
        }
        if (UBEMPTY) {
                ub[i].ut_uid = p->pt_uid;
		ub[i].ut_name[0] = '\0';
        }
        ub[i].ut_cpu[0] += MINT(p->pt_cpu[0]);
        ub[i].ut_cpu[1] += MINT(p->pt_cpu[1]);
        memk = KCORE(pb.pt_mem);
        ub[i].ut_kcore[0] += memk * MINT(p->pt_cpu[0]);
        ub[i].ut_kcore[1] += memk * MINT(p->pt_cpu[1]);
        ub[i].ut_pc++;
}

squeeze()               /*eliminate holes in hash table*/
{
        register i, k;

        for (i = k = 0; i < A_USIZE; i++)
                if (!UBEMPTY) {
                        ub[k].ut_uid = ub[i].ut_uid;
			ub[k].ut_name[0] = '\0';
                        ub[k].ut_cpu[0] = ub[i].ut_cpu[0];
                        ub[k].ut_cpu[1] = ub[i].ut_cpu[1];
                        ub[k].ut_kcore[0] = ub[i].ut_kcore[0];
                        ub[k].ut_kcore[1] = ub[i].ut_kcore[1];
                        ub[k].ut_pc = ub[i].ut_pc;
                        k++;
                }
        usize = k;
}

ucmp(p1, p2)
register struct utab *p1, *p2;
{
	if (p1->ut_uid > p2->ut_uid)
                return 1;
        else if (p1->ut_uid < p2->ut_uid)
                return (-1);
        else 
                return (0);
}

output()
{
        register i;
        char *uidtonam();

        for (i = 0; i < usize; i++) {
                tb.ta_uid = ub[i].ut_uid;
                CPYN(tb.ta_name, uidtonam(ub[i].ut_uid));
                tb.ta_cpu[0] = ub[i].ut_cpu[0];
                tb.ta_cpu[1] = ub[i].ut_cpu[1];
                tb.ta_kcore[0] = ub[i].ut_kcore[0];
                tb.ta_kcore[1] = ub[i].ut_kcore[1];
                tb.ta_pc = ub[i].ut_pc;
                fwrite(&tb, sizeof(tb), 1, stdout);
        }
}
