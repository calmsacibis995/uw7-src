#ident	"@(#)kern-pdi:io/layer/mpio/mpio_msg.c	1.1.1.4"

#include    "mpio.h"
#include    "mpio_os.h"

/*
 *  This module contains the MPIO driver message dictionary and
 *  message logging facility.
 *
 */
typedef struct{
    int	level;    /* of seriousness     */
    char *    msg;
} mpio_msg_t, *mpio_msg_p_t;

STATIC mpio_msg_t    mpio_english_dictionary[] = {
{ 0,	0 },
{ CE_WARN, "mpio: can't alloc memory"},					/*001*/
{ CE_WARN, "mpio: initiate path 0x%x of device 0x%x failure recovery"}, /*002*/
{ CE_WARN, "mpio: begin auto path repair for device 0x%x"},		/*003*/
{ CE_WARN, "mpio: recovering from path 0x%x failure failed"},	        /*004*/
{ CE_WARN, "mpio: repair path 0x%x failed"},			        /*005*/
{ CE_WARN, "mpio: tresspass path 0x%x"},				/*006*/
{ CE_WARN, "mpio: tresspassing path 0x%x failed"},			/*007*/
{ CE_WARN, "mpio: shutdown path 0x%x due to excessive trespasses"},	/*008*/
{ CE_WARN, "mpio: can't deregister 0x%x, an unknown path"},		/*009*/
{ CE_WARN, "mpio: registering a new MPIO device failed"},	        /*010*/
{ CE_WARN, "mpio: MP driver failed to provide path signature"},         /*011*/
{ CE_WARN, "mpio: Driver init failure"},                                /*012*/
{ CE_WARN, "mpio: SDI reported an unknown device configuration"},       /*013*/
{ 0,	0 }
};

mpio_msg_p_t mpio_dictionary_p = mpio_english_dictionary;

void err_log0(int ar0)
{
    mpio_msg_p_t    msg_p;
    msg_p = &mpio_dictionary_p[ar0];
    ERR_LOG(msg_p->level, msg_p->msg);
}

void err_log1(int ar0, int ar1)
{
    mpio_msg_p_t    msg_p;
    msg_p = &mpio_dictionary_p[ar0];
    ERR_LOG(msg_p->level, msg_p->msg, ar1);
}

void err_log2(int ar0, int ar1, int ar2)
{
    mpio_msg_p_t    msg_p;
    msg_p = &mpio_dictionary_p[ar0];
    ERR_LOG(msg_p->level, msg_p->msg,ar1,ar2);
}

void err_log3(int ar0, int ar1, int ar2, int ar3)
{
    mpio_msg_p_t    msg_p;
    msg_p = &mpio_dictionary_p[ar0];
    ERR_LOG(msg_p->level, msg_p->msg,ar1,ar2,ar3);
}

/**** END mpio_msg.c ****/
