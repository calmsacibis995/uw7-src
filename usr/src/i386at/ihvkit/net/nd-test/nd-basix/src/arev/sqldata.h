#ifndef BYTE
   #define BYTE  unsigned char
#endif
#ifndef DWORD
   #define DWORD  unsigned long
#endif
#ifndef LONG
   #define LONG  unsigned long
#endif
#if defined(__BCPLUSPLUS__)

	#ifndef CHAR
	   #define CHAR  char 
	#endif
	
	#ifndef INT
	   #define INT  int 
	#endif

	#ifndef WORD
	   #define WORD unsigned int 
	#endif

#else

	#ifndef CHAR
	   #define CHAR  signed char 
	#endif

	#ifndef INT
  	   #define INT	short
	#endif

	#ifndef WORD
	   #define WORD ushort 
	#endif

#endif


#define MAX_LAN_CHANNELS        9
#define FILESERVER		0
#define WORKSTATION		1

#define	TEST_RESULT		0
#define	LAN_ADAPTER		1
#define	HBA_CARD		0
#define	GET_TESTNUMBER_ID	4
#define	SQL_COMMAND		13

#define PRODUCT_LIST		2
#define TEST_REMARK		3
#define GET_SESSION		4  /*TEST NUMBER ID */
#define TEST_UPDATE		5
#define NEW_PRODUCT		6
#define NEW_DISKETTE		7
#define NEW_CONTROLLER		8
#define NEW_DRIVE		9
#define NEW_COMMENT		10
#define NEXTID			11

#define ERROR			0
#define OK			1
#define END			-109


/************** Added status,control flags on July 7, 1994 ****************/

/*****status flags ***********/
#define PASS 0
#define INCOMPLETE 1
#define FAIL 2
#define OPTIONAL 3

/*******Control flags *******/
#define NO_FLAGS 0

typedef struct
{
	INT Type; 
	WORD Length;		
	char Data[530];
}Packet_Struct;

/********* This is the System Information, You are calling this as SYSTEM
           Structure
**************************/
typedef struct
{
	INT Length;		
	INT DataBaseID;
	DWORD MachineID;
	INT CompanyID;
	INT TestType;
	char Name[60];
	char Model[60];
	char Revision[36];
	char CPUMan[50];
	char CPUType[16];
	char CPUSpeed[10];
	char Serial[36];
	char CPUBrdRev[36];
	char Bios[70];
	INT  Memory;
        char FController[46];
        char DriveA[6];
        char DriveB[6];
	char Eisaconfig[16];
	char EisaDate[10];
        DWORD EisaSize;
	char BootDevice[26];
} Product_Struct;



typedef struct
{
	INT Type; 
	WORD Length;		
	WORD DataBaseID;
	WORD TestNumberID;
	WORD IndexNumber;
	INT AdapterType;
	WORD Number;
	char Name[80];
	INT IRQ;
	long IO;
	long RAM;
	long ROM;
	INT DMA;
	char DriverName[16];
	char DriverDate[16];
	long DriverSize;
} Adapter_Struct;

typedef struct
{
	INT Type; 
	WORD Length;		
	WORD DataBaseID;

	/****Additional fields added to suit systems group's 
	     Request on July 7, 1994
	***************************************************/
	WORD TestNumberID;
	WORD SectionNumber;
	WORD SuiteNumber;
	
	WORD TestNumber;  /**previous field Name: Test_Number; **/
	                  /**WORD Session_Number;              **/
	WORD ControlFlags;/** set to unsigned int 0 	       **/
	INT  Iteration;
	INT  Executions;  /** set to int 0		       **/
	INT  Status;      /**previous field Name: Error_Count; **/
			  /**Valid Values:PASS/FAIL/INCOMPLETE **/
	       
                          /* Field Removed : char Kbytes[10];  **/
	char OSName[36];  /**previous field Name: Dos[25];     **/
	char Program[20]; /**Name of the certification program **/
			  /** "uwcert"                         **/
  	
	char Version[36];  /**previous field Name: Dos[25];     **/
	long StartTime;
	long StopTime;
} Test_Struct;

typedef struct
{
	INT Type; 
	WORD Length;		
	WORD DataBaseID;
	WORD TestNumberID;   /**previous field :Test_Number; **/
	WORD TestNumber;     /**previous field :Session_Number;**/
	char Remarks[250];
} Remarks_Struct;

typedef struct
{
	WORD DataBaseID;
	char Name[60];
	char Model[60];
	char Revision[50];
} Identify_Struct;

struct Link
{
	struct Link	*next;
	WORD	DataBaseID;
	char	String[80];
};

struct Node
{
	struct Node *next;	
	Adapter_Struct Adapter;
};
