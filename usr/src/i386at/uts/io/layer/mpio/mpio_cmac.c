#ident	"@(#)kern-pdi:io/layer/mpio/mpio_cmac.c	1.1.8.1"

#include "mpio_core.h"
#include "mpio_os.h"

#ifdef MPIO_DEBUG
/*
 * print N=size bytes in long format
 */
printblk( uint * bufp, int size)
{
    int i;
    int newline;

    for (i = 0, newline = 0; i < size/4; bufp++ , i++){
        if (newline == 0)
            printf ("           0x%08x",*bufp);
        else
            printf (" 0x%08x",*bufp);
        newline = (newline+1)%4;
        if (newline == 0)
            printf ("\n");
    }
    printf ("\n");
}

/*
 *  0xxxxxx mpio_vdev 1 call
 */
mpio_vdev ( void *arg)
{
vdev_p_t    vdevp;
vdevp = (vdev_p_t)arg;
    printf("links\n");
    printblk((uint*)&vdevp->links, sizeof(vdevp->links));
    printf("signature\n");
    printblk((uint*)&vdevp->signature, sizeof(vdevp->signature));
    printf("open_count           0x%08x\n", vdevp->open_count);
    printf("number_of_paths      0x%08x\n", vdevp->number_of_paths);
    printf("old_ipl              %x\n, vdevp->lock.old_ipl");
    printf("lockp");
    printblk((uint*)&vdevp->lock.p, sizeof(vdevp->lock.p));
    printf("lpg_vector\n");
    printblk((uint*)&vdevp->lpg_vector, sizeof(vdevp->lpg_vector));

}

/*
 * Usage: <addr> <offset> <count> walk 3 call
 */
walk( mpio_qm_anchor_t * anchorp, int offset , int len )
{
    uint * entryp;
    mpio_qm_link_t * linkp;
    int index;

    index = offset/4;
    entryp = (uint *)MPIO_QM_HEAD(anchorp);
    while ( entryp != NULL && len ){
        linkp = (mpio_qm_link_t *) &entryp[index];
        printf("%x    %x\n", entryp, linkp->next);
        entryp = MPIO_QM_NEXT(anchorp, entryp);    
        len --;
    }

}


/***************************/
    buft(void *arg)
/***************************/
{
    buf_t*    bp;

    bp = (buf_t*)arg;

    printf("b_flags         %x\n",bp->b_flags);
    printf("b_error         %x\n",bp->b_error);
    printf("b_forw          %x\n",bp->b_forw);
    printf("b_back          %x\n",bp->b_back);
    printf("av_forw         %x\n",bp->av_forw);
    printf("av_back         %x\n",bp->av_back);
    printf("b_bufsize       %x\n",bp->b_bufsize);
    printf("b_bcount        %x\n",bp->b_bcount);
    printf("b_blkno         %x\n",bp->b_blkno);
    printf("b_blkoff        %x\n",bp->b_blkoff);
    printf("b_addrtype      %x\n",bp->b_addrtype);
    printf("b_scgth_count   %x\n",bp->b_scgth_count);
    printf("b_un            %x\n",bp->b_un.b_addr);
    printf("b_proc          %x\n",bp->b_proc);
    printf("b_resid         %x\n",bp->b_resid);
    printf("b_start         %x\n",bp->b_start);
    printf("b_iodone        %x\n",bp->b_iodone);
    printf("b_misc          %x\n",bp->b_misc);
    printf("\n");

}


/***************************/
    mpiodip()
/***************************/
{
    if ( dip == NULL )
        return;

    printf("num_of_vdevs        %x\n",dip->num_of_vdevs);
    printf("ioreq_in            %x\n",dip->ioreq_in);
    printf("ioreq_out           %x\n",dip->ioreq_out);
    printf("ready               %x\n",dip->ready);
    printf("old_ipl             %x\n",dip->lock.old_ipl);
    printf("lockp               %x\n",dip->lock.p);
    printf("\n");

}

#endif
