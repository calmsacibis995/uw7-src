#ifndef _IO_TSM_ETHTSM_ETHTXT_H
#define _IO_TSM_ETHTSM_ETHTXT_H

#ident	"@(#)ethtxt.h	2.1"
#ident	"$Header$"

#define	ETHERTSM_FRAME_8022_TXTMSG		"ETHERNET_802.2"
#define	ETHERTSM_FRAME_8023_TXTMSG		"ETHERNET_802.3"
#define	ETHERTSM_FRAME_EII_TXTMSG		"ETHERNET_II"
#define	ETHERTSM_FRAME_SNAP_TXTMSG		"ETHERNET_SNAP"
#define	ETHERTSM_NODE_EQUAL_TXTMSG		"NODE=%s"
#define	ETHERTSM_FRAME_EQUAL_TXTMSG		"FRAME=%s"
#define	ETHERTSM_STATS_VERSION_ERROR_MSG				\
	"This version of the statistics table is not supported.\n"
#define	ETHERTSM_MEMORY_ALLOC_ERROR_MSG					\
	"Unable to allocate memory.\n\r"

#define ETHERTSM_STAT_TOTAL_TX_MSG		"MTotalTxPacketCount"
#define	ETHERTSM_STAT_TOTAL_RX_MSG		"MTotalRxPacketCount"
#define	ETHERTSM_STAT_NO_ECB_MSG		"MNoECBAvailableCount"
#define	ETHERTSM_STAT_TX_BIG_MSG		"MPacketTxTooBigCount"
#define	ETHERTSM_STAT_TX_SMALL_MSG		"MPacketTxTooSmallCount"
#define	ETHERTSM_STAT_RX_OVERFLOW_MSG		"MPacketRxOverflowCount"
#define	ETHERTSM_STAT_RX_BIG_MSG		"MPacketRxTooBigCount"
#define	ETHERTSM_STAT_RX_SMALL_MSG		"MPacketRxTooSmallCount"
#define	ETHERTSM_STAT_TX_MISC_MSG		"MTotalTxMiscCount"
#define	ETHERTSM_STAT_RX_MISC_MSG		"MTotalRxMiscCount"
#define	ETHERTSM_STAT_RETRY_TX_MSG		"MRetryTxCount"
#define	ETHERTSM_STAT_CHECKSUM_MSG		"MChecksumErrorCount"
#define	ETHERTSM_STAT_RX_MISMATCH_MSG		"MHardwareRxMismatchCount"
#define	ETHERTSM_STAT_TX_OK_BYTE_MSG		"MTotalTxOKByteCount"
#define	ETHERTSM_STAT_RX_OK_BYTE_MSG		"MTotalRxOKByteCount"
#define	ETHERTSM_STAT_GRP_ADDR_TX_MSG		"MTotalGroupAddrTxCount"
#define	ETHERTSM_STAT_GRP_ADDR_RX_MSG		"MTotalGroupAddrRxCount"
#define	ETHERTSM_STAT_ADAPTER_RESET_MSG		"MAdapterResetCount"
#define	ETHERTSM_STAT_ADAP_OPR_TIME_MSG		"MAdapterOprTimeStamp"
#define	ETHERTSM_STAT_QDEPTH_MSG		"MQdepth"

#define	ETHERTSM_STAT_TX_SINGLE_COL_MSG					\
	"ETH_TxOKSingleCollisionsCount"
#define	ETHERTSM_STAT_TX_MULTI_COL_MSG					\
	"ETH_TxOKMultipleCollisionsCount"
#define	ETHERTSM_STAT_TX_DEFER_MSG					\
	"ETH_TxOKButDeferred"
#define	ETHERTSM_STAT_TX_ABORT_LATE_COL_MSG				\
	"ETH_TxAbortLateCollision"
#define	ETHERTSM_STAT_TX_ABORT_EXC_COL_MSG				\
	"ETH_TxAbortExcessCollision"
#define	ETHERTSM_STAT_TX_ABORT_CARRIER_MSG				\
	"ETH_TxAbortCarrierSense"
#define	ETHERTSM_STAT_TX_ABORT_EXC_DEFER_MSG				\
	"ETH_TxAbortExcessiveDeferral"
#define	ETHERTSM_STAT_RX_ABORT_FRAME_ALIGN_MSG				\
	"ETH_RxAbortFrameAlignment"
#define	ETHERTSM_GRP_BIT_OVERRIDE_MSG					\
	"The group bit in the node address override was cleared.\n"
#define	ETHERTSM_LOCAL_BIT_OVERRIDE_MSG					\
	"The local bit in the node address override was set.\n"
#define	ETHERTSM_WHITESPACE_MSG						\
	" \t\n"
#define	ETHERTSM_ODI_SPECVER_MSG					\
	"ODI_CSPEC_VERSION: 1.10"

#endif	/* _IO_TSM_ETHTSM_ETHTXT_H */
