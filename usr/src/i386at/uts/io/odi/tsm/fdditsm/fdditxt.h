#ifndef	_IO_TSM_FDDITSM_FDDITXT_H
#define	_IO_TSM_FDDITSM_FDDITXT_H
#ident	"@(#)fdditxt.h	2.1"
#ident	"$Header$"

/*
 * fddistrings.c contains the ASCII strings that are specific to the
 * fddi MLID
 */

#define	FDDITSM_FRAME_EQUAL_TXTMSG  	"FRAME=%s"
#define	FDDITSM_NODE_EQUAL_TXTMSG  	"NODE=%s"
#define	FDDITSM_FRAME_8022_TXTMSG	"FDDI_802.2"
#define	FDDITSM_FRAME_SNAP_TXTMSG	"FDDI_SNAP"

#define	FDDITSM_STATS_VERSION_ERROR_MSG \
	"This version of the statistics table is not supported.\n"
#define	FDDITSM_MEMORY_ALLOC_ERROR_MSG	"Unable to allocate memory.\n\r"


/* Generic Statistics strings */


#define FDDITSM_STAT_TOTAL_TX_MSG	"MTotalTxPacketCount"
#define	FDDITSM_STAT_TOTAL_RX_MSG	"MTotalRxPacketCount"
#define	FDDITSM_STAT_NO_ECB_MSG		"MNoECBAvailableCount"
#define	FDDITSM_STAT_TX_BIG_MSG		"MPacketTxTooBigCount"
#define	FDDITSM_STAT_TX_SMALL_MSG	"MPacketTxTooSmallCount"
#define	FDDITSM_STAT_RX_OVERFLOW_MSG	"MPacketRxOverflowCount"
#define	FDDITSM_STAT_RX_BIG_MSG		"MPacketRxTooBigCount"
#define	FDDITSM_STAT_RX_SMALL_MSG	"MPacketRxTooSmallCount"
#define	FDDITSM_STAT_TX_MISC_MSG	"MTotalTxMiscCount"
#define	FDDITSM_STAT_RX_MISC_MSG	"MTotalRxMiscCount"
#define	FDDITSM_STAT_RETRY_TX_MSG	"MRetryTxCount"
#define	FDDITSM_STAT_CHECKSUM_MSG	"MChecksumErrorCount"
#define	FDDITSM_STAT_RX_MISMATCH_MSG	"MHardwareRxMismatchCount"
#define	FDDITSM_STAT_TX_OK_BYTE_MSG	"MTotalTxOKByteCount"
#define	FDDITSM_STAT_RX_OK_BYTE_MSG	"MTotalRxOKByteCount"
#define	FDDITSM_STAT_GRP_ADDR_TX_MSG	"MTotalGroupAddrTxCount"
#define	FDDITSM_STAT_GRP_ADDR_RX_MSG	"MTotalGroupAddrRxCount"
#define	FDDITSM_STAT_ADAPTER_RESET_MSG	"MAdapterResetCount"
#define	FDDITSM_STAT_ADAP_OPR_TIME_MSG	"MAdapterOprTimeStamp"
#define	FDDITSM_STAT_QDEPTH_MSG		"MQdepth"


/* Media Specific Statistics strings */

#define	FDDITSM_STAT_CONFIG_STATE_MSG		"FDI_ConfigurationState"
#define	FDDITSM_STAT_UPSTREAM_NODE_MSG		"FDI_UpstreamNode"
#define	FDDITSM_STAT_DOWNSTREAM_NODE_MSG	"FDI_DownstreamNode"
#define	FDDITSM_STAT_FRAME_ERROR_MSG		"FDI_FrameErrorCount"
#define	FDDITSM_STAT_FRAME_LOST_MSG		"FDI_FramesLostCount"
#define	FDDITSM_STAT_RING_MANG_STATE_MSG	"FDI_RingManagementState"
#define	FDDITSM_STAT_LCT_FAILURE_MSG		"FDI_LCTFailureCount"
#define	FDDITSM_STAT_LEM_REJECT_MSG		"FDI_LemRejectCount"
#define	FDDITSM_STAT_LEM_COUNT_MSG		"FDI_LemCount"
#define	FDDITSM_STAT_LCONNECT_STATE_MSG		"FDI_LConnectionState"


#define	FDDITSM_GRP_BIT_OVERRIDE_MSG		"The group bit in the node address override was cleared.\n"
#define	FDDITSM_LOCAL_BIT_OVERRIDE_MSG		"The local bit in the node address override was set.\n"
#define	FDDITSM_WHITESPACE_MSG			" \t\n"
#define	FDDITSM_ODI_SPECVER_MSG			"ODI_CSPEC_VERSION: 1.10"

#endif	/* _IO_TSM_FDDITSM_FDDITXT_H */
