#ifndef _IO_ND_DLPI_PROTOTYPES_H
#define _IO_ND_DLPI_PROTOTYPES_H

#ident "@(#)prototype.h	29.3"
#ident "$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1996.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

/* mdi.c */
int mdi_inactive_handler(queue_t *, mblk_t *);
void mdi_initialize_driver(per_sap_info_t *);
void mdi_shutdown_driver(per_sap_info_t *, u_int);

/* mdi.c */
void dlpi_send_dl_error_ack(per_sap_info_t *, ulong, int, int);
void dlpi_enable_WRqs(per_card_info_t *cp);
int mdi_INIT_prim_handler(queue_t *, mblk_t *);

/* lliioctl.c */
void dlpi_send_iocack(per_sap_info_t *);
void dlpi_send_iocnak(per_sap_info_t *, int);
int  dlpi_ioctl(queue_t *q, mblk_t *mp, per_card_info_t *cp);


/* mdiioctl.c */
void mdi_send_MACIOC_SETADDR(per_sap_info_t *, void (*handler)(), unchar *);
void mdi_send_MACIOC_GETADDR(per_sap_info_t *, void (*handler)());
void mdi_send_MACIOC_GETSTAT(per_sap_info_t *, void (*handler)());
void mdi_send_MACIOC_SETMCA(per_sap_info_t *, void (*handler)(), unchar *);
void mdi_send_MACIOC_DELMCA(per_sap_info_t *, void (*handler)(), unchar *);
void mdi_send_MACIOC_GETMCA(per_sap_info_t *, void (*handler)());
void mdi_send_MACIOC_CLRSTAT(per_sap_info_t *, void (*handler)());
void mdi_send_MACIOC_dup(per_sap_info_t *sp, int ioctl_cmd, int size,
					void (*handler)(), mblk_t *datamp);
void mdi_send_MACIOC_cp_queue(per_sap_info_t *sp, int ioctl_cmd, int size,
					void (*handler)(), unchar *data);

/* dlpi.c */
ulong dlpi_mca_count(per_card_info_t *, per_sap_info_t *);
STATIC void dlpi_delmca_ack(per_sap_info_t *, mblk_t *);

/* dlpidata.c */
void dlpi_send_dl_unitdata_ind(per_sap_info_t *, struct per_frame_info *);
void dlpi_send_dl_xid_ind(per_sap_info_t *, struct per_frame_info *, int);
void dlpi_send_dl_test_ind(per_sap_info_t *, struct per_frame_info *, int);
void dlpi_send_dl_xid_con(per_sap_info_t *, struct per_frame_info *, int);
void dlpi_send_dl_test_con(per_sap_info_t *, struct per_frame_info *, int);
void dlpi_send_unitdata(per_sap_info_t *, int, mblk_t *, int, int, int );
void dlpi_send_llc_message(per_sap_info_t *,int, mblk_t *, int, int, int );
void dlpi_putnext_down_queue(per_card_info_t *, mblk_t *);

/* util.c */
int dlpi_putnext(queue_t *, mblk_t *);
int dlpi_canput(queue_t *);
per_sap_info_t *dlpi_allocsap(queue_t *,per_card_info_t *, int, cred_t *);
per_sap_info_t *dlpi_findsap(ulong,ulong, per_card_info_t *);
per_sap_info_t *dlpi_findsnapsap(ulong,ulong, per_card_info_t *);
uint dlpi_hsh(unchar *, uint);

void dlpi_freesubssaps(per_sap_info_t *);
void dlpi_dumpsaps(unsigned char *, int, per_card_info_t *);
int dlpi_init_sap_counters(struct sap_counters *);

#ifdef DLPI_DEBUG
    void dlpi_printf();
#endif

/* llc.c */
int dlpi_llc_rx(per_sap_info_t *, struct per_frame_info *);
int dlpi_make_llc_ui(per_sap_info_t *, unchar, unchar *);
int dlpi_make_llc_xid(per_sap_info_t *, unchar, int, int, unchar * );
int dlpi_make_llc_test(per_sap_info_t *, unchar, int, int, unchar *);
per_sap_info_t *dlpi_findfirstllcsap(struct per_frame_info *,
						per_card_info_t *);
per_sap_info_t *dlpi_findnextllcsap(ulong, per_card_info_t *,
						per_sap_info_t *);

/* bind.c */
int dlpi_dobind(per_sap_info_t *, ulong, ulong, ulong);

/* txmon.c */
void dlpi_txmon_check(per_card_info_t *);

/* Global variables */
extern per_media_info_t	mdi_media_info[];

/* sr.c */
int dlpiSR_init(per_card_info_t *);
void dlpiSR_uninit(per_card_info_t *);

#endif	/* _IO_ND_DLPI_PROTOTYPES_H */

