#ifndef	_IO_MSM_MSMPARSE_H
#define	_IO_MSM_MSMPARSE_H

#ident	"@(#)msmparse.h	2.1"

/*
 * type field defines.
 */
#define CUSTOMPARAM		0x0000
#define INTPARAM		0x0001
#define INT1PARAM		0x0002
#define PORTPARAM		0x0003
#define PORT1PARAM		0x0004
#define DMAPARAM		0x0005
#define DMA1PARAM		0x0006
#define MEMPARAM		0x0007
#define MEM1PARAM		0x0008
#define SLOTPARAM		0x0009
#define NODEPARAM		0x000A
#define CHANNELPARAM		0x000B
#define FRAMEPARAM		0x000C
#define BUSPARAM		0x000D
#define NAMEPARAM		0x000E
#define RETRIESPARAM		0x000F
#define BELOW16PARAM		0x0010
#define BUFFERS16PARAM		0x0011

/*
 * flags field defines.
 */
#define	OPTIONALPARAM		0x0000
#define	REQUIREDPARAM		0x0001
#define	DEFAULTPRESENT		0x0002
#define	KEYWORDPARAM		0x0010
#define	ENUMPARAM		0x0020
#define	RANGEPARAM		0x0040
#define	STRINGPARAM		0x0080
#define	SHARABLE   		0x0100
#define  PARAMMASK         	0x00F3

/*
 * parse types
 */
#define	BADTYPE			0x0000
#define	STRINGTYPE		0x0004
#define	INTTYPE			0x0008
#define	UINTTYPE		0x0010
#define	XINTTYPE		0x0020
#define	TYPEMASK		0x003C

#define	MAX_PROMPT_SIZE		512
#define	MAX_PARAM_LEN		80

struct   _CONFIG_TABLE_;

typedef struct _MODULE_TRACK_ {
	struct _MODULE_TRACK_	*NextLink;
   	struct _MODULE_HANDLE_	*ModuleHandle;
   	struct _CONFIG_TABLE_   *OriginalConfig;
	UINT32			Count;
} MODULE_TRACK;

/*
 * the following declaration is intented to illustrate the form of
 * a parameter option structure. an actual declartion will 
 * probably not include the union since there is no real use for it.
 */
typedef struct _PARAMETER_OPTIONS_ {
	UINT32	OptionCount;
	union {
		int		NumOptVal[1];
		UINT32		UNumOptVal[1];
		MEON_STRING	*StrOptVal[1];
		MEON_STRING	CharOptVal[1];
	} ENUM_LIST;
} PARAMETER_OPTIONS;

typedef struct _DRIVER_OPTION_ {
	struct _DRIVER_OPTION_	*Link;
	MEON_STRING		*ParseString;
	union {
		PARAMETER_OPTIONS	*OptionPtr;
		int			Min;
	} Parameter0;

	union {
		UINT32			Range;
		int			Max;
	} Parameter1;

	union {
		int			Default;
		MEON_STRING		*StringDefault;
	} Parameter2;

	UINT16				Type;
	UINT16				Flags;
	MEON_STRING			String[MAX_PARAM_LEN];
} DRIVER_OPTION;

typedef struct _COMMAND_STRUCT_ {
	struct _COMMAND_STRUCT_	*CommandLineNext;
	MEON			*CommandLineBuffer;
} COMMAND_STRUCT;

#endif	/* _IO_MSM_MSMPARSE_H */
