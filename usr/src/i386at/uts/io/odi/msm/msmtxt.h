#ifndef _IO_MSM_MSMTXT_H
#define	_IO_MSM_MSMTXT_H

#ident	"@(#)msmtxt.h	10.1"
#ident	"$Header$"

/*
 *	msmtxt.h, contains msm messages.
 */

#define CMSM_INIT_ALLOC_MSG						\
	(MEON_STRING *)"CMSM Init Alloc Memory"
#define CMSM_NO_POLL_MSG						\
	(MEON_STRING *)"CMSM Unable to allocate resources for Polling"
#define CMSM_NO_BUSLIST_MSG						\
	(MEON_STRING *)"CMSM Unable to create Bus list"
#define CMSM_UNABLE_TO_RESET_MSG					\
	(MEON_STRING *)"CMSM Unable to reset MLID %s.\n\r"
#define CMSM_TOOLONG_INTR_MSG						\
	(MEON_STRING *)"Interrupt %d took too long. Disabling adapter %s\n\r"
#define CMSM_INTR_NOTCONSUMED_MSG					\
	(MEON_STRING *)"Interrupt %d was not consumed, adapter %s\n\r"
#define CMSM_SHMEM_NOTREG_MSG						\
	(MEON_STRING *)"Could not register shared memory for adapter %s\n\r"
#define CMSM_EVENT_NOTIFI_MSG						\
	(MEON_STRING *)"CMSM Event Notification"
#define CMSM_POLLING_ROUTINE_MSG 					\
	(MEON_STRING *)"CMSM Polling Routine"
#define CMSM_ERROR_ADD_POLLING_MSG					\
	(MEON_STRING *)"CMSM The polling procedure cannot be added."
#define CMSM_ERROR_ADD_EVENT_NOTIFI_MSG					\
	(MEON_STRING *)"CMSM Event notifi cannot be registered."
#define CMSM_POLLING_NEEDS_HSM_ROUTINE_MSG				\
	(MEON_STRING *)"%s-NW-: Cannot add poll without an HSM routine.\n\r"
#define	CMSM_MLID_BOARD_MSG						\
	(MEON_STRING *)"%s MLID Board"
#define CMSM_NO_MLID_RESOURCE_TAG_MSG					\
	(MEON_STRING *)"%s-NW-: A resource tag is unavailable.\n\r"
#define CMSM_MLID_FAILED_LSL_REGISTER_MSG				\
	(MEON_STRING *)"%s-NW-: The MLID was not registered with the LSL.\n\r"
#define	CMSM_IPX_MSG							\
	(MEON_STRING *)"IPX"
#define	CMSM_UNABLE_TO_ALLOCATE_MEM_MSG					\
	(MEON_STRING *)"%s-NW-: Unable to allocate memory.\n\r"
#define CMSM_MEDIA_PARM_BLOCK_SIZE_MSG					\
	(MEON_STRING *)"%s-NW-: The media parameter block is too small.\n\r"
#define CMSM_ALLOC_MEM							\
	(MEON_STRING *)"%s Alloc Memory"
#define CMSM_CACHE_BELOW_16_MEG_MSG					\
	(MEON_STRING *)"%s Cache Below 16 Meg"
#define CMSM_HARDWARE_OPTIONS_MSG					\
	(MEON_STRING *)"%s Hardware Options"
#define	CMSM_NOFILE_MSG							\
	(MEON_STRING *)"%s-NW-: The file %s cannot be found.\n\r"
#define	CMSM_BADFILE_MSG						\
    	(MEON_STRING *)"%s-NW-: The file %s is not a regular file.\n\r"
#define	CMSM_NOOPEN_MSG							\
	(MEON_STRING *)"%s-NW-: The file %s cannot be opened.\n\r"
#define	CMSM_NOSTAT_MSG							\
	(MEON_STRING *)"%s-NW-: The firmware file %s cannot be stat'ed.\n\r"
#define	CMSM_BADSZ_MSG							\
    	(MEON_STRING *)"%s-NW-: The file %s is zero length or too big.\n\r"
#define	CMSM_FILE_NOMEM_MSG						\
	(MEON_STRING *)"%s-NW-: Cannot allocate memory for file.\n\r"
#define	CMSM_NORD_MSG						\
	(MEON_STRING *)"%s-NW-: The file %s cannot be read.\n\r"
#define	CMSM_FIRMWARE_ERROR_MSG						\
	(MEON_STRING *)"%s-NW-: Error during firmware file read.\n\r"
#define	CMSM_MSGFILE_BIGCOUNT_MSG					\
	(MEON_STRING *)"%s-NW-: Count (%d) in message file is too big.\n\r"
#define	CMSM_MSGFILE_BADPTR_MSG						\
	(MEON_STRING *)"%s-NW-: Pointer (%d) in message file is too big.\n\r"
#define	CMSM_MSGFILE_ERROR_MSG						\
	(MEON_STRING *)"%s-NW-: Error during message file read.\n\r"
#define	CMSM_HARDWARE_CONFIG_CONFLICT_MSG				\
	(MEON_STRING *)"%s-NW-: The hardware configuration conflicts.\n\r"
#define	CMSM_NO_IO_OR_MEM_MSG						\
	(MEON_STRING *)"%s-NW-: Niether IO mapped, nor MEM mapped adapter.\n\r"
#define CMSM_VIRTAL_ADAPTER_MISSING_MSG					\
	(MEON_STRING *)"%s-NW-: Matching adapter could not be found.\n\r"
#define	CMSM_ECB_BUFFERS_MSG						\
	(MEON_STRING *)"%s ECB Buffers"
#define	CMSM_NEED_HSM_INT_ROUTINE_MSG					\
	(MEON_STRING *)"%s-NW-: Cannot set INT without an HSM routine.\n\r"
#define	CMSM_SPURIOUS_INT_MSG					\
	(MEON_STRING *)"-NW-: Spurious interrupt %d.\n\r"
#define CMSM_HARDWARE_ISR_MSG 						\
	(MEON_STRING *)"%s Hardware ISR"
#define	CMSM_HARDWARE_INT_NOT_SET_MSG					\
	(MEON_STRING *)"%s-NW-: The hardware interrupt cannot be set.\n\r"
#define	CMSM_NOAES_MSG							\
	(MEON_STRING *)"%s-NW-: Unable to set AES event\n\r "
#define	CMSM_NOINTAES_MSG						\
	(MEON_STRING *)"%s-NW-: Unable to set INT AES event\n\r "
#define	CMSM_TSM_NOINIT_MSG						\
	(MEON_STRING *)"%s-NW-: TSM could not be initialized\n\r"
#define	CMSM_TSM_PARSE_PARM_MSG						\
	(MEON_STRING *)"%s-NW-: TSM could not parse parameters\n\r"
#define	CMSM_PREFIX_MSG	     						\
	(MEON_STRING *)"%.8s-NW-"
#define	CMSM_PRINT_STRING_MSG						\
	(MEON_STRING *)"%.8s-NW-Adapter %x-Board %x:\r\n%s"
#define	CMSM_ASM_PRINT_STRING_MSG					\
	(MEON_STRING *)"%.8s-NW-Adapter %x-Board %x:\r\n%d:%s"
#define	CMSM_PARSER_NEWLINE_MSG						\
	(MEON_STRING *)"\n"
#define	CMSM_PARSER_PERIOD_MSG						\
	(MEON_STRING *)"."
#define	CMSM_PARSER_USING_MSG 						\
	(MEON_STRING *)" using"
#define	CMSM_PARSER_IO_PORTS_MSG					\
	(MEON_STRING *)" I/O ports "
#define	CMSM_PARSER_TO_MSG  						\
	(MEON_STRING *)" to "
#define	CMSM_PARSER_AND_MSG  						\
	(MEON_STRING *)" and "
#define	CMSM_PARSER_MEMORY_MSG						\
	(MEON_STRING *)", Memory "
#define	CMSM_PARSER_INTERRUPT_MSG					\
	(MEON_STRING *)", Interrupt "
#define	CMSM_PARSER_DMA_CHANNEL_MSG					\
	(MEON_STRING *)", DMA Channel "
#define	CMSM_PARSER_SLOT_MSG						\
	(MEON_STRING *)" SLOT "
#define	CMSM_PARSER_BOARD_TO_LOAD_MSG					\
	(MEON_STRING *)"\nSelect Board To Load : "
#define	CMSM_PARSER_1_MSG						\
	(MEON_STRING *)"1"
#define	CMSM_PARSER_0_MSG						\
	(MEON_STRING *)"0"
#define	CMSM_PARSER_INT_TXTMSG						\
	(MEON_STRING *)"INT=%X"
#define	CMSM_PARSER_INT1_TXTMSG						\
	(MEON_STRING *)"INT1=%X"
#define	CMSM_PARSER_PORT_TXTMSG						\
	(MEON_STRING *)"PORT=%X"
#define	CMSM_PARSER_PORT0_TXTMSG					\
	(MEON_STRING *)"PORT0"
#define	CMSM_PARSER_PORT1_TXTMSG					\
	(MEON_STRING *)"PORT1=%X"
#define	CMSM_PARSER_DMA_TXTMSG						\
	(MEON_STRING *)"DMA=%X"
#define	CMSM_PARSER_DMA0_TXTMSG						\
	(MEON_STRING *)"DMA0"
#define	CMSM_PARSER_DMA1_TXTMSG						\
	(MEON_STRING *)"DMA1=%X"
#define	CMSM_PARSER_MEM_TXTMSG						\
	(MEON_STRING *)"MEM=%X"
#define	CMSM_PARSER_MEM0_TXTMSG						\
	(MEON_STRING *)"MEM0"
#define	CMSM_PARSER_MEM1_TXTMSG	   					\
	(MEON_STRING *)"MEM1=%X"
#define	CMSM_PARSER_SLOT_TXTMSG						\
	(MEON_STRING *)"SLOT=%X"
#define	CMSM_PARSER_CHANNEL_TXTMSG					\
	(MEON_STRING *)"=CHANNEL=%X"
#define	CMSM_PARSER_RETRIES_TXTMSG					\
	(MEON_STRING *)"RETRIES=%U"
#define	CMSM_PARSER_BUFFERS16_EQUAL_TXTMSG				\
	(MEON_STRING *)"BUFFERS16=%U"
#define	CMSM_PARSER_BUFFERS16_TXTMSG					\
	(MEON_STRING *)"BUFFERS16"
#define	CMSM_PARSER_BELOW16_TXTMSG					\
	(MEON_STRING *)"BELOW16"
#define	CMSM_PARSER_NODE_EQUAL_TXTMSG					\
	(MEON_STRING *)"NODE=%16s"
#define	CMSM_PARSER_NODE_TXTMSG						\
	(MEON_STRING *)"NODE"
#define	CMSM_PARSER_BUS_EQUAL_TXTMSG					\
	(MEON_STRING *)"BUS=%s"
#define	CMSM_PARSER_NAME_EQUAL_TXTMSG					\
	(MEON_STRING *)"NAME=%s"
#define	CMSM_PARSER_INTO_TXTMSG						\
	(MEON_STRING *)"INT0"
#define	CMSM_PARSER_WHITESPACE_SEPARATORS_MSG				\
	(MEON_STRING *)" \t\n"
#define	CMSM_PARSER_WHITESPACE_EQUAL_SEPARATORS_MSG			\
	(MEON_STRING *)" \t\n="
#define	CMSM_PARSER_SQUARE_BRACKET_MSG 					\
	(MEON_STRING *)"]"
#define	CMSM_PARSER_BOARD_NAME_TO_LONG_MSG				\
	(MEON_STRING *)"Board name is too long and was truncated: %s\n"
#define	CMSM_PARSER_BOARD_NAME_USED_MSG					\
	(MEON_STRING *)"Board name %s is already in use and was ignored.\n"
#define CMSM_PARSER_BOARD_NAME_MISSING_MSG				\
	(MEON_STRING *)"Expected board name missing or invalid.\n"
#define	CMSM_PARSER_VALID_INTS_MSG					\
	(MEON_STRING *)"-+0123456789"
#define	CMSM_PARSER_VALID_U_INTS_MSG					\
	(MEON_STRING *)"0123456789"
#define	CMSM_PARSER_VALID_X_INTS_MSG					\
	(MEON_STRING *)"0123456789ABCDEFabcdef"
#define	CMSM_PARSER_VALID_STRINGS_MSG					\
	(MEON_STRING *)"\x20..\x7e"
#define	CMSM_PARSER_VALID_YES_NO_MSG					\
	(MEON_STRING *)"YyNn"
#define	CMSM_PARSER_LOAD_ANOTHER_FRAME_MSG				\
	(MEON_STRING *)"Do you want to load another frame type for a previously loaded board? : "
#define	CMSM_PARSER_N_MSG						\
	(MEON_STRING *)"n"
#define	CMSM_PARSER_ENTER1_VALUE_MSG					\
	(MEON_STRING *)"\nEnter the value for "
#define	CMSM_PARSER_SPACE_COLON_SPACE_MSG				\
	(MEON_STRING *)" : "
#define	CMSM_PARSER_SUPPORTED_MSG					\
	(MEON_STRING *)"Supported "
#define	CMSM_PARSER_VALUES_ARE_MSG					\
	(MEON_STRING *)" values are : "
#define	CMSM_PARSER_PERIOD_SPACE_MSG					\
	(MEON_STRING *)". "
#define	CMSM_PARSER_COMMA_SPACE_MSG					\
	(MEON_STRING *)", "
#define	CMSM_PARSER_ENTER_VALUE_MSG					\
	(MEON_STRING *)"Enter Value for "
#define	CMSM_PARSER_BETWEEN_MSG						\
	(MEON_STRING *)" between "
#define	CMSM_PARSER_SPACE_MINUS_SPACE_MSG				\
	(MEON_STRING *)" - "
#define	CMSM_PARSER_PERCENT_S_MSG					\
	(MEON_STRING *)"%s"
#define	CMSM_PARSER_COMMAND_PROMPT_MSG					\
	(MEON_STRING *)"Command Line Parameter Prompt"
#define	CMSM_PARSER_NODE_ADDR_ERROR_MSG					\
	(MEON_STRING *)"%s-NW-: The node address override is invalid"
#define	CMSM_PARSER_KEYWORD_OUT_RANGE_MSG				\
	(MEON_STRING *)"A Keyword was out of range or a required param is missing.\r\n"
#define	CMSM_PARSER_FRAME_TXTMSG					\
	(MEON_STRING *)"FRAME"
#define	CMSM_ERROR_NESL_REGISTER_MSG					\
	(MEON_STRING *)"Event registration with NESL failed. \n\r"
#define	CMSM_PARSER_LSB_TXTMSG						\
	"LSB"
#define	CMSM_ODI_SPECVER_MSG						\
	"ODI_CSPEC_VERSION: 1.10"
#define CMSM_PARSER_DRVPARM_ERROR_MSG					\
	"Error parsing for driver parameter.\n\r"
#define CMSM_LOCK_ERR_MSG						\
	(MEON_STRING *)"%s-NW-: Could not obtain rdlock for %s.\n\r"

#endif	/* _IO_MSM_MSMTXT_H */
