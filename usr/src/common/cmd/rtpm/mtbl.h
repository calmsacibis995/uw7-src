#ident	"@(#)rtpm:mtbl.h	1.4.1.1"

#define HZ_TITLE	gettxt("RTPM:192", "hz")
#define HZ_IDX		0
#define hz (mettbl[0].metval->met.sngl)

#define NCPU_TITLE	gettxt("RTPM:193", "cpu")
#define NCPU_IDX	1
#define ncpu (mettbl[1].metval->met.sngl)

#define NDISK_TITLE	gettxt("RTPM:194", "disk")
#define NDISK_IDX	2
#define ndisk (mettbl[2].metval->met.sngl)

#define NFSTYP_TITLE	gettxt("RTPM:194", "fstype")
#define NFSTYP_IDX	3
#define nfstyp (mettbl[3].metval->met.sngl)

#define FSNAMES_TITLE	gettxt("RTPM:196", "fsnames")
#define FSNAMES_IDX	4
#define fsname (mettbl[4].metval)

#define KMPOOLS_TITLE	gettxt("RTPM:197", "kmpool")
#define KMPOOLS_IDX	5
#define nkmpool (mettbl[5].metval->met.sngl)

#define KMASIZE_TITLE	gettxt("RTPM:198", "kmasize")
#define KMASIZE_IDX	6
#define kmasize mettbl[6].metval

#define PGSZ_TITLE	gettxt("RTPM:199", "pgsz")
#define PGSZ_IDX	7
#define pgsz (mettbl[7].metval->met.sngl)

#define DS_NAME_TITLE	gettxt("RTPM:200", "dsname")
#define DS_NAME_IDX	8
#define dsname (mettbl[8].metval)

#define NETHER	(CALC_MET+1)
#define NETHER_TITLE	gettxt("RTPM:201", "nether")
#define NETHER_IDX	9
#define nether (mettbl[9].metval->met.sngl)

#define ETHNAME	(CALC_MET+2)
#define ETHNAME_TITLE	gettxt("RTPM:202", "ethname")
#define ETHNAME_IDX	10
#define ethname (mettbl[10].metval)

	/* Maximum Configured SPX connections. */
#define SPX_max_connections		(CALC_MET + 2960)
#define SPX_max_connections_TITLE	gettxt("RTPM:669", "spx_max_connections")
#define SPX_max_connections_IDX 	11
#define maxspxconn	mettbl[11].metval->met.sngl

#define METSTART 12	/* point where resources end / real mets start */

#define USR_TIME_TITLE	gettxt("RTPM:203", "%%usr")
#define USR_TIME_IDX	METSTART + 0

#define SYS_TIME_TITLE	gettxt("RTPM:204", "%%sys")
#define SYS_TIME_IDX	METSTART + 1

#define WIO_TIME_TITLE	gettxt("RTPM:205", "%%wio")
#define WIO_TIME_IDX	METSTART + 2

#define IDL_TIME_TITLE	gettxt("RTPM:206", "%%idl")
#define IDL_TIME_IDX	METSTART + 3

#define IGET_TITLE	gettxt("RTPM:207", "iget/s")
#define IGET_IDX	METSTART + 4

#define DIRBLK_TITLE	gettxt("RTPM:208", "dirblk/s")
#define DIRBLK_IDX	METSTART + 5

#define IPAGE_TITLE	gettxt("RTPM:209", "ipage/s")
#define IPAGE_IDX	METSTART + 6

#define INOPAGE_TITLE	gettxt("RTPM:210", "inopage/s")
#define INOPAGE_IDX	METSTART + 7

#define FREEMEM_TITLE	gettxt("RTPM:211", "freemem")
#define FREEMEM_IDX	METSTART + 8

#define FREESWAP_TITLE	gettxt("RTPM:212", "freeswp")
#define FREESWAP_IDX	METSTART + 9

#define FSWIO_TITLE	gettxt("RTPM:213", "fswio")
#define FSWIO_IDX	METSTART + 10

#define PHYSWIO_TITLE	gettxt("RTPM:214", "physwio")
#define PHYSWIO_IDX	METSTART + 11

#define RUNQ_TITLE	gettxt("RTPM:215", "runq")
#define RUNQ_IDX	METSTART + 12

#define RUNOCC_TITLE	gettxt("RTPM:216", "%%runocc")
#define RUNOCC_IDX	METSTART + 13

#define SWPQ_TITLE	gettxt("RTPM:217", "swpq")
#define SWPQ_IDX	METSTART + 14

#define SWPOCC_TITLE	gettxt("RTPM:218", "%%swpocc")
#define SWPOCC_IDX	METSTART + 15

#define PROCFAIL_TITLE	gettxt("RTPM:219", "procfail")
#define PROCFAIL_IDX	METSTART + 16

#define PROCFAILR_TITLE	gettxt("RTPM:220", "procfail/s")
#define PROCFAILR_IDX	METSTART + 17

#define PROCUSE_TITLE	gettxt("RTPM:221", "procinuse")
#define PROCUSE_IDX	METSTART + 18

#define PROCMAX_TITLE	gettxt("RTPM:222", "procmax")
#define PROCMAX_IDX	METSTART + 19

#define KMEM_MEM_TITLE	gettxt("RTPM:223", "mem")
#define KMEM_MEM_IDX	METSTART + 20

#define KMEM_BALLOC_TITLE	gettxt("RTPM:224", "balloc")
#define KMEM_BALLOC_IDX	METSTART + 21

#define KMEM_RALLOC_TITLE	gettxt("RTPM:225", "ralloc")
#define KMEM_RALLOC_IDX	METSTART + 22

#define KMEM_FAIL_TITLE	gettxt("RTPM:226", "kmfail")
#define KMEM_FAIL_IDX	METSTART + 23

#define KMEMR_FAIL_TITLE	gettxt("RTPM:227", "kmfail/s")
#define KMEMR_FAIL_IDX	METSTART + 24

#define PREATCH_TITLE	gettxt("RTPM:228", "preatch/s")
#define PREATCH_IDX	METSTART + 25

#define ATCH_TITLE	gettxt("RTPM:229", "atch/s")
#define ATCH_IDX	METSTART + 26

#define ATCHFREE_TITLE	gettxt("RTPM:230", "atchfree/s")
#define ATCHFREE_IDX	METSTART + 27

#define ATCHFREE_PGOUT_TITLE	gettxt("RTPM:231", "atfrpgot/s")
#define ATCHFREE_PGOUT_IDX	METSTART + 28

#define ATCHMISS_TITLE	gettxt("RTPM:232", "atchmiss/s")
#define ATCHMISS_IDX	METSTART + 29

#define PGIN_TITLE	gettxt("RTPM:233", "pgin/s")
#define PGIN_IDX	METSTART + 30

#define PGPGIN_TITLE	gettxt("RTPM:234", "pgpgin/s")
#define PGPGIN_IDX	METSTART + 31

#define PGOUT_TITLE	gettxt("RTPM:235", "pgout/s")
#define PGOUT_IDX	METSTART + 32

#define PGPGOUT_TITLE	gettxt("RTPM:236", "pgpgout/s")
#define PGPGOUT_IDX	METSTART + 33

#define SWPOUT_TITLE	gettxt("RTPM:237", "swpout/s")
#define SWPOUT_IDX	METSTART + 34

#define PPGSWPOUT_TITLE	gettxt("RTPM:238", "ppgswpot/s")
#define PPGSWPOUT_IDX	METSTART + 35

#define VPGSWPOUT_TITLE	gettxt("RTPM:239", "vpgswpot/s")
#define VPGSWPOUT_IDX	METSTART + 36

#define SWPIN_TITLE	gettxt("RTPM:240", "swpin/s")
#define SWPIN_IDX	METSTART + 37

#define PGSWPIN_TITLE	gettxt("RTPM:241", "pgswpin/s")
#define PGSWPIN_IDX	METSTART + 38

#define VIRSCAN_TITLE	gettxt("RTPM:242", "virscan/s")
#define VIRSCAN_IDX	METSTART + 39

#define VIRFREE_TITLE	gettxt("RTPM:243", "virfree/s")
#define VIRFREE_IDX	METSTART + 40

#define PHYSFREE_TITLE	gettxt("RTPM:244", "physfree/s")
#define PHYSFREE_IDX	METSTART + 41

#define PFAULT_TITLE	gettxt("RTPM:245", "pfault/s")
#define PFAULT_IDX	METSTART + 42

#define VFAULT_TITLE	gettxt("RTPM:246", "vfault/s")
#define VFAULT_IDX	METSTART + 43

#define SFTLCK_TITLE	gettxt("RTPM:247", "sftlck/s")
#define SFTLCK_IDX	METSTART + 44

#define SYSCALL_TITLE	gettxt("RTPM:248", "syscall/s")
#define SYSCALL_IDX	METSTART + 45

#define FORK_TITLE	gettxt("RTPM:249", "fork/s")
#define FORK_IDX	METSTART + 46

#define LWPCREATE_TITLE	gettxt("RTPM:250", "lwpcreat/s")
#define LWPCREATE_IDX	METSTART + 47

#define EXEC_TITLE	gettxt("RTPM:251", "exec/s")
#define EXEC_IDX	METSTART + 48

#define READ_TITLE	gettxt("RTPM:252", "read/s")
#define READ_IDX	METSTART + 49

#define WRITE_TITLE	gettxt("RTPM:253", "write/s")
#define WRITE_IDX	METSTART + 50

#define READCH_TITLE	gettxt("RTPM:254", "readch/s")
#define READCH_IDX	METSTART + 51

#define WRITECH_TITLE	gettxt("RTPM:255", "writech/s")
#define WRITECH_IDX	METSTART + 52

#define LOOKUP_TITLE	gettxt("RTPM:256", "lookup/s")
#define LOOKUP_IDX	METSTART + 53

#define DNLCHITS_TITLE	gettxt("RTPM:257", "dnlchits/s")
#define DNLCHITS_IDX	METSTART + 54

#define DNLCMISS_TITLE	gettxt("RTPM:258", "dnlcmiss/s")
#define DNLCMISS_IDX	METSTART + 55

#define FILETBLINUSE_TITLE	gettxt("RTPM:259", "fltblinuse")
#define FILETBLINUSE_IDX	METSTART + 56

#define FILETBLFAIL_TITLE	gettxt("RTPM:260", "fltblfail")
#define FILETBLFAIL_IDX	METSTART + 57

#define FILETBLFAILR_TITLE	gettxt("RTPM:261", "fltblfail/s")
#define FILETBLFAILR_IDX	METSTART + 58

#define FLCKTBLMAX_TITLE	gettxt("RTPM:262", "flcktblmax")
#define FLCKTBLMAX_IDX	METSTART + 59

#define FLCKTBLINUSE_TITLE	gettxt("RTPM:263", "flcktbluse")
#define FLCKTBLINUSE_IDX	METSTART + 60

#define FLCKTBLFAIL_TITLE	gettxt("RTPM:264", "flcktblfal")
#define FLCKTBLFAIL_IDX	METSTART + 61

#define FLCKTBLFAILR_TITLE	gettxt("RTPM:265", "flcktblfail/s")
#define FLCKTBLFAILR_IDX	METSTART + 62

#define FLCKTBLTOTAL_TITLE	gettxt("RTPM:266", "flcktbl/s")
#define FLCKTBLTOTAL_IDX	METSTART + 63

#define MAXINODE_TITLE	gettxt("RTPM:267", "maxinode")
#define MAXINODE_IDX	METSTART + 64

#define CURRINODE_TITLE	gettxt("RTPM:268", "currinode")
#define CURRINODE_IDX	METSTART + 65

#define INUSEINODE_TITLE	gettxt("RTPM:269", "inodeinuse")
#define INUSEINODE_IDX	METSTART + 66

#define FAILINODE_TITLE	gettxt("RTPM:270", "inodefail")
#define FAILINODE_IDX	METSTART + 67

#define FAILINODER_TITLE	gettxt("RTPM:271", "inodefail/s")
#define FAILINODER_IDX	METSTART + 68

#define MPS_PSWITCH_TITLE	gettxt("RTPM:272", "pswtch/s")
#define MPS_PSWITCH_IDX	METSTART + 69

#define MPS_RUNQUE_TITLE	gettxt("RTPM:273", "prunq")
#define MPS_RUNQUE_IDX	METSTART + 70

#define MPS_RUNOCC_TITLE	gettxt("RTPM:274", "%%prunocc")
#define MPS_RUNOCC_IDX	METSTART + 71

#define MPB_BREAD_TITLE	gettxt("RTPM:275", "bread/s")
#define MPB_BREAD_IDX	METSTART + 72

#define MPB_BWRITE_TITLE	gettxt("RTPM:276", "bwrite/s")
#define MPB_BWRITE_IDX	METSTART + 73

#define MPB_LREAD_TITLE	gettxt("RTPM:277", "lread/s")
#define MPB_LREAD_IDX	METSTART + 74

#define MPB_LWRITE_TITLE	gettxt("RTPM:278", "lwrite/s")
#define MPB_LWRITE_IDX	METSTART + 75

#define MPB_PHREAD_TITLE	gettxt("RTPM:279", "phread/s")
#define MPB_PHREAD_IDX	METSTART + 76

#define MPB_PHWRITE_TITLE	gettxt("RTPM:280", "phwrite/s")
#define MPB_PHWRITE_IDX	METSTART + 77

#define MPT_RCVINT_TITLE	gettxt("RTPM:281", "rcvint/s")
#define MPT_RCVINT_IDX	METSTART + 78

#define MPT_XMTINT_TITLE	gettxt("RTPM:282", "xmtint/s")
#define MPT_XMTINT_IDX	METSTART + 79

#define MPT_MDMINT_TITLE	gettxt("RTPM:283", "mdmint/s")
#define MPT_MDMINT_IDX	METSTART + 80

#define MPT_RAWCH_TITLE	gettxt("RTPM:284", "rawch/s")
#define MPT_RAWCH_IDX	METSTART + 81

#define MPT_CANCH_TITLE	gettxt("RTPM:285", "canch/s")
#define MPT_CANCH_IDX	METSTART + 82

#define MPT_OUTCH_TITLE	gettxt("RTPM:286", "outch/s")
#define MPT_OUTCH_IDX	METSTART + 83

#define MPI_MSG_TITLE	gettxt("RTPM:287", "ipcmsgq/s")
#define MPI_MSG_IDX	METSTART + 84

#define MPI_SEMA_TITLE	gettxt("RTPM:288", "ipcsema/s")
#define MPI_SEMA_IDX	METSTART + 85

#define MPR_LWP_FAIL_TITLE	gettxt("RTPM:289", "lwpfail")
#define MPR_LWP_FAIL_IDX	METSTART + 86

#define MPR_LWP_FAILR_TITLE	gettxt("RTPM:290", "lwpfail/s")
#define MPR_LWP_FAILR_IDX	METSTART + 87

#define MPR_LWP_USE_TITLE	gettxt("RTPM:291", "lwpinuse")
#define MPR_LWP_USE_IDX	METSTART + 88

#define MPR_LWP_MAX_TITLE	gettxt("RTPM:292", "lwpmax")
#define MPR_LWP_MAX_IDX	METSTART + 89

#define DS_CYLS_TITLE	gettxt("RTPM:293", "dscyl")
#define DS_CYLS_IDX	METSTART + 90

#define DS_FLAGS_TITLE	gettxt("RTPM:294", "dsflags")
#define DS_FLAGS_IDX	METSTART + 91

#define DS_QLEN_TITLE	gettxt("RTPM:295", "instqlen")
#define DS_QLEN_IDX	METSTART + 92

#define DS_ACTIVE_TITLE	gettxt("RTPM:296", "%%busy")
#define DS_ACTIVE_IDX	METSTART + 93

#define DS_RESP_TITLE	gettxt("RTPM:297", "avgqlen")
#define DS_RESP_IDX	METSTART + 94

#define DS_READ_TITLE	gettxt("RTPM:298", "dsread/s")
#define DS_READ_IDX	METSTART + 95

#define DS_READBLK_TITLE	gettxt("RTPM:299", "dsrblk/s")
#define DS_READBLK_IDX	METSTART + 96

#define DS_WRITE_TITLE	gettxt("RTPM:300", "dswrit/s")
#define DS_WRITE_IDX	METSTART + 97

#define DS_WRITEBLK_TITLE	gettxt("RTPM:301", "dswblk/s")
#define DS_WRITEBLK_IDX	METSTART + 98

#define TOT_CPU		(CALC_MET+10)
#define TOT_CPU_TITLE	gettxt("RTPM:302", "%%(usr+sys)")
#define TOT_CPU_IDX	METSTART + 99

#define TOT_IDL		(CALC_MET+20)
#define TOT_IDL_TITLE	gettxt("RTPM:303", "%%(wio+idl)")
#define TOT_IDL_IDX	METSTART + 100

#define LWP_SLEEP	(CALC_MET+30)
#define LWP_SLEEP_TITLE	gettxt("RTPM:304", "lwp_sleep")
#define LWP_SLEEP_IDX	METSTART + 101

#define LWP_RUN		(CALC_MET+40)
#define LWP_RUN_TITLE	gettxt("RTPM:305", "lwp_run")
#define LWP_RUN_IDX	METSTART + 102

#define LWP_IDLE	(CALC_MET+50)
#define LWP_IDLE_TITLE	gettxt("RTPM:306", "lwp_idle")
#define LWP_IDLE_IDX	METSTART + 103

#define LWP_ONPROC	(CALC_MET+60)
#define LWP_ONPROC_TITLE	gettxt("RTPM:307", "lwp_onproc")
#define LWP_ONPROC_IDX	METSTART + 104

#define LWP_ZOMB	(CALC_MET+70)
#define LWP_ZOMB_TITLE	gettxt("RTPM:308", "lwp_zombie")
#define LWP_ZOMB_IDX	METSTART + 105

#define LWP_STOP	(CALC_MET+80)
#define LWP_STOP_TITLE	gettxt("RTPM:309", "lwp_stop")
#define LWP_STOP_IDX	METSTART + 106

#define LWP_OTHER	(CALC_MET+90)
#define LWP_OTHER_TITLE	gettxt("RTPM:310", "lwp_other")
#define LWP_OTHER_IDX	METSTART + 107

#define LWP_TOTAL	(CALC_MET+100)
#define LWP_TOTAL_TITLE	gettxt("RTPM:311", "lwp_total")
#define LWP_TOTAL_IDX	METSTART + 108

#define LWP_NPROC	(CALC_MET+110)
#define LWP_NPROC_TITLE	gettxt("RTPM:312", "lwp_nproc")
#define LWP_NPROC_IDX	METSTART + 109

#define TOT_RW		(CALC_MET+120)
#define TOT_RW_TITLE	gettxt("RTPM:313", "(rd+wrt)/s")
#define TOT_RW_IDX	METSTART + 110

#define TOT_KRWCH	(CALC_MET+130)
#define TOT_KRWCH_TITLE	gettxt("RTPM:314", "(r+w)Kb/s")
#define TOT_KRWCH_IDX	METSTART + 111

#define DNLC_PERCENT	(CALC_MET+140)
#define DNLC_PERCENT_TITLE	gettxt("RTPM:315", "%%dnlc")
#define DNLC_PERCENT_IDX	METSTART + 112

#define TOT_KMA_PAGES	(CALC_MET+150)
#define TOT_KMA_PAGES_TITLE	gettxt("RTPM:316", "kma(pg)")
#define TOT_KMA_PAGES_IDX	METSTART + 113

#define ETH_InUcastPkts	(CALC_MET + 160)
#define ETH_InUcastPkts_TITLE	gettxt("RTPM:317", "InUcastPkts/s")
#define ETH_InUcastPkts_IDX	METSTART + 114

#define ETH_OutUcastPkts	(CALC_MET + 170)
#define ETH_OutUcastPkts_TITLE	gettxt("RTPM:318", "OutUcastPkts/s")
#define ETH_OutUcastPkts_IDX	METSTART + 115

#define ETH_InNUcastPkts	(CALC_MET + 180)
#define ETH_InNUcastPkts_TITLE	gettxt("RTPM:319", "InNUcastPkts/s")
#define ETH_InNUcastPkts_IDX	METSTART + 116

#define ETH_OutNUcastPkts	(CALC_MET + 190)
#define ETH_OutNUcastPkts_TITLE	gettxt("RTPM:320", "OutNUcastPkts/s")
#define ETH_OutNUcastPkts_IDX	METSTART + 117

#define ETH_InOctets	(CALC_MET + 200 )
#define ETH_InOctets_TITLE	gettxt("RTPM:321", "InOctets/s")
#define ETH_InOctets_IDX	METSTART + 118

#define ETH_OutOctets	(CALC_MET + 210)
#define ETH_OutOctets_TITLE	gettxt("RTPM:322", "OutOctets/s")
#define ETH_OutOctets_IDX	METSTART + 119

#define ETH_InErrors	(CALC_MET + 220)
#define ETH_InErrors_TITLE	gettxt("RTPM:323", "InErrors")
#define ETH_InErrors_IDX	METSTART + 120

#define ETHR_InErrors_TITLE	gettxt("RTPM:324", "InErrors/s")
#define ETHR_InErrors_IDX	METSTART + 121

#define ETH_etherAlignErrors	(CALC_MET + 230)
#define ETH_etherAlignErrors_TITLE	gettxt("RTPM:325", "etherAlignErrors")
#define ETH_etherAlignErrors_IDX	METSTART + 122

#define ETHR_etherAlignErrors_TITLE	gettxt("RTPM:326", "etherAlignErrors/s")
#define ETHR_etherAlignErrors_IDX	METSTART + 123

#define ETH_etherCRCerrors	(CALC_MET + 240)
#define ETH_etherCRCerrors_TITLE	gettxt("RTPM:327", "etherCRCerrors")
#define ETH_etherCRCerrors_IDX	METSTART + 124

#define ETHR_etherCRCerrors_TITLE	gettxt("RTPM:328", "etherCRCerrors/s")
#define ETHR_etherCRCerrors_IDX	METSTART + 125

#define ETH_etherOverrunErrors	(CALC_MET + 250)
#define ETH_etherOverrunErrors_TITLE	gettxt("RTPM:329", "etherOverrunErrors")
#define ETH_etherOverrunErrors_IDX	METSTART + 126

#define ETHR_etherOverrunErrors_TITLE	gettxt("RTPM:330", "etherOverrunErrors/s")
#define ETHR_etherOverrunErrors_IDX	METSTART + 127

#define ETH_etherUnderrunErrors	(CALC_MET + 260)
#define ETH_etherUnderrunErrors_TITLE	gettxt("RTPM:331", "etherUnderrunErrors")
#define ETH_etherUnderrunErrors_IDX	METSTART + 128

#define ETHR_etherUnderrunErrors_TITLE	gettxt("RTPM:332", "etherUnderrunErrors/s")
#define ETHR_etherUnderrunErrors_IDX	METSTART + 129

#define ETH_etherMissedPkts	(CALC_MET + 270)
#define ETH_etherMissedPkts_TITLE	gettxt("RTPM:333", "etherMissedPkts")
#define ETH_etherMissedPkts_IDX	METSTART + 130

#define ETHR_etherMissedPkts_TITLE	gettxt("RTPM:334", "etherMissedPkts/s")
#define ETHR_etherMissedPkts_IDX	METSTART + 131

#define ETH_InDiscards	(CALC_MET + 280)
#define ETH_InDiscards_TITLE	gettxt("RTPM:335", "InDiscards")
#define ETH_InDiscards_IDX	METSTART + 132

#define ETHR_InDiscards_TITLE	gettxt("RTPM:336", "InDiscards/s")
#define ETHR_InDiscards_IDX	METSTART + 133

#define ETH_etherReadqFull	(CALC_MET + 290)
#define ETH_etherReadqFull_TITLE	gettxt("RTPM:337", "etherReadqFull")
#define ETH_etherReadqFull_IDX	METSTART + 134

#define ETHR_etherReadqFull_TITLE	gettxt("RTPM:338", "etherReadqFull/s")
#define ETHR_etherReadqFull_IDX	METSTART + 135

#define ETH_etherRcvResources	(CALC_MET + 300)
#define ETH_etherRcvResources_TITLE	gettxt("RTPM:339", "etherRcvResources")
#define ETH_etherRcvResources_IDX	METSTART + 136

#define ETHR_etherRcvResources_TITLE	gettxt("RTPM:340", "etherRcvResources/s")
#define ETHR_etherRcvResources_IDX	METSTART + 137

#define ETH_etherCollisions	(CALC_MET + 310)
#define ETH_etherCollisions_TITLE	gettxt("RTPM:341", "etherCollisions")
#define ETH_etherCollisions_IDX	METSTART + 138

#define ETHR_etherCollisions_TITLE	gettxt("RTPM:342", "etherCollisions/s")
#define ETHR_etherCollisions_IDX	METSTART + 139

#define ETH_OutDiscards	(CALC_MET + 320)
#define ETH_OutDiscards_TITLE	gettxt("RTPM:343", "OutDiscards")
#define ETH_OutDiscards_IDX	METSTART + 140

#define ETHR_OutDiscards_TITLE	gettxt("RTPM:344", "OutDiscards/s")
#define ETHR_OutDiscards_IDX	METSTART + 141

#define ETH_OutErrors	(CALC_MET + 330)
#define ETH_OutErrors_TITLE	gettxt("RTPM:345", "OutErrors")
#define ETH_OutErrors_IDX	METSTART + 142

#define ETHR_OutErrors_TITLE	gettxt("RTPM:346", "OutErrors/s")
#define ETHR_OutErrors_IDX	METSTART + 143

#define ETH_etherAbortErrors	(CALC_MET + 340)
#define ETH_etherAbortErrors_TITLE	gettxt("RTPM:347", "etherAbortErrors")
#define ETH_etherAbortErrors_IDX	METSTART + 144

#define ETHR_etherAbortErrors_TITLE	gettxt("RTPM:348", "etherAbortErrors/s")
#define ETHR_etherAbortErrors_IDX	METSTART + 145

#define ETH_etherCarrierLost	(CALC_MET + 350)
#define ETH_etherCarrierLost_TITLE	gettxt("RTPM:349", "etherCarrierLost")
#define ETH_etherCarrierLost_IDX	METSTART + 146

#define ETHR_etherCarrierLost_TITLE	gettxt("RTPM:350", "etherCarrierLost/s")
#define ETHR_etherCarrierLost_IDX	METSTART + 147

#define ETH_OutQlen	(CALC_MET + 360)
#define ETH_OutQlen_TITLE	gettxt("RTPM:351", "OutQlen")
#define ETH_OutQlen_IDX	METSTART + 148

#define IPR_total	(CALC_MET + 380)
#define IPR_total_TITLE	gettxt("RTPM:352", "ip_total/s")
#define IPR_total_IDX	METSTART + 149

#define IP_badsum	(CALC_MET + 390)
#define IP_badsum_TITLE	gettxt("RTPM:353", "ip_badsum")
#define IP_badsum_IDX	METSTART + 150

#define IPR_badsum	(CALC_MET + 400)
#define IPR_badsum_TITLE	gettxt("RTPM:354", "ip_badsum/s")
#define IPR_badsum_IDX	METSTART + 151

#define IP_tooshort	(CALC_MET + 410)
#define IP_tooshort_TITLE	gettxt("RTPM:355", "ip_tooshort")
#define IP_tooshort_IDX	METSTART + 152

#define IPR_tooshort	(CALC_MET + 420)
#define IPR_tooshort_TITLE	gettxt("RTPM:356", "ip_tooshort/s")
#define IPR_tooshort_IDX	METSTART + 153

#define IP_toosmall	(CALC_MET + 430)
#define IP_toosmall_TITLE	gettxt("RTPM:357", "ip_toosmall")
#define IP_toosmall_IDX	METSTART + 154

#define IPR_toosmall	(CALC_MET + 440)
#define IPR_toosmall_TITLE	gettxt("RTPM:358", "ip_toosmall/s")
#define IPR_toosmall_IDX	METSTART + 155

#define IP_badhlen	(CALC_MET + 450)
#define IP_badhlen_TITLE	gettxt("RTPM:359", "ip_badhlen")
#define IP_badhlen_IDX	METSTART + 156

#define IPR_badhlen	(CALC_MET + 460)
#define IPR_badhlen_TITLE	gettxt("RTPM:360", "ip_badhlen/s")
#define IPR_badhlen_IDX	METSTART + 157

#define IP_badlen	(CALC_MET + 470)
#define IP_badlen_TITLE	gettxt("RTPM:361", "ip_badlen")
#define IP_badlen_IDX	METSTART + 158

#define IPR_badlen	(CALC_MET + 480)
#define IPR_badlen_TITLE	gettxt("RTPM:362", "ip_badlen/s")
#define IPR_badlen_IDX	METSTART + 159

#define IP_unknownproto	(CALC_MET + 490)
#define IP_unknownproto_TITLE	gettxt("RTPM:363", "ip_unknownproto")
#define IP_unknownproto_IDX	METSTART + 160

#define IPR_unknownproto	(CALC_MET + 500)
#define IPR_unknownproto_TITLE	gettxt("RTPM:364", "ip_unknownproto/s")
#define IPR_unknownproto_IDX	METSTART + 161

#define IP_fragments	(CALC_MET + 510)
#define IP_fragments_TITLE	gettxt("RTPM:365", "ip_fragments")
#define IP_fragments_IDX	METSTART + 162

#define IPR_fragments	(CALC_MET + 520)
#define IPR_fragments_TITLE	gettxt("RTPM:366", "ip_fragments/s")
#define IPR_fragments_IDX	METSTART + 163

#define IP_fragdropped	(CALC_MET + 530)
#define IP_fragdropped_TITLE	gettxt("RTPM:367", "ip_fragdropped")
#define IP_fragdropped_IDX	METSTART + 164

#define IPR_fragdropped	(CALC_MET + 540)
#define IPR_fragdropped_TITLE	gettxt("RTPM:368", "ip_fragdropped/s")
#define IPR_fragdropped_IDX	METSTART + 165

#define IP_fragtimeout	(CALC_MET + 550)
#define IP_fragtimeout_TITLE	gettxt("RTPM:369", "ip_fragtimeout")
#define IP_fragtimeout_IDX	METSTART + 166

#define IPR_fragtimeout	(CALC_MET + 560)
#define IPR_fragtimeout_TITLE	gettxt("RTPM:370", "ip_fragtimeout/s")
#define IPR_fragtimeout_IDX	METSTART + 167

#define IP_reasms	(CALC_MET + 570)
#define IP_reasms_TITLE	gettxt("RTPM:371", "ip_reasms")
#define IP_reasms_IDX	METSTART + 168

#define IPR_reasms	(CALC_MET + 580)
#define IPR_reasms_TITLE	gettxt("RTPM:372", "ip_reasms/s")
#define IPR_reasms_IDX	METSTART + 169

#define IP_forward	(CALC_MET + 590)
#define IP_forward_TITLE	gettxt("RTPM:373", "ip_forward")
#define IP_forward_IDX	METSTART + 170

#define IPR_forward	(CALC_MET + 600)
#define IPR_forward_TITLE	gettxt("RTPM:374", "ip_forward/s")
#define IPR_forward_IDX	METSTART + 171

#define IP_cantforward	(CALC_MET + 610)
#define IP_cantforward_TITLE	gettxt("RTPM:375", "ip_cantforward")
#define IP_cantforward_IDX	METSTART + 172

#define IPR_cantforward	(CALC_MET + 620)
#define IPR_cantforward_TITLE	gettxt("RTPM:376", "ip_cantforward/s")
#define IPR_cantforward_IDX	METSTART + 173

#define IP_noroutes	(CALC_MET + 630)
#define IP_noroutes_TITLE	gettxt("RTPM:377", "ip_noroutes")
#define IP_noroutes_IDX	METSTART + 174

#define IPR_noroutes	(CALC_MET + 640)
#define IPR_noroutes_TITLE	gettxt("RTPM:378", "ip_noroutes/s")
#define IPR_noroutes_IDX	METSTART + 175

#define IP_redirectsent	(CALC_MET + 650)
#define IP_redirectsent_TITLE	gettxt("RTPM:379", "ip_redirectsent")
#define IP_redirectsent_IDX	METSTART + 176

#define IPR_redirectsent	(CALC_MET + 660)
#define IPR_redirectsent_TITLE	gettxt("RTPM:380", "ip_redirectsent/s")
#define IPR_redirectsent_IDX	METSTART + 177

#define IP_inerrors	(CALC_MET + 670)
#define IP_inerrors_TITLE	gettxt("RTPM:381", "ip_inerrors")
#define IP_inerrors_IDX	METSTART + 178

#define IPR_inerrors	(CALC_MET + 680)
#define IPR_inerrors_TITLE	gettxt("RTPM:382", "ip_inerrors/s")
#define IPR_inerrors_IDX	METSTART + 179

#define IPR_indelivers	(CALC_MET + 700)
#define IPR_indelivers_TITLE	gettxt("RTPM:383", "ip_indelivers/s")
#define IPR_indelivers_IDX	METSTART + 180

#define IPR_outrequests	(CALC_MET + 720)
#define IPR_outrequests_TITLE	gettxt("RTPM:384", "ip_outrequests/s")
#define IPR_outrequests_IDX	METSTART + 181

#define IP_outerrors	(CALC_MET + 730)
#define IP_outerrors_TITLE	gettxt("RTPM:385", "ip_outerrors")
#define IP_outerrors_IDX	METSTART + 182

#define IPR_outerrors	(CALC_MET + 740)
#define IPR_outerrors_TITLE	gettxt("RTPM:386", "ip_outerrors/s")
#define IPR_outerrors_IDX	METSTART + 183

#define IP_pfrags	(CALC_MET + 750)
#define IP_pfrags_TITLE	gettxt("RTPM:387", "ip_pfrags")
#define IP_pfrags_IDX	METSTART + 184

#define IPR_pfrags	(CALC_MET + 760)
#define IPR_pfrags_TITLE	gettxt("RTPM:388", "ip_pfrags/s")
#define IPR_pfrags_IDX	METSTART + 185

#define IP_fragfails	(CALC_MET + 770)
#define IP_fragfails_TITLE	gettxt("RTPM:389", "ip_fragfails")
#define IP_fragfails_IDX	METSTART + 186

#define IPR_fragfails	(CALC_MET + 780)
#define IPR_fragfails_TITLE	gettxt("RTPM:390", "ip_fragfails/s")
#define IPR_fragfails_IDX	METSTART + 187

#define IP_frags	(CALC_MET + 790)
#define IP_frags_TITLE	gettxt("RTPM:391", "ip_frags")
#define IP_frags_IDX	METSTART + 188

#define IPR_frags	(CALC_MET + 800)
#define IPR_frags_TITLE	gettxt("RTPM:392", "ip_frags/s")
#define IPR_frags_IDX	METSTART + 189

#define ICMP_error	(CALC_MET + 810)
#define ICMP_error_TITLE	gettxt("RTPM:393", "icmp_error")
#define ICMP_error_IDX	METSTART + 190

#define ICMPR_error	(CALC_MET + 820)
#define ICMPR_error_TITLE	gettxt("RTPM:394", "icmp_error/s")
#define ICMPR_error_IDX	METSTART + 191

#define ICMP_oldicmp	(CALC_MET + 830)
#define ICMP_oldicmp_TITLE	gettxt("RTPM:395", "icmp_oldicmp")
#define ICMP_oldicmp_IDX	METSTART + 192

#define ICMPR_oldicmp	(CALC_MET + 840)
#define ICMPR_oldicmp_TITLE	gettxt("RTPM:396", "icmp_oldicmp/s")
#define ICMPR_oldicmp_IDX	METSTART + 193

#define ICMP_outhist0	(CALC_MET + 850)
#define ICMP_outhist0_TITLE	gettxt("RTPM:397", "icmp_echo_reply_out")
#define ICMP_outhist0_IDX	METSTART + 194

#define ICMPR_outhist0	(CALC_MET + 860)
#define ICMPR_outhist0_TITLE	gettxt("RTPM:398", "icmp_echo_reply_out/s")
#define ICMPR_outhist0_IDX	METSTART + 195

#define ICMP_outhist3	(CALC_MET + 870)
#define ICMP_outhist3_TITLE	gettxt("RTPM:399", "icmp_dest_unreachable_out")
#define ICMP_outhist3_IDX	METSTART + 196

#define ICMPR_outhist3	(CALC_MET + 880)
#define ICMPR_outhist3_TITLE	gettxt("RTPM:400", "icmp_dest_unreachable_out/s")
#define ICMPR_outhist3_IDX	METSTART + 197

#define ICMP_outhist4	(CALC_MET + 890)
#define ICMP_outhist4_TITLE	gettxt("RTPM:401", "icmp_source_quench_out")
#define ICMP_outhist4_IDX	METSTART + 198

#define ICMPR_outhist4	(CALC_MET + 900)
#define ICMPR_outhist4_TITLE	gettxt("RTPM:402", "icmp_source_quench_out/s")
#define ICMPR_outhist4_IDX	METSTART + 199

#define ICMP_outhist5	(CALC_MET + 910)
#define ICMP_outhist5_TITLE	gettxt("RTPM:403", "icmp_routing_redirects_out")
#define ICMP_outhist5_IDX	METSTART + 200

#define ICMPR_outhist5	(CALC_MET + 920)
#define ICMPR_outhist5_TITLE	gettxt("RTPM:404", "icmp_routing_redirect_out/s")
#define ICMPR_outhist5_IDX	METSTART + 201

#define ICMP_outhist8	(CALC_MET + 930)
#define ICMP_outhist8_TITLE	gettxt("RTPM:405", "icmp_echo_out")
#define ICMP_outhist8_IDX	METSTART + 202

#define ICMPR_outhist8	(CALC_MET + 940)
#define ICMPR_outhist8_TITLE	gettxt("RTPM:406", "icmp_echo_out/s")
#define ICMPR_outhist8_IDX	METSTART + 203

#define ICMP_outhist11	(CALC_MET + 950)
#define ICMP_outhist11_TITLE	gettxt("RTPM:407", "icmp_time_exceeded_out")
#define ICMP_outhist11_IDX	METSTART + 204

#define ICMPR_outhist11	(CALC_MET + 960)
#define ICMPR_outhist11_TITLE	gettxt("RTPM:408", "icmp_time_exceeded_out/s")
#define ICMPR_outhist11_IDX	METSTART + 205

#define ICMP_outhist12	(CALC_MET + 970)
#define ICMP_outhist12_TITLE	gettxt("RTPM:409", "icmp_parameter_problems_out")
#define ICMP_outhist12_IDX	METSTART + 206

#define ICMPR_outhist12	(CALC_MET + 980)
#define ICMPR_outhist12_TITLE	gettxt("RTPM:410", "icmp_parameter_problems_out/s")
#define ICMPR_outhist12_IDX	METSTART + 207

#define ICMP_outhist13	(CALC_MET + 990)
#define ICMP_outhist13_TITLE	gettxt("RTPM:411", "icmp_time_stamp_out")
#define ICMP_outhist13_IDX	METSTART + 208

#define ICMPR_outhist13	(CALC_MET + 1000)
#define ICMPR_outhist13_TITLE	gettxt("RTPM:412", "icmp_time_stamp_out/s")
#define ICMPR_outhist13_IDX	METSTART + 209

#define ICMP_outhist14	(CALC_MET + 1010)
#define ICMP_outhist14_TITLE	gettxt("RTPM:413", "icmp_time_stamp_reply_out")
#define ICMP_outhist14_IDX	METSTART + 210

#define ICMPR_outhist14	(CALC_MET + 1020)
#define ICMPR_outhist14_TITLE	gettxt("RTPM:414", "icmp_time_stamp_reply_out/s")
#define ICMPR_outhist14_IDX	METSTART + 211

#define ICMP_outhist15	(CALC_MET + 1030)
#define ICMP_outhist15_TITLE	gettxt("RTPM:415", "icmp_info_request_out")
#define ICMP_outhist15_IDX	METSTART + 212

#define ICMPR_outhist15	(CALC_MET + 1040)
#define ICMPR_outhist15_TITLE	gettxt("RTPM:416", "icmp_info_request_out/s")
#define ICMPR_outhist15_IDX	METSTART + 213

#define ICMP_outhist16	(CALC_MET + 1050)
#define ICMP_outhist16_TITLE	gettxt("RTPM:417", "icmp_info_reply_out")
#define ICMP_outhist16_IDX	METSTART + 214

#define ICMPR_outhist16	(CALC_MET + 1060)
#define ICMPR_outhist16_TITLE	gettxt("RTPM:418", "icmp_info_reply_out/s")
#define ICMPR_outhist16_IDX	METSTART + 215

#define ICMP_outhist17	(CALC_MET + 1070)
#define ICMP_outhist17_TITLE	gettxt("RTPM:419", "icmp_address_mask_request_out")
#define ICMP_outhist17_IDX	METSTART + 216

#define ICMPR_outhist17	(CALC_MET + 1080)
#define ICMPR_outhist17_TITLE	gettxt("RTPM:420", "icmp_address_mask_request_out/s")
#define ICMPR_outhist17_IDX	METSTART + 217

#define ICMP_outhist18	(CALC_MET + 1090)
#define ICMP_outhist18_TITLE	gettxt("RTPM:421", "icmp_address_mask_reply_out")
#define ICMP_outhist18_IDX	METSTART + 218

#define ICMPR_outhist18	(CALC_MET + 1100)
#define ICMPR_outhist18_TITLE	gettxt("RTPM:422", "icmp_address_mask_reply_out/s")
#define ICMPR_outhist18_IDX	METSTART + 219

#define ICMP_badcode	(CALC_MET + 1110)
#define ICMP_badcode_TITLE	gettxt("RTPM:423", "icmp_badcode")
#define ICMP_badcode_IDX	METSTART + 220

#define ICMPR_badcode	(CALC_MET + 1120)
#define ICMPR_badcode_TITLE	gettxt("RTPM:424", "icmp_badcode/s")
#define ICMPR_badcode_IDX	METSTART + 221

#define ICMP_tooshort	(CALC_MET + 1130)
#define ICMP_tooshort_TITLE	gettxt("RTPM:425", "icmp_tooshort")
#define ICMP_tooshort_IDX	METSTART + 222

#define ICMPR_tooshort	(CALC_MET + 1140)
#define ICMPR_tooshort_TITLE	gettxt("RTPM:426", "icmp_tooshort/s")
#define ICMPR_tooshort_IDX	METSTART + 223

#define ICMP_checksum	(CALC_MET + 1150)
#define ICMP_checksum_TITLE	gettxt("RTPM:427", "icmp_checksum")
#define ICMP_checksum_IDX	METSTART + 224

#define ICMPR_checksum	(CALC_MET + 1160)
#define ICMPR_checksum_TITLE	gettxt("RTPM:428", "icmp_checksum/s")
#define ICMPR_checksum_IDX	METSTART + 225

#define ICMP_badlen	(CALC_MET + 1170)
#define ICMP_badlen_TITLE	gettxt("RTPM:429", "icmp_badlen")
#define ICMP_badlen_IDX	METSTART + 226

#define ICMPR_badlen	(CALC_MET + 1180)
#define ICMPR_badlen_TITLE	gettxt("RTPM:430", "icmp_badlen/s")
#define ICMPR_badlen_IDX	METSTART + 227

#define ICMP_inhist0	(CALC_MET + 1190)
#define ICMP_inhist0_TITLE	gettxt("RTPM:431", "icmp_echo_reply_in")
#define ICMP_inhist0_IDX	METSTART + 228

#define ICMPR_inhist0	(CALC_MET + 1200)
#define ICMPR_inhist0_TITLE	gettxt("RTPM:432", "icmp_echo_reply_in/s")
#define ICMPR_inhist0_IDX	METSTART + 229

#define ICMP_inhist3	(CALC_MET + 1210)
#define ICMP_inhist3_TITLE	gettxt("RTPM:433", "icmp_dest_unreachable_in")
#define ICMP_inhist3_IDX	METSTART + 230

#define ICMPR_inhist3	(CALC_MET + 1220)
#define ICMPR_inhist3_TITLE	gettxt("RTPM:434", "icmp_dest_unreachable_in/s")
#define ICMPR_inhist3_IDX	METSTART + 231

#define ICMP_inhist4	(CALC_MET + 1230)
#define ICMP_inhist4_TITLE	gettxt("RTPM:435", "icmp_source_quench_in")
#define ICMP_inhist4_IDX	METSTART + 232

#define ICMPR_inhist4	(CALC_MET + 1240)
#define ICMPR_inhist4_TITLE	gettxt("RTPM:436", "icmp_source_quench_in/s")
#define ICMPR_inhist4_IDX	METSTART + 233

#define ICMP_inhist5	(CALC_MET + 1250)
#define ICMP_inhist5_TITLE	gettxt("RTPM:437", "icmp_routing_redirects_in")
#define ICMP_inhist5_IDX	METSTART + 234

#define ICMPR_inhist5	(CALC_MET + 1260)
#define ICMPR_inhist5_TITLE	gettxt("RTPM:438", "icmp_routing_redirect_in/s")
#define ICMPR_inhist5_IDX	METSTART + 235

#define ICMP_inhist8	(CALC_MET + 1270)
#define ICMP_inhist8_TITLE	gettxt("RTPM:439", "icmp_echo_in")
#define ICMP_inhist8_IDX	METSTART + 236

#define ICMPR_inhist8	(CALC_MET + 1280)
#define ICMPR_inhist8_TITLE	gettxt("RTPM:440", "icmp_echo_in/s")
#define ICMPR_inhist8_IDX	METSTART + 237

#define ICMP_inhist11	(CALC_MET + 1290)
#define ICMP_inhist11_TITLE	gettxt("RTPM:441", "icmp_time_exceeded_in")
#define ICMP_inhist11_IDX	METSTART + 238

#define ICMPR_inhist11	(CALC_MET + 1300)
#define ICMPR_inhist11_TITLE	gettxt("RTPM:442", "icmp_time_exceeded_in/s")
#define ICMPR_inhist11_IDX	METSTART + 239

#define ICMP_inhist12	(CALC_MET + 1310)
#define ICMP_inhist12_TITLE	gettxt("RTPM:443", "icmp_parameter_problems_in")
#define ICMP_inhist12_IDX	METSTART + 240

#define ICMPR_inhist12	(CALC_MET + 1320)
#define ICMPR_inhist12_TITLE	gettxt("RTPM:444", "icmp_parameter_problems_in/s")
#define ICMPR_inhist12_IDX	METSTART + 241

#define ICMP_inhist13	(CALC_MET + 1330)
#define ICMP_inhist13_TITLE	gettxt("RTPM:445", "icmp_time_stamp_in")
#define ICMP_inhist13_IDX	METSTART + 242

#define ICMPR_inhist13	(CALC_MET + 1340)
#define ICMPR_inhist13_TITLE	gettxt("RTPM:446", "icmp_time_stamp_in/s")
#define ICMPR_inhist13_IDX	METSTART + 243

#define ICMP_inhist14	(CALC_MET + 1350)
#define ICMP_inhist14_TITLE	gettxt("RTPM:447", "icmp_time_stamp_reply_in")
#define ICMP_inhist14_IDX	METSTART + 244

#define ICMPR_inhist14	(CALC_MET + 1360)
#define ICMPR_inhist14_TITLE	gettxt("RTPM:448", "icmp_time_stamp_reply_in/s")
#define ICMPR_inhist14_IDX	METSTART + 245

#define ICMP_inhist15	(CALC_MET + 1370)
#define ICMP_inhist15_TITLE	gettxt("RTPM:449", "icmp_info_request_in")
#define ICMP_inhist15_IDX	METSTART + 246

#define ICMPR_inhist15	(CALC_MET + 1380)
#define ICMPR_inhist15_TITLE	gettxt("RTPM:450", "icmp_info_request_in/s")
#define ICMPR_inhist15_IDX	METSTART + 247

#define ICMP_inhist16	(CALC_MET + 1390)
#define ICMP_inhist16_TITLE	gettxt("RTPM:451", "icmp_info_reply_in")
#define ICMP_inhist16_IDX	METSTART + 248

#define ICMPR_inhist16	(CALC_MET + 1400)
#define ICMPR_inhist16_TITLE	gettxt("RTPM:452", "icmp_info_reply_in/s")
#define ICMPR_inhist16_IDX	METSTART + 249

#define ICMP_inhist17	(CALC_MET + 1410)
#define ICMP_inhist17_TITLE	gettxt("RTPM:453", "icmp_address_mask_request_in")
#define ICMP_inhist17_IDX	METSTART + 250

#define ICMPR_inhist17	(CALC_MET + 1420)
#define ICMPR_inhist17_TITLE	gettxt("RTPM:454", "icmp_address_mask_request_in/s")
#define ICMPR_inhist17_IDX	METSTART + 251

#define ICMP_inhist18	(CALC_MET + 1430)
#define ICMP_inhist18_TITLE	gettxt("RTPM:455", "icmp_address_mask_reply_in")
#define ICMP_inhist18_IDX	METSTART + 252

#define ICMPR_inhist18	(CALC_MET + 1440)
#define ICMPR_inhist18_TITLE	gettxt("RTPM:456", "icmp_address_mask_reply_in/s")
#define ICMPR_inhist18_IDX	METSTART + 253

#define ICMP_reflect	(CALC_MET + 1450)
#define ICMP_reflect_TITLE	gettxt("RTPM:457", "icmp_reflect")
#define ICMP_reflect_IDX	METSTART + 254

#define ICMPR_reflect	(CALC_MET + 1460)
#define ICMPR_reflect_TITLE	gettxt("RTPM:458", "icmp_reflect/s")
#define ICMPR_reflect_IDX	METSTART + 255

#define ICMPR_intotal	(CALC_MET + 1480)
#define ICMPR_intotal_TITLE	gettxt("RTPM:459", "icmp_intotal/s")
#define ICMPR_intotal_IDX	METSTART + 256

#define ICMPR_outtotal	(CALC_MET + 1500)
#define ICMPR_outtotal_TITLE	gettxt("RTPM:460", "icmp_outtotal/s")
#define ICMPR_outtotal_IDX	METSTART + 257

#define ICMP_outerrors	(CALC_MET + 1510)
#define ICMP_outerrors_TITLE	gettxt("RTPM:461", "icmp_outerrors")
#define ICMP_outerrors_IDX	METSTART + 258

#define ICMPR_outerrors	(CALC_MET + 1520)
#define ICMPR_outerrors_TITLE	gettxt("RTPM:462", "icmp_outerrors/s")
#define ICMPR_outerrors_IDX	METSTART + 259

#define TCPR_sndtotal	(CALC_MET + 1540)
#define TCPR_sndtotal_TITLE	gettxt("RTPM:463", "tcp_sndtotal/s")
#define TCPR_sndtotal_IDX	METSTART + 260

#define TCPR_sndpack	(CALC_MET + 1560)
#define TCPR_sndpack_TITLE	gettxt("RTPM:464", "tcp_sndpack/s")
#define TCPR_sndpack_IDX	METSTART + 261

#define TCPR_sndbyte	(CALC_MET + 1580)
#define TCPR_sndbyte_TITLE	gettxt("RTPM:465", "tcp_sndbyte/s")
#define TCPR_sndbyte_IDX	METSTART + 262

#define TCP_sndrexmitpack	(CALC_MET + 1590)
#define TCP_sndrexmitpack_TITLE	gettxt("RTPM:466", "tcp_sndrexmitpack")
#define TCP_sndrexmitpack_IDX	METSTART + 263

#define TCPR_sndrexmitpack	(CALC_MET + 1600)
#define TCPR_sndrexmitpack_TITLE	gettxt("RTPM:467", "tcp_sndrexmitpack/s")
#define TCPR_sndrexmitpack_IDX	METSTART + 264

#define TCP_sndrexmitbyte	(CALC_MET + 1610)
#define TCP_sndrexmitbyte_TITLE	gettxt("RTPM:468", "tcp_sndrexmitbyte")
#define TCP_sndrexmitbyte_IDX	METSTART + 265

#define TCPR_sndrexmitbyte	(CALC_MET + 1620)
#define TCPR_sndrexmitbyte_TITLE	gettxt("RTPM:469", "tcp_sndrexmitbyte/s")
#define TCPR_sndrexmitbyte_IDX	METSTART + 266

#define TCP_sndacks	(CALC_MET + 1630)
#define TCP_sndacks_TITLE	gettxt("RTPM:470", "tcp_sndacks")
#define TCP_sndacks_IDX	METSTART + 267

#define TCPR_sndacks	(CALC_MET + 1640)
#define TCPR_sndacks_TITLE	gettxt("RTPM:471", "tcp_sndacks/s")
#define TCPR_sndacks_IDX	METSTART + 268

#define TCP_delack	(CALC_MET + 1650)
#define TCP_delack_TITLE	gettxt("RTPM:472", "tcp_delack")
#define TCP_delack_IDX	METSTART + 269

#define TCPR_delack	(CALC_MET + 1660)
#define TCPR_delack_TITLE	gettxt("RTPM:473", "tcp_delack/s")
#define TCPR_delack_IDX	METSTART + 270

#define TCP_sndurg	(CALC_MET + 1670)
#define TCP_sndurg_TITLE	gettxt("RTPM:474", "tcp_sndurg")
#define TCP_sndurg_IDX	METSTART + 271

#define TCPR_sndurg	(CALC_MET + 1680)
#define TCPR_sndurg_TITLE	gettxt("RTPM:475", "tcp_sndurg/s")
#define TCPR_sndurg_IDX	METSTART + 272

#define TCP_sndprobe	(CALC_MET + 1690)
#define TCP_sndprobe_TITLE	gettxt("RTPM:476", "tcp_sndprobe")
#define TCP_sndprobe_IDX	METSTART + 273

#define TCPR_sndprobe	(CALC_MET + 1700)
#define TCPR_sndprobe_TITLE	gettxt("RTPM:477", "tcp_sndprobe/s")
#define TCPR_sndprobe_IDX	METSTART + 274

#define TCP_sndwinup	(CALC_MET + 1710)
#define TCP_sndwinup_TITLE	gettxt("RTPM:478", "tcp_sndwinup")
#define TCP_sndwinup_IDX	METSTART + 275

#define TCPR_sndwinup	(CALC_MET + 1720)
#define TCPR_sndwinup_TITLE	gettxt("RTPM:479", "tcp_sndwinup/s")
#define TCPR_sndwinup_IDX	METSTART + 276

#define TCP_sndctrl	(CALC_MET + 1730)
#define TCP_sndctrl_TITLE	gettxt("RTPM:480", "tcp_sndctrl")
#define TCP_sndctrl_IDX	METSTART + 277

#define TCPR_sndctrl	(CALC_MET + 1740)
#define TCPR_sndctrl_TITLE	gettxt("RTPM:481", "tcp_sndctrl/s")
#define TCPR_sndctrl_IDX	METSTART + 278

#define TCP_sndrsts	(CALC_MET + 1750)
#define TCP_sndrsts_TITLE	gettxt("RTPM:482", "tcp_sndrsts")
#define TCP_sndrsts_IDX	METSTART + 279

#define TCPR_sndrsts	(CALC_MET + 1760)
#define TCPR_sndrsts_TITLE	gettxt("RTPM:483", "tcp_sndrsts/s")
#define TCPR_sndrsts_IDX	METSTART + 280

#define TCP_rcvtotal	(CALC_MET + 1770)
#define TCP_rcvtotal_TITLE	gettxt("RTPM:484", "tcp_rcvtotal")
#define TCP_rcvtotal_IDX	METSTART + 281

#define TCPR_rcvtotal	(CALC_MET + 1780)
#define TCPR_rcvtotal_TITLE	gettxt("RTPM:485", "tcp_rcvtotal/s")
#define TCPR_rcvtotal_IDX	METSTART + 282

#define TCP_rcvackpack	(CALC_MET + 1790)
#define TCP_rcvackpack_TITLE	gettxt("RTPM:486", "tcp_rcvackpack")
#define TCP_rcvackpack_IDX	METSTART + 283

#define TCPR_rcvackpack	(CALC_MET + 1800)
#define TCPR_rcvackpack_TITLE	gettxt("RTPM:487", "tcp_rcvackpack/s")
#define TCPR_rcvackpack_IDX	METSTART + 284

#define TCP_rcvackbyte	(CALC_MET + 1810)
#define TCP_rcvackbyte_TITLE	gettxt("RTPM:488", "tcp_rcvackbyte")
#define TCP_rcvackbyte_IDX	METSTART + 285

#define TCPR_rcvackbyte	(CALC_MET + 1820)
#define TCPR_rcvackbyte_TITLE	gettxt("RTPM:489", "tcp_rcvackbyte/s")
#define TCPR_rcvackbyte_IDX	METSTART + 286

#define TCP_rcvdupack	(CALC_MET + 1830)
#define TCP_rcvdupack_TITLE	gettxt("RTPM:490", "tcp_rcvdupack")
#define TCP_rcvdupack_IDX	METSTART + 287

#define TCPR_rcvdupack	(CALC_MET + 1840)
#define TCPR_rcvdupack_TITLE	gettxt("RTPM:491", "tcp_rcvdupack/s")
#define TCPR_rcvdupack_IDX	METSTART + 288

#define TCP_rcvacktoomuch	(CALC_MET + 1850)
#define TCP_rcvacktoomuch_TITLE	gettxt("RTPM:492", "tcp_rcvacktoomuch")
#define TCP_rcvacktoomuch_IDX	METSTART + 289

#define TCPR_rcvacktoomuch	(CALC_MET + 1860)
#define TCPR_rcvacktoomuch_TITLE	gettxt("RTPM:493", "tcp_rcvacktoomuch/s")
#define TCPR_rcvacktoomuch_IDX	METSTART + 290

#define TCP_rcvpack	(CALC_MET + 1870)
#define TCP_rcvpack_TITLE	gettxt("RTPM:494", "tcp_rcvpack")
#define TCP_rcvpack_IDX	METSTART + 291

#define TCPR_rcvpack	(CALC_MET + 1880)
#define TCPR_rcvpack_TITLE	gettxt("RTPM:495", "tcp_rcvpack/s")
#define TCPR_rcvpack_IDX	METSTART + 292

#define TCP_rcvbyte	(CALC_MET + 1890)
#define TCP_rcvbyte_TITLE	gettxt("RTPM:496", "tcp_rcvbyte")
#define TCP_rcvbyte_IDX	METSTART + 293

#define TCPR_rcvbyte	(CALC_MET + 1900)
#define TCPR_rcvbyte_TITLE	gettxt("RTPM:497", "tcp_rcvbyte/s")
#define TCPR_rcvbyte_IDX	METSTART + 294

#define TCP_rcvduppack	(CALC_MET + 1910)
#define TCP_rcvduppack_TITLE	gettxt("RTPM:498", "tcp_rcvduppack")
#define TCP_rcvduppack_IDX	METSTART + 295

#define TCPR_rcvduppack	(CALC_MET + 1920)
#define TCPR_rcvduppack_TITLE	gettxt("RTPM:499", "tcp_rcvduppack/s")
#define TCPR_rcvduppack_IDX	METSTART + 296

#define TCP_rcvdupbyte	(CALC_MET + 1930)
#define TCP_rcvdupbyte_TITLE	gettxt("RTPM:500", "tcp_rcvdupbyte")
#define TCP_rcvdupbyte_IDX	METSTART + 297

#define TCPR_rcvdupbyte	(CALC_MET + 1940)
#define TCPR_rcvdupbyte_TITLE	gettxt("RTPM:501", "tcp_rcvdupbyte/s")
#define TCPR_rcvdupbyte_IDX	METSTART + 298

#define TCP_rcvpartduppack	(CALC_MET + 1950)
#define TCP_rcvpartduppack_TITLE	gettxt("RTPM:502", "tcp_rcvpartduppack")
#define TCP_rcvpartduppack_IDX	METSTART + 299

#define TCPR_rcvpartduppack	(CALC_MET + 1960)
#define TCPR_rcvpartduppack_TITLE	gettxt("RTPM:503", "tcp_rcvpartduppack/s")
#define TCPR_rcvpartduppack_IDX	METSTART + 300

#define TCP_rcvpartdupbyte	(CALC_MET + 1970)
#define TCP_rcvpartdupbyte_TITLE	gettxt("RTPM:504", "tcp_rcvpartdupbyte")
#define TCP_rcvpartdupbyte_IDX	METSTART + 301

#define TCPR_rcvpartdupbyte	(CALC_MET + 1980)
#define TCPR_rcvpartdupbyte_TITLE	gettxt("RTPM:505", "tcp_rcvpartdupbyte/s")
#define TCPR_rcvpartdupbyte_IDX	METSTART + 302

#define TCP_rcvoopack	(CALC_MET + 1990)
#define TCP_rcvoopack_TITLE	gettxt("RTPM:506", "tcp_rcvoopack")
#define TCP_rcvoopack_IDX	METSTART + 303

#define TCPR_rcvoopack	(CALC_MET + 2000)
#define TCPR_rcvoopack_TITLE	gettxt("RTPM:507", "tcp_rcvoopack/s")
#define TCPR_rcvoopack_IDX	METSTART + 304

#define TCP_rcvoobyte	(CALC_MET + 2010)
#define TCP_rcvoobyte_TITLE	gettxt("RTPM:508", "tcp_rcvoobyte")
#define TCP_rcvoobyte_IDX	METSTART + 305

#define TCPR_rcvoobyte	(CALC_MET + 2020)
#define TCPR_rcvoobyte_TITLE	gettxt("RTPM:509", "tcp_rcvoobyte/s")
#define TCPR_rcvoobyte_IDX	METSTART + 306

#define TCP_rcvpackafterwin	(CALC_MET + 2030)
#define TCP_rcvpackafterwin_TITLE	gettxt("RTPM:510", "tcp_rcvpackafterwin")
#define TCP_rcvpackafterwin_IDX	METSTART + 307

#define TCPR_rcvpackafterwin	(CALC_MET + 2040)
#define TCPR_rcvpackafterwin_TITLE	gettxt("RTPM:511", "tcp_rcvpackafterwin/s")
#define TCPR_rcvpackafterwin_IDX	METSTART + 308

#define TCP_rcvbyteafterwin	(CALC_MET + 2050)
#define TCP_rcvbyteafterwin_TITLE	gettxt("RTPM:512", "tcp_rcvbyteafterwin")
#define TCP_rcvbyteafterwin_IDX	METSTART + 309

#define TCPR_rcvbyteafterwin	(CALC_MET + 2060)
#define TCPR_rcvbyteafterwin_TITLE	gettxt("RTPM:513", "tcp_rcvbyteafterwin/s")
#define TCPR_rcvbyteafterwin_IDX	METSTART + 310

#define TCP_rcvwinprobe	(CALC_MET + 2070)
#define TCP_rcvwinprobe_TITLE	gettxt("RTPM:514", "tcp_rcvwinprobe")
#define TCP_rcvwinprobe_IDX	METSTART + 311

#define TCPR_rcvwinprobe	(CALC_MET + 2080)
#define TCPR_rcvwinprobe_TITLE	gettxt("RTPM:515", "tcp_rcvwinprobe/s")
#define TCPR_rcvwinprobe_IDX	METSTART + 312

#define TCP_rcvwinupd	(CALC_MET + 2090)
#define TCP_rcvwinupd_TITLE	gettxt("RTPM:516", "tcp_rcvwinupd")
#define TCP_rcvwinupd_IDX	METSTART + 313

#define TCPR_rcvwinupd	(CALC_MET + 2100)
#define TCPR_rcvwinupd_TITLE	gettxt("RTPM:517", "tcp_rcvwinupd/s")
#define TCPR_rcvwinupd_IDX	METSTART + 314

#define TCP_rcvafterclose	(CALC_MET + 2110)
#define TCP_rcvafterclose_TITLE	gettxt("RTPM:518", "tcp_rcvafterclose")
#define TCP_rcvafterclose_IDX	METSTART + 315

#define TCPR_rcvafterclose	(CALC_MET + 2120)
#define TCPR_rcvafterclose_TITLE	gettxt("RTPM:519", "tcp_rcvafterclose/s")
#define TCPR_rcvafterclose_IDX	METSTART + 316

#define TCP_rcvbadsum	(CALC_MET + 2130)
#define TCP_rcvbadsum_TITLE	gettxt("RTPM:520", "tcp_rcvbadsum")
#define TCP_rcvbadsum_IDX	METSTART + 317

#define TCPR_rcvbadsum	(CALC_MET + 2140)
#define TCPR_rcvbadsum_TITLE	gettxt("RTPM:521", "tcp_rcvbadsum/s")
#define TCPR_rcvbadsum_IDX	METSTART + 318

#define TCP_rcvbadoff	(CALC_MET + 2150)
#define TCP_rcvbadoff_TITLE	gettxt("RTPM:522", "tcp_rcvbadoff")
#define TCP_rcvbadoff_IDX	METSTART + 319

#define TCPR_rcvbadoff	(CALC_MET + 2160)
#define TCPR_rcvbadoff_TITLE	gettxt("RTPM:523", "tcp_rcvbadoff/s")
#define TCPR_rcvbadoff_IDX	METSTART + 320

#define TCP_rcvshort	(CALC_MET + 2170)
#define TCP_rcvshort_TITLE	gettxt("RTPM:524", "tcp_rcvshort")
#define TCP_rcvshort_IDX	METSTART + 321

#define TCPR_rcvshort	(CALC_MET + 2180)
#define TCPR_rcvshort_TITLE	gettxt("RTPM:525", "tcp_rcvshort/s")
#define TCPR_rcvshort_IDX	METSTART + 322

#define TCP_inerrors	(CALC_MET + 2190)
#define TCP_inerrors_TITLE	gettxt("RTPM:526", "tcp_inerrors")
#define TCP_inerrors_IDX	METSTART + 323

#define TCPR_inerrors	(CALC_MET + 2200)
#define TCPR_inerrors_TITLE	gettxt("RTPM:527", "tcp_inerrors/s")
#define TCPR_inerrors_IDX	METSTART + 324

#define TCP_connattempt	(CALC_MET + 2210)
#define TCP_connattempt_TITLE	gettxt("RTPM:528", "tcp_connattempt")
#define TCP_connattempt_IDX	METSTART + 325

#define TCPR_connattempt	(CALC_MET + 2220)
#define TCPR_connattempt_TITLE	gettxt("RTPM:529", "tcp_connattempt/s")
#define TCPR_connattempt_IDX	METSTART + 326

#define TCP_accepts	(CALC_MET + 2230)
#define TCP_accepts_TITLE	gettxt("RTPM:530", "tcp_accepts")
#define TCP_accepts_IDX	METSTART + 327

#define TCPR_accepts	(CALC_MET + 2240)
#define TCPR_accepts_TITLE	gettxt("RTPM:531", "tcp_accepts/s")
#define TCPR_accepts_IDX	METSTART + 328

#define TCP_connects	(CALC_MET + 2250)
#define TCP_connects_TITLE	gettxt("RTPM:532", "tcp_connects")
#define TCP_connects_IDX	METSTART + 329

#define TCPR_connects	(CALC_MET + 2260)
#define TCPR_connects_TITLE	gettxt("RTPM:533", "tcp_connects/s")
#define TCPR_connects_IDX	METSTART + 330

#define TCP_closed	(CALC_MET + 2270)
#define TCP_closed_TITLE	gettxt("RTPM:534", "tcp_closed")
#define TCP_closed_IDX	METSTART + 331

#define TCPR_closed	(CALC_MET + 2280)
#define TCPR_closed_TITLE	gettxt("RTPM:535", "tcp_closed/s")
#define TCPR_closed_IDX	METSTART + 332

#define TCP_drops	(CALC_MET + 2290)
#define TCP_drops_TITLE	gettxt("RTPM:536", "tcp_drops")
#define TCP_drops_IDX	METSTART + 333

#define TCPR_drops	(CALC_MET + 2300)
#define TCPR_drops_TITLE	gettxt("RTPM:537", "tcp_drops/s")
#define TCPR_drops_IDX	METSTART + 334

#define TCP_conndrops	(CALC_MET + 2310)
#define TCP_conndrops_TITLE	gettxt("RTPM:538", "tcp_conndrops")
#define TCP_conndrops_IDX	METSTART + 335

#define TCPR_conndrops	(CALC_MET + 2320)
#define TCPR_conndrops_TITLE	gettxt("RTPM:539", "tcp_conndrops/s")
#define TCPR_conndrops_IDX	METSTART + 336

#define TCP_attemptfails	(CALC_MET + 2330)
#define TCP_attemptfails_TITLE	gettxt("RTPM:540", "tcp_attemptfails")
#define TCP_attemptfails_IDX	METSTART + 337

#define TCPR_attemptfails	(CALC_MET + 2340)
#define TCPR_attemptfails_TITLE	gettxt("RTPM:541", "tcp_attemptfails/s")
#define TCPR_attemptfails_IDX	METSTART + 338

#define TCP_estabresets	(CALC_MET + 2350)
#define TCP_estabresets_TITLE	gettxt("RTPM:542", "tcp_estabresets")
#define TCP_estabresets_IDX	METSTART + 339

#define TCPR_estabresets	(CALC_MET + 2360)
#define TCPR_estabresets_TITLE	gettxt("RTPM:543", "tcp_estabresets/s")
#define TCPR_estabresets_IDX	METSTART + 340

#define TCP_rttupdated	(CALC_MET + 2370)
#define TCP_rttupdated_TITLE	gettxt("RTPM:544", "tcp_rttupdated")
#define TCP_rttupdated_IDX	METSTART + 341

#define TCPR_rttupdated	(CALC_MET + 2380)
#define TCPR_rttupdated_TITLE	gettxt("RTPM:545", "tcp_rttupdated/s")
#define TCPR_rttupdated_IDX	METSTART + 342

#define TCP_segstimed	(CALC_MET + 2390)
#define TCP_segstimed_TITLE	gettxt("RTPM:546", "tcp_segstimed")
#define TCP_segstimed_IDX	METSTART + 343

#define TCPR_segstimed	(CALC_MET + 2400)
#define TCPR_segstimed_TITLE	gettxt("RTPM:547", "tcp_segstimed/s")
#define TCPR_segstimed_IDX	METSTART + 344

#define TCP_rexmttimeo	(CALC_MET + 2410)
#define TCP_rexmttimeo_TITLE	gettxt("RTPM:548", "tcp_rexmttimeo")
#define TCP_rexmttimeo_IDX	METSTART + 345

#define TCPR_rexmttimeo	(CALC_MET + 2420)
#define TCPR_rexmttimeo_TITLE	gettxt("RTPM:549", "tcp_rexmttimeo/s")
#define TCPR_rexmttimeo_IDX	METSTART + 346

#define TCP_timeoutdrop	(CALC_MET + 2430)
#define TCP_timeoutdrop_TITLE	gettxt("RTPM:550", "tcp_timeoutdrop")
#define TCP_timeoutdrop_IDX	METSTART + 347

#define TCPR_timeoutdrop	(CALC_MET + 2440)
#define TCPR_timeoutdrop_TITLE	gettxt("RTPM:551", "tcp_timeoutdrop/s")
#define TCPR_timeoutdrop_IDX	METSTART + 348

#define TCP_persisttimeo	(CALC_MET + 2450)
#define TCP_persisttimeo_TITLE	gettxt("RTPM:552", "tcp_persisttimeo")
#define TCP_persisttimeo_IDX	METSTART + 349

#define TCPR_persisttimeo	(CALC_MET + 2460)
#define TCPR_persisttimeo_TITLE	gettxt("RTPM:553", "tcp_persisttimeo/s")
#define TCPR_persisttimeo_IDX	METSTART + 350

#define TCP_keeptimeo	(CALC_MET + 2470)
#define TCP_keeptimeo_TITLE	gettxt("RTPM:554", "tcp_keeptimeo")
#define TCP_keeptimeo_IDX	METSTART + 351

#define TCPR_keeptimeo	(CALC_MET + 2480)
#define TCPR_keeptimeo_TITLE	gettxt("RTPM:555", "tcp_keeptimeo/s")
#define TCPR_keeptimeo_IDX	METSTART + 352

#define TCP_keepprobe	(CALC_MET + 2490)
#define TCP_keepprobe_TITLE	gettxt("RTPM:556", "tcp_keepprobe")
#define TCP_keepprobe_IDX	METSTART + 353

#define TCPR_keepprobe	(CALC_MET + 2500)
#define TCPR_keepprobe_TITLE	gettxt("RTPM:557", "tcp_keepprobe/s")
#define TCPR_keepprobe_IDX	METSTART + 354

#define TCP_keepdrops	(CALC_MET + 2510)
#define TCP_keepdrops_TITLE	gettxt("RTPM:558", "tcp_keepdrops")
#define TCP_keepdrops_IDX	METSTART + 355

#define TCPR_keepdrops	(CALC_MET + 2520)
#define TCPR_keepdrops_TITLE	gettxt("RTPM:559", "tcp_keepdrops/s")
#define TCPR_keepdrops_IDX	METSTART + 356

#define TCP_linger	(CALC_MET + 2530)
#define TCP_linger_TITLE	gettxt("RTPM:560", "tcp_linger")
#define TCP_linger_IDX	METSTART + 357

#define TCPR_linger	(CALC_MET + 2540)
#define TCPR_linger_TITLE	gettxt("RTPM:561", "tcp_linger/s")
#define TCPR_linger_IDX	METSTART + 358

#define TCP_lingerexp	(CALC_MET + 2550)
#define TCP_lingerexp_TITLE	gettxt("RTPM:562", "tcp_lingerexp")
#define TCP_lingerexp_IDX	METSTART + 359

#define TCPR_lingerexp	(CALC_MET + 2560)
#define TCPR_lingerexp_TITLE	gettxt("RTPM:563", "tcp_lingerexp/s")
#define TCPR_lingerexp_IDX	METSTART + 360

#define TCP_lingercan	(CALC_MET + 2570)
#define TCP_lingercan_TITLE	gettxt("RTPM:564", "tcp_lingercan")
#define TCP_lingercan_IDX	METSTART + 361

#define TCPR_lingercan	(CALC_MET + 2580)
#define TCPR_lingercan_TITLE	gettxt("RTPM:565", "tcp_lingercan/s")
#define TCPR_lingercan_IDX	METSTART + 362

#define TCP_lingerabort	(CALC_MET + 2590)
#define TCP_lingerabort_TITLE	gettxt("RTPM:566", "tcp_lingerabort")
#define TCP_lingerabort_IDX	METSTART + 363

#define TCPR_lingerabort	(CALC_MET + 2600)
#define TCPR_lingerabort_TITLE	gettxt("RTPM:567", "tcp_lingerabort/s")
#define TCPR_lingerabort_IDX	METSTART + 364

#define UDP_hdrops	(CALC_MET + 2610)
#define UDP_hdrops_TITLE	gettxt("RTPM:568", "udp_hdrops")
#define UDP_hdrops_IDX	METSTART + 365

#define UDPR_hdrops	(CALC_MET + 2620)
#define UDPR_hdrops_TITLE	gettxt("RTPM:569", "udp_hdrops/s")
#define UDPR_hdrops_IDX	METSTART + 366

#define UDP_badlen	(CALC_MET + 2630)
#define UDP_badlen_TITLE	gettxt("RTPM:570", "udp_badlen")
#define UDP_badlen_IDX	METSTART + 367

#define UDPR_badlen	(CALC_MET + 2640)
#define UDPR_badlen_TITLE	gettxt("RTPM:571", "udp_badlen/s")
#define UDPR_badlen_IDX	METSTART + 368

#define UDP_badsum	(CALC_MET + 2650)
#define UDP_badsum_TITLE	gettxt("RTPM:572", "udp_badsum")
#define UDP_badsum_IDX	METSTART + 369

#define UDPR_badsum	(CALC_MET + 2660)
#define UDPR_badsum_TITLE	gettxt("RTPM:573", "udp_badsum/s")
#define UDPR_badsum_IDX	METSTART + 370

#define UDP_fullsock	(CALC_MET + 2670)
#define UDP_fullsock_TITLE	gettxt("RTPM:574", "udp_fullsock")
#define UDP_fullsock_IDX	METSTART + 371

#define UDPR_fullsock	(CALC_MET + 2680)
#define UDPR_fullsock_TITLE	gettxt("RTPM:575", "udp_fullsock/s")
#define UDPR_fullsock_IDX	METSTART + 372

#define UDP_noports	(CALC_MET + 2690)
#define UDP_noports_TITLE	gettxt("RTPM:576", "udp_noports")
#define UDP_noports_IDX	METSTART + 373

#define UDPR_noports	(CALC_MET + 2700)
#define UDPR_noports_TITLE	gettxt("RTPM:577", "udp_noports/s")
#define UDPR_noports_IDX	METSTART + 374

#define UDPR_indelivers	(CALC_MET + 2720)
#define UDPR_indelivers_TITLE	gettxt("RTPM:578", "udp_indelivers/s")
#define UDPR_indelivers_IDX	METSTART + 375

#define UDP_inerrors	(CALC_MET + 2730)
#define UDP_inerrors_TITLE	gettxt("RTPM:579", "udp_inerrors")
#define UDP_inerrors_IDX	METSTART + 376

#define UDPR_inerrors	(CALC_MET + 2740)
#define UDPR_inerrors_TITLE	gettxt("RTPM:580", "udp_inerrors/s")
#define UDPR_inerrors_IDX	METSTART + 377

#define UDPR_outtotal	(CALC_MET + 2760)
#define UDPR_outtotal_TITLE	gettxt("RTPM:581", "udp_outtotal/s")
#define UDPR_outtotal_IDX	METSTART + 378

#define IPR_sum	(CALC_MET + 2780)
#define IPR_sum_TITLE	gettxt("RTPM:582", "ip_sum/s")
#define IPR_sum_IDX	METSTART + 379

#define ICMPR_sum	(CALC_MET + 2800)
#define ICMPR_sum_TITLE	gettxt("RTPM:583", "icmp_sum/s")
#define ICMPR_sum_IDX	METSTART + 380

#define UDPR_sum	(CALC_MET + 2820)
#define UDPR_sum_TITLE	gettxt("RTPM:584", "udp_sum/s")
#define UDPR_sum_IDX	METSTART + 381

#define TCPR_sum	(CALC_MET + 2840)
#define TCPR_sum_TITLE	gettxt("RTPM:585", "tcp_sum/s")
#define TCPR_sum_IDX	METSTART + 382

#define NETERR_sum	(CALC_MET + 2850)
#define NETERR_sum_TITLE	gettxt("RTPM:586", "neterr_sum")
#define NETERR_sum_IDX	METSTART + 383

#define NETERRR_sum	(CALC_MET + 2860)
#define NETERRR_sum_TITLE	gettxt("RTPM:587", "neterr_sum/s")
#define NETERRR_sum_IDX	METSTART + 384

#define RCACHE_PERCENT	(CALC_MET + 2870)
#define RCACHE_PERCENT_TITLE	gettxt("RTPM:588", "%%rcache")
#define RCACHE_PERCENT_IDX	(METSTART + 385)

#define WCACHE_PERCENT	(CALC_MET + 2880)
#define WCACHE_PERCENT_TITLE	gettxt("RTPM:589", "%%wcache")
#define WCACHE_PERCENT_IDX	(METSTART + 386)

#define MEM_PERCENT	(CALC_MET + 2890)
#define MEM_PERCENT_TITLE	gettxt("RTPM:590", "%%mem")
#define MEM_PERCENT_IDX	(METSTART + 387)

#define MEM_SWAP_PERCENT	(CALC_MET + 2900)
#define MEM_SWAP_PERCENT_TITLE	gettxt("RTPM:591", "%%memswp")
#define MEM_SWAP_PERCENT_IDX	(METSTART + 388)

#define DSK_SWAP_PERCENT	(CALC_MET + 2910)
#define DSK_SWAP_PERCENT_TITLE	gettxt("RTPM:592", "%%dskswp")
#define DSK_SWAP_PERCENT_IDX	(METSTART + 389)

#define DSK_SWAPPG	(CALC_MET + 2920)
#define DSK_SWAPPG_TITLE	gettxt("RTPM:593", "dskswp")
#define DSK_SWAPPG_IDX	(METSTART + 390)

#define DSK_SWAPPGFREE	(CALC_MET + 2930)
#define DSK_SWAPPGFREE_TITLE	gettxt("RTPM:594", "dskfreeswp")
#define DSK_SWAPPGFREE_IDX	(METSTART + 391)

#define MEM_SWAPPG	(CALC_MET+2940)
#define MEM_SWAPPG_TITLE	gettxt("RTPM:595", "memswp")
#define MEM_SWAPPG_IDX	(METSTART + 392)

#define TOTALMEM	(CALC_MET+2950)
#define TOTALMEM_TITLE	gettxt("RTPM:596", "totalmem")
#define TOTALMEM_IDX	(METSTART+393)

#define STR_STREAM_INUSE_TITLE	gettxt("RTPM:597", "streams")
#define STR_STREAM_INUSE_IDX	(METSTART + 394)

#define STR_STREAM_TOTAL_TITLE	gettxt("RTPM:598", "streams/s")
#define STR_STREAM_TOTAL_IDX	(METSTART + 395)

#define STR_QUEUE_INUSE_TITLE	gettxt("RTPM:599", "queues")
#define STR_QUEUE_INUSE_IDX	(METSTART + 396)

#define STR_QUEUE_TOTAL_TITLE	gettxt("RTPM:600", "queues/s")
#define STR_QUEUE_TOTAL_IDX	(METSTART + 397)

#define STR_MDBBLK_INUSE_TITLE	gettxt("RTPM:601", "mdbblks")
#define STR_MDBBLK_INUSE_IDX	(METSTART + 398)

#define STR_MDBBLK_TOTAL_TITLE	gettxt("RTPM:602", "mdbblks/s")
#define STR_MDBBLK_TOTAL_IDX	(METSTART + 399)

#define STR_MSGBLK_INUSE_TITLE	gettxt("RTPM:603", "msgblks")
#define STR_MSGBLK_INUSE_IDX	(METSTART + 400)

#define STR_MSGBLK_TOTAL_TITLE	gettxt("RTPM:604", "msgblks/s")
#define STR_MSGBLK_TOTAL_IDX	(METSTART + 401)


#define STR_LINK_INUSE_TITLE	gettxt("RTPM:605", "links")
#define STR_LINK_INUSE_IDX	(METSTART + 402)

#define STR_LINK_TOTAL_TITLE	gettxt("RTPM:606", "links/s")
#define STR_LINK_TOTAL_IDX	(METSTART + 403)

#define STR_EVENT_INUSE_TITLE	gettxt("RTPM:607", "events")
#define STR_EVENT_INUSE_IDX	(METSTART + 404)

#define STR_EVENT_TOTAL_TITLE	gettxt("RTPM:608", "events/s")
#define STR_EVENT_TOTAL_IDX	(METSTART + 405)

#define STR_EVENT_FAIL_TITLE	gettxt("RTPM:609", "eventfail")
#define STR_EVENT_FAIL_IDX	(METSTART + 406)

#define STR_EVENT_FAILR_TITLE	gettxt("RTPM:610", "eventfail/s")
#define STR_EVENT_FAILR_IDX	(METSTART + 407)

	/* Total Known Servers */
#define SAP_total_servers		(CALC_MET + 2980)
#define SAP_total_servers_TITLE	gettxt("RTPM:671", "sap_total_servers")
#define SAP_total_servers_IDX 		METSTART + 408

	/* Total Unused Server Entries */
#define SAP_unused		(CALC_MET + 2990)
#define SAP_unused_TITLE	gettxt("RTPM:672", "sap_unused")
#define SAP_unused_IDX 		METSTART + 409

	/* Total Lans known to SAP */
#define SAP_Lans		(CALC_MET + 3000)
#define SAP_Lans_TITLE	gettxt("RTPM:673", "sap_Lans")
#define SAP_Lans_IDX 		METSTART + 410

	/* Total Sap Packets Received */
#define SAP_TotalInSaps		(CALC_MET + 3010)
#define SAPR_TotalInSaps_TITLE	gettxt("RTPM:674", "sap_TotalInSaps/s")
#define SAPR_TotalInSaps_IDX 	METSTART + 411

	/* GSQ Packets Received */
#define SAP_GSQReceived		(CALC_MET + 3020)
#define SAPR_GSQReceived_TITLE	gettxt("RTPM:675", "sap_GSQReceived/s")
#define SAPR_GSQReceived_IDX 	METSTART + 412

	/* GSR Packets Received */
#define SAP_GSRReceived		(CALC_MET + 3030)
#define SAPR_GSRReceived_TITLE	gettxt("RTPM:676", "sap_GSRReceived/s")
#define SAPR_GSRReceived_IDX 	METSTART + 413

	/* NSQ Packets Received */
#define SAP_NSQReceived		(CALC_MET + 3040)
#define SAPR_NSQReceived_TITLE	gettxt("RTPM:677", "sap_NSQReceived/s")
#define SAPR_NSQReceived_IDX 	METSTART + 414

	/* Local Requests to Advertise a Server Received */
#define SAP_SASReceived		(CALC_MET + 3050)
#define SAPR_SASReceived_TITLE	gettxt("RTPM:678", "sap_SASReceived/s")
#define SAPR_SASReceived_IDX 	METSTART + 415

	/* Local Requests to Notify of Changes Received */
#define SAP_SNCReceived		(CALC_MET + 3060)
#define SAPR_SNCReceived_TITLE	gettxt("RTPM:679", "sap_SNCReceived/s")
#define SAPR_SNCReceived_IDX 	METSTART + 416

	/* Local Requests to Get Shared Memory Id Received */
#define SAP_GSIReceived		(CALC_MET + 3070)
#define SAPR_GSIReceived_TITLE	gettxt("RTPM:680", "sap_GSIReceived/s")
#define SAPR_GSIReceived_IDX 	METSTART + 417

	/* Packets Received, Source not on our LAN */
#define SAP_NotNeighbor		(CALC_MET + 3080)
#define SAP_NotNeighbor_TITLE	gettxt("RTPM:681", "sap_NotNeighbor")
#define SAP_NotNeighbor_IDX 	METSTART + 418
#define SAPR_NotNeighbor_TITLE	gettxt("RTPM:682", "sap_NotNeighbor/s")
#define SAPR_NotNeighbor_IDX 	METSTART + 419

	/* Packets Received & Dropped, Echo of packet sent by SAPD */
#define SAP_EchoMyOutput		(CALC_MET + 3090)
#define SAP_EchoMyOutput_TITLE	gettxt("RTPM:683", "sap_EchoMyOutput")
#define SAP_EchoMyOutput_IDX 	METSTART + 420
#define SAPR_EchoMyOutput_TITLE	gettxt("RTPM:684", "sap_EchoMyOutput/s")
#define SAPR_EchoMyOutput_IDX 	METSTART + 421

	/* Packets Received, Bad Size Sap Packets */
#define SAP_BadSizeInSaps		(CALC_MET + 3100)
#define SAP_BadSizeInSaps_TITLE	gettxt("RTPM:685", "sap_BadSizeInSaps")
#define SAP_BadSizeInSaps_IDX 	METSTART + 422
#define SAPR_BadSizeInSaps_TITLE	gettxt("RTPM:686", "sap_BadSizeInSaps/s")
#define SAPR_BadSizeInSaps_IDX 	METSTART + 423

	/* Invalid Sap Source detected */
#define SAP_BadSapSource		(CALC_MET + 3110)
#define SAP_BadSapSource_TITLE	gettxt("RTPM:687", "sap_BadSapSource")
#define SAP_BadSapSource_IDX 	METSTART + 424
#define SAPR_BadSapSource_TITLE	gettxt("RTPM:688", "sap_BadSapSource/s")
#define SAPR_BadSapSource_IDX 	METSTART + 425

	/* Total Sap Packets Sent */
#define SAP_TotalOutSaps		(CALC_MET + 3120)
#define SAPR_TotalOutSaps_TITLE	gettxt("RTPM:689", "sap_TotalOutSaps/s")
#define SAPR_TotalOutSaps_IDX 	METSTART + 426

	/* Nearest Server Replies Sent */
#define SAP_NSRSent		(CALC_MET + 3130)
#define SAPR_NSRSent_TITLE	gettxt("RTPM:690", "sap_NSRSent/s")
#define SAPR_NSRSent_IDX 	METSTART + 427

	/* General Server Replies Sent */
#define SAP_GSRSent		(CALC_MET + 3140)
#define SAPR_GSRSent_TITLE	gettxt("RTPM:691", "sap_GSRSent/s")
#define SAPR_GSRSent_IDX 	METSTART + 428

	/* General Server Queries Sent */
#define SAP_GSQSent		(CALC_MET + 3150)
#define SAPR_GSQSent_TITLE	gettxt("RTPM:692", "sap_GSQSent/s")
#define SAPR_GSQSent_IDX 	METSTART + 429

	/* ACK Responses to Advertise a Local Server Sent */
#define SAP_SASAckSent		(CALC_MET + 3160)
#define SAPR_SASAckSent_TITLE	gettxt("RTPM:693", "sap_SASAckSent/s")
#define SAPR_SASAckSent_IDX 	METSTART + 430

	/* NACK Responses to Advertise a Local Server Sent */
#define SAP_SASNackSent		(CALC_MET + 3170)
#define SAP_SASNackSent_TITLE	gettxt("RTPM:694", "sap_SASNackSent")
#define SAP_SASNackSent_IDX 	METSTART + 431
#define SAPR_SASNackSent_TITLE	gettxt("RTPM:695", "sap_SASNackSent/s")
#define SAPR_SASNackSent_IDX 	METSTART + 432

	/* ACK Responses to Notify Local Process of Changes Sent */
#define SAP_SNCAckSent		(CALC_MET + 3180)
#define SAPR_SNCAckSent_TITLE	gettxt("RTPM:696", "sap_SNCAckSent/s")
#define SAPR_SNCAckSent_IDX 	METSTART + 433

	/* NACK Responses to Notify Local Process of Changes Sent */
#define SAP_SNCNackSent		(CALC_MET + 3190)
#define SAP_SNCNackSent_TITLE	gettxt("RTPM:697", "sap_SNCNackSent")
#define SAP_SNCNackSent_IDX 	METSTART + 434
#define SAPR_SNCNackSent_TITLE	gettxt("RTPM:698", "sap_SNCNackSent/s")
#define SAPR_SNCNackSent_IDX 	METSTART + 435

	/* ACK Responses to Get Shared Memory Id Sent */
#define SAP_GSIAckSent		(CALC_MET + 3200)
#define SAPR_GSIAckSent_TITLE	gettxt("RTPM:699", "sap_GSIAckSent/s")
#define SAPR_GSIAckSent_IDX 	METSTART + 436

	/* Packets where destination net not a local net */
#define SAP_BadDestOutSaps		(CALC_MET + 3210)
#define SAP_BadDestOutSaps_TITLE	gettxt("RTPM:700", "sap_BadDestOutSaps")
#define SAP_BadDestOutSaps_IDX 		METSTART + 437
#define SAPR_BadDestOutSaps_TITLE	gettxt("RTPM:701", "sap_BadDestOutSaps/s")
#define SAPR_BadDestOutSaps_IDX 	METSTART + 438

	/* Server structure allocation request failures (shared memory) */
#define SAP_SrvAllocFailed		(CALC_MET + 3220)
#define SAP_SrvAllocFailed_TITLE	gettxt("RTPM:702", "sap_SrvAllocFailed")
#define SAP_SrvAllocFailed_IDX 		METSTART + 439
#define SAPR_SrvAllocFailed_TITLE	gettxt("RTPM:703", "sap_SrvAllocFailed/s")
#define SAPR_SrvAllocFailed_IDX 	METSTART + 440

	/* Source structure allocation request failures (malloc) */
#define SAP_MallocFailed		(CALC_MET + 3230)
#define SAP_MallocFailed_TITLE	gettxt("RTPM:704", "sap_MallocFailed")
#define SAP_MallocFailed_IDX 	METSTART + 441
#define SAPR_MallocFailed_TITLE	gettxt("RTPM:705", "sap_MallocFailed/s")
#define SAPR_MallocFailed_IDX 	METSTART + 442

	/* Total Network Down packets received from RIP */
#define SAP_TotalInRipSaps		(CALC_MET + 3240)
#define SAP_TotalInRipSaps_TITLE	gettxt("RTPM:706", "sap_TotalInRipSaps")
#define SAP_TotalInRipSaps_IDX 	METSTART + 443
#define SAPR_TotalInRipSaps_TITLE	gettxt("RTPM:707", "sap_TotalInRipSaps/s")
#define SAPR_TotalInRipSaps_IDX 	METSTART + 444


	/* Bad packets received from RIP */
#define SAP_BadRipSaps		(CALC_MET + 3250)
#define SAP_BadRipSaps_TITLE	gettxt("RTPM:708", "sap_BadRipSaps")
#define SAP_BadRipSaps_IDX 	METSTART + 445
#define SAPR_BadRipSaps_TITLE	gettxt("RTPM:709", "sap_BadRipSaps/s")
#define SAPR_BadRipSaps_IDX 	METSTART + 446

	/* Services set to DOWN from RIP packets received */
#define SAP_RipServerDown		(CALC_MET + 3260)
#define SAP_RipServerDown_TITLE	gettxt("RTPM:710", "sap_RipServerDown")
#define SAP_RipServerDown_IDX 	METSTART + 447
#define SAPR_RipServerDown_TITLE	gettxt("RTPM:711", "sap_RipServerDown/s")
#define SAPR_RipServerDown_IDX 	METSTART + 448

	/* Local processes requesting notification of changes */
#define SAP_ProcessesToNotify		(CALC_MET + 3270)
#define SAPR_ProcessesToNotify_TITLE	gettxt("RTPM:712", "sap_ProcessesToNotify/s")
#define SAPR_ProcessesToNotify_IDX 	METSTART + 449

	/* Notifications of change sent to local processes */
#define SAP_NotificationsSent		(CALC_MET + 3280)
#define SAPR_NotificationsSent_TITLE	gettxt("RTPM:713", "sap_NotificationsSent/s")
#define SAPR_NotificationsSent_IDX 	METSTART + 450

	/* Network Id (hex) */
#define SAPLAN_Network		(CALC_MET + 3290)
#define SAPLAN_Network_TITLE	gettxt("RTPM:714", "saplan_Network")
#define SAPLAN_Network_IDX 	METSTART + 451

	/* Lan Index */
#define SAPLAN_LanNumber		(CALC_MET + 3300)
#define SAPLAN_LanNumber_TITLE	gettxt("RTPM:715", "saplan_LanNumber")
#define SAPLAN_LanNumber_IDX 	METSTART + 452

	/* Periodic update interval in seconds */
#define SAPLAN_UpdateInterval		(CALC_MET + 3310)
#define SAPLAN_UpdateInterval_TITLE	gettxt("RTPM:716", "saplan_UpdateInterval")
#define SAPLAN_UpdateInterval_IDX 	METSTART + 453

	/* Periodic Intervals before timeout a server */
#define SAPLAN_AgeFactor		(CALC_MET + 3320)
#define SAPLAN_AgeFactor_TITLE	gettxt("RTPM:717", "saplan_AgeFactor")
#define SAPLAN_AgeFactor_IDX 	METSTART + 454

	/* Minimum time in MS between packets */
#define SAPLAN_PacketGap		(CALC_MET + 3330)
#define SAPLAN_PacketGap_TITLE	gettxt("RTPM:718", "saplan_PacketGap")
#define SAPLAN_PacketGap_IDX 	METSTART + 455

	/* Packet Size */
#define SAPLAN_PacketSize		(CALC_MET + 3340)
#define SAPLAN_PacketSize_TITLE	gettxt("RTPM:719", "saplan_PacketSize")
#define SAPLAN_PacketSize_IDX 	METSTART + 456

	/* Packets Sent */
#define SAPLAN_PacketsSent		(CALC_MET + 3350)
#define SAPLANR_PacketsSent_TITLE	gettxt("RTPM:720", "saplan_PacketsSent/s")
#define SAPLANR_PacketsSent_IDX 	METSTART + 457

	/* Packets Received */
#define SAPLAN_PacketsReceived		(CALC_MET + 3360)
#define SAPLANR_PacketsReceived_TITLE	gettxt("RTPM:721", "saplan_PacketsReceived/s")
#define SAPLANR_PacketsReceived_IDX 	METSTART + 458

	/* Bad Packets Received */
#define SAPLAN_BadPktsReceived		(CALC_MET + 3370)
#define SAPLAN_BadPktsReceived_TITLE	gettxt("RTPM:722", "saplan_BadPktsReceived")
#define SAPLAN_BadPktsReceived_IDX 	METSTART + 459
#define SAPLANR_BadPktsReceived_TITLE	gettxt("RTPM:723", "saplan_BadPktsReceived/s")
#define SAPLANR_BadPktsReceived_IDX 	METSTART + 460

	/* Up stream packets DLPI header too small, dropped */
#define IPXLAN_InProtoSize		(CALC_MET + 3380)
#define IPXLAN_InProtoSize_TITLE	gettxt("RTPM:724", "ipxlan_InProtoSize")
#define IPXLAN_InProtoSize_IDX 	METSTART + 461
#define IPXLANR_InProtoSize_TITLE	gettxt("RTPM:725", "ipxlan_InProtoSize/s")
#define IPXLANR_InProtoSize_IDX 	METSTART + 462

	/* Up stream packets, not DLPI data type, dropped */
#define IPXLAN_InBadDLPItype		(CALC_MET + 3390)
#define IPXLAN_InBadDLPItype_TITLE	gettxt("RTPM:726", "ipxlan_InBadDLPItype")
#define IPXLAN_InBadDLPItype_IDX 	METSTART + 463
#define IPXLANR_InBadDLPItype_TITLE	gettxt("RTPM:727", "ipxlan_InBadDLPItype/s")
#define IPXLANR_InBadDLPItype_IDX 	METSTART + 464

	/* Up stream data IPX packets coalesced */
#define IPXLAN_InCoalesced		(CALC_MET + 3400)
#define IPXLAN_InCoalesced_TITLE	gettxt("RTPM:728", "ipxlan_InCoalesced")
#define IPXLAN_InCoalesced_IDX 	METSTART + 465
#define IPXLANR_InCoalesced_TITLE	gettxt("RTPM:729", "ipxlan_InCoalesced/s")
#define IPXLANR_InCoalesced_IDX 	METSTART + 466

	/* Up stream IPX/Propagation packets propagated */
#define IPXLAN_InPropagation		(CALC_MET + 3410)
#define IPXLANR_InPropagation_TITLE	gettxt("RTPM:730", "ipxlan_InPropagation/s")
#define IPXLANR_InPropagation_IDX 	METSTART + 467

	/* Up stream IPX/Propagation packets not propagated */
#define IPXLAN_InNoPropagate		(CALC_MET + 3420)
#define IPXLAN_InNoPropagate_TITLE	gettxt("RTPM:731", "ipxlan_InNoPropagate")
#define IPXLAN_InNoPropagate_IDX 	METSTART + 468
#define IPXLANR_InNoPropagate_TITLE	gettxt("RTPM:732", "ipxlan_InNoPropagate/s")
#define IPXLANR_InNoPropagate_IDX 	METSTART + 469

	/* Total IPX data packets received from the lan(s) */
#define IPXLAN_InTotal		(CALC_MET + 3430)
#define IPXLANR_InTotal_TITLE	gettxt("RTPM:733", "ipxlan_InTotal/s")
#define IPXLANR_InTotal_IDX 	METSTART + 470

	/* Packets, smaller than IPX header size, dropped */
#define IPXLAN_InBadLength		(CALC_MET + 3440)
#define IPXLAN_InBadLength_TITLE	gettxt("RTPM:734", "ipxlan_InBadLength")
#define IPXLAN_InBadLength_IDX 	METSTART + 471
#define IPXLANR_InBadLength_TITLE	gettxt("RTPM:735", "ipxlan_InBadLength/s")
#define IPXLANR_InBadLength_IDX 	METSTART + 472

	/* Broadcast packets echoed back by DLPI driver, dropped */
#define IPXLAN_InDriverEcho		(CALC_MET + 3450)
#define IPXLAN_InDriverEcho_TITLE	gettxt("RTPM:736", "ipxlan_InDriverEcho")
#define IPXLAN_InDriverEcho_IDX 	METSTART + 473
#define IPXLANR_InDriverEcho_TITLE	gettxt("RTPM:737", "ipxlan_InDriverEcho/s")
#define IPXLANR_InDriverEcho_IDX 	METSTART + 474

	/* IPX/RIP packets */
#define IPXLAN_InRip		(CALC_MET + 3460)
#define IPXLANR_InRip_TITLE	gettxt("RTPM:738", "ipxlan_InRip/s")
#define IPXLANR_InRip_IDX 	METSTART + 475

	/* IPX/RIP processed by router and dropped */
#define IPXLAN_InRipDropped		(CALC_MET + 3470)
#define IPXLAN_InRipDropped_TITLE	gettxt("RTPM:739", "ipxlan_InRipDropped")
#define IPXLAN_InRipDropped_IDX 	METSTART + 476
#define IPXLANR_InRipDropped_TITLE	gettxt("RTPM:740", "ipxlan_InRipDropped/s")
#define IPXLANR_InRipDropped_IDX 	METSTART + 477

	/* IPX/RIP processed by router and routed to IPX */
#define IPXLAN_InRipRouted		(CALC_MET + 3480)
#define IPXLANR_InRipRouted_TITLE	gettxt("RTPM:741", "ipxlan_InRipRouted/s")
#define IPXLANR_InRipRouted_IDX 	METSTART + 478

	/* IPX/SAP packets */
#define IPXLAN_InSap		(CALC_MET + 3490)
#define IPXLANR_InSap_TITLE	gettxt("RTPM:742", "ipxlan_InSap/s")
#define IPXLANR_InSap_IDX 	METSTART + 479

	/* IPX/SAP packets invalid, dropped */
#define IPXLAN_InSapBad		(CALC_MET + 3500)
#define IPXLAN_InSapBad_TITLE	gettxt("RTPM:743", "ipxlan_InSapBad")
#define IPXLAN_InSapBad_IDX 	METSTART + 480
#define IPXLANR_InSapBad_TITLE	gettxt("RTPM:744", "ipxlan_InSapBad/s")
#define IPXLANR_InSapBad_IDX 	METSTART + 481

	/* IPX/SAP packets, routed to ipx */
#define IPXLAN_InSapIpx		(CALC_MET + 3510)
#define IPXLANR_InSapIpx_TITLE	gettxt("RTPM:745", "ipxlan_InSapIpx/s")
#define IPXLANR_InSapIpx_IDX 	METSTART + 482

	/* IPX/SAP packets, not under ipx, routed to sapd */
#define IPXLAN_InSapNoIpxToSapd		(CALC_MET + 3520)
#define IPXLANR_InSapNoIpxToSapd_TITLE	gettxt("RTPM:746", "ipxlan_InSapNoIpxToSapd/s")
#define IPXLANR_InSapNoIpxToSapd_IDX 	METSTART + 483

	/* IPX/SAP packets, not under ipx, no sapd, dropped */
#define IPXLAN_InSapNoIpxDrop		(CALC_MET + 3530)
#define IPXLAN_InSapNoIpxDrop_TITLE	gettxt("RTPM:747", "ipxlan_InSapNoIpxDrop")
#define IPXLAN_InSapNoIpxDrop_IDX 	METSTART + 484
#define IPXLANR_InSapNoIpxDrop_TITLE	gettxt("RTPM:748", "ipxlan_InSapNoIpxDrop/s")
#define IPXLANR_InSapNoIpxDrop_IDX 	METSTART + 485

	/* IPX/DIAGNOSTIC packets addressed to my net */
#define IPXLAN_InDiag		(CALC_MET + 3540)
#define IPXLAN_InDiag_TITLE	gettxt("RTPM:749", "ipxlan_InDiag")
#define IPXLAN_InDiag_IDX 	METSTART + 486
#define IPXLANR_InDiag_TITLE	gettxt("RTPM:750", "ipxlan_InDiag/s")
#define IPXLANR_InDiag_IDX 	METSTART + 487

	/* IPX/DIAGNOSTIC packets addressed internal net, routed to ipx */
#define IPXLAN_InDiagInternal		(CALC_MET + 3550)
#define IPXLAN_InDiagInternal_TITLE	gettxt("RTPM:751", "ipxlan_InDiagInternal")
#define IPXLAN_InDiagInternal_IDX 	METSTART + 488
#define IPXLANR_InDiagInternal_TITLE	gettxt("RTPM:752", "ipxlan_InDiagInternal/s")
#define IPXLANR_InDiagInternal_IDX 	METSTART + 489

	/* IPX/DIAGNOSTIC packets addressed to NIC */
#define IPXLAN_InDiagNIC		(CALC_MET + 3560)
#define IPXLAN_InDiagNIC_TITLE	gettxt("RTPM:753", "ipxlan_InDiagNIC")
#define IPXLAN_InDiagNIC_IDX 	METSTART + 490
#define IPXLANR_InDiagNIC_TITLE	gettxt("RTPM:754", "ipxlan_InDiagNIC/s")
#define IPXLANR_InDiagNIC_IDX 	METSTART + 491

	/* IPX/DIAGNOSTIC packets routed to ipx */
#define IPXLAN_InDiagIpx		(CALC_MET + 3570)
#define IPXLAN_InDiagIpx_TITLE	gettxt("RTPM:755", "ipxlan_InDiagIpx")
#define IPXLAN_InDiagIpx_IDX 	METSTART + 492
#define IPXLANR_InDiagIpx_TITLE	gettxt("RTPM:756", "ipxlan_InDiagIpx/s")
#define IPXLANR_InDiagIpx_IDX 	METSTART + 493

	/* IPX/DIAGNOSTIC packets no ipx, lan router responded */
#define IPXLAN_InDiagNoIpx		(CALC_MET + 3580)
#define IPXLAN_InDiagNoIpx_TITLE	gettxt("RTPM:757", "ipxlan_InDiagNoIpx")
#define IPXLAN_InDiagNoIpx_IDX 	METSTART + 494
#define IPXLANR_InDiagNoIpx_TITLE	gettxt("RTPM:758", "ipxlan_InDiagNoIpx/s")
#define IPXLANR_InDiagNoIpx_IDX 	METSTART + 495

	/* Packets, addressed to NIC, not diagnostic, dropped */
#define IPXLAN_InNICDropped		(CALC_MET + 3590)
#define IPXLAN_InNICDropped_TITLE	gettxt("RTPM:759", "ipxlan_InNICDropped")
#define IPXLAN_InNICDropped_IDX 	METSTART + 496
#define IPXLANR_InNICDropped_TITLE	gettxt("RTPM:760", "ipxlan_InNICDropped/s")
#define IPXLANR_InNICDropped_IDX 	METSTART + 497

	/* Broadcast packets */
#define IPXLAN_InBroadcast		(CALC_MET + 3600)
#define IPXLANR_InBroadcast_TITLE	gettxt("RTPM:761", "ipxlan_InBroadcast/s")
#define IPXLANR_InBroadcast_IDX 	METSTART + 498

	/* Broadcast packets addressed to internal net */
#define IPXLAN_InBroadcastInternal		(CALC_MET + 3610)
#define IPXLANR_InBroadcastInternal_TITLE	gettxt("RTPM:762", "ipxlan_InBroadcastInternal/s")
#define IPXLANR_InBroadcastInternal_IDX 	METSTART + 499

	/* Broadcast packets addressed to NIC */
#define IPXLAN_InBroadcastNIC		(CALC_MET + 3620)
#define IPXLANR_InBroadcastNIC_TITLE	gettxt("RTPM:763", "ipxlan_InBroadcastNIC/s")
#define IPXLANR_InBroadcastNIC_IDX 	METSTART + 500

	/* IPX/DIAGNOSTIC broadcast packets addressed to NIC */
#define IPXLAN_InBroadcastDiag		(CALC_MET + 3630)
#define IPXLAN_InBroadcastDiag_TITLE	gettxt("RTPM:764", "ipxlan_InBroadcastDiag")
#define IPXLAN_InBroadcastDiag_IDX 	METSTART + 501
#define IPXLANR_InBroadcastDiag_TITLE	gettxt("RTPM:765", "ipxlan_InBroadcastDiag/s")
#define IPXLANR_InBroadcastDiag_IDX 	METSTART + 502

	/* IPX/DIAGNOSTIC packets, forwarded to lan(s) */
#define IPXLAN_InBroadcastDiagFwd		(CALC_MET + 3640)
#define IPXLAN_InBroadcastDiagFwd_TITLE	gettxt("RTPM:766", "ipxlan_InBroadcastDiagFwd")
#define IPXLAN_InBroadcastDiagFwd_IDX 	METSTART + 503
#define IPXLANR_InBroadcastDiagFwd_TITLE	gettxt("RTPM:767", "ipxlan_InBroadcastDiagFwd/s")
#define IPXLANR_InBroadcastDiagFwd_IDX 	METSTART + 504

	/* IPX/DIAGNOSTIC packets, routed to ipx */
#define IPXLAN_InBroadcastDiagRoute		(CALC_MET + 3650)
#define IPXLAN_InBroadcastDiagRoute_TITLE	gettxt("RTPM:768", "ipxlan_InBroadcastDiagRoute")
#define IPXLAN_InBroadcastDiagRoute_IDX 	METSTART + 505
#define IPXLANR_InBroadcastDiagRoute_TITLE	gettxt("RTPM:769", "ipxlan_InBroadcastDiagRoute/s")
#define IPXLANR_InBroadcastDiagRoute_IDX 	METSTART + 506

	/* IPX/DIAGNOSTIC packets, no ipx, lan router responded */
#define IPXLAN_InBroadcastDiagResp		(CALC_MET + 3660)
#define IPXLAN_InBroadcastDiagResp_TITLE	gettxt("RTPM:770", "ipxlan_InBroadcastDiagResp")
#define IPXLAN_InBroadcastDiagResp_IDX 	METSTART + 507
#define IPXLANR_InBroadcastDiagResp_TITLE	gettxt("RTPM:771", "ipxlan_InBroadcastDiagResp/s")
#define IPXLANR_InBroadcastDiagResp_IDX 	METSTART + 508

	/* Broadcast Packets addressed to NIC, dropped */
#define IPXLAN_InBroadcastRoute		(CALC_MET + 3670)
#define IPXLAN_InBroadcastRoute_TITLE	gettxt("RTPM:1129", "ipxlan_InBroadcastRoute")
#define IPXLAN_InBroadcastRoute_IDX 	METSTART + 509
#define IPXLANR_InBroadcastRoute_TITLE	gettxt("RTPM:1130", "ipxlan_InBroadcastRoute/s")
#define IPXLANR_InBroadcastRoute_IDX 	METSTART + 510

	/* Packets, destination not my net, forwarded to next router */
#define IPXLAN_InForward		(CALC_MET + 3680)
#define IPXLANR_InForward_TITLE	gettxt("RTPM:774", "ipxlan_InForward/s")
#define IPXLANR_InForward_IDX 	METSTART + 511

	/* Packets, routed to node on connected net */
#define IPXLAN_InRoute		(CALC_MET + 3690)
#define IPXLANR_InRoute_TITLE	gettxt("RTPM:775", "ipxlan_InRoute/s")
#define IPXLANR_InRoute_IDX 	METSTART + 512

	/* Packets, routed to internal net */
#define IPXLAN_InInternalNet		(CALC_MET + 3700)
#define IPXLANR_InInternalNet_TITLE	gettxt("RTPM:776", "ipxlan_InInternalNet/s")
#define IPXLANR_InInternalNet_IDX 	METSTART + 513

	/* Down stream IPX/Propagation packets propagated */
#define IPXLAN_OutPropagation		(CALC_MET + 3710)
#define IPXLANR_OutPropagation_TITLE	gettxt("RTPM:777", "ipxlan_OutPropagation/s")
#define IPXLANR_OutPropagation_IDX 	METSTART + 514

	/* Total IPX data packets from upstream */
#define IPXLAN_OutTotalStream		(CALC_MET + 3720)
#define IPXLANR_OutTotalStream_TITLE	gettxt("RTPM:778", "ipxlan_OutTotalStream/s")
#define IPXLANR_OutTotalStream_IDX 	METSTART + 515

	/* Total IPX data packets sent to a lan or internal net */
#define IPXLAN_OutTotal		(CALC_MET + 3730)
#define IPXLANR_OutTotal_TITLE	gettxt("RTPM:779", "ipxlan_OutTotal/s")
#define IPXLANR_OutTotal_IDX 	METSTART + 516

	/* Packets, returned to socket router, dest/src socket same, dropped */
#define IPXLAN_OutSameSocket		(CALC_MET + 3740)
#define IPXLAN_OutSameSocket_TITLE	gettxt("RTPM:780", "ipxlan_OutSameSocket")
#define IPXLAN_OutSameSocket_IDX 	METSTART + 517
#define IPXLANR_OutSameSocket_TITLE	gettxt("RTPM:781", "ipxlan_OutSameSocket/s")
#define IPXLANR_OutSameSocket_IDX 	METSTART + 518

	/* Packets, destination net/node filled with internal net/node */
#define IPXLAN_OutFillInDest		(CALC_MET + 3750)
#define IPXLANR_OutFillInDest_TITLE	gettxt("RTPM:782", "ipxlan_OutFillInDest/s")
#define IPXLANR_OutFillInDest_IDX 	METSTART + 519

	/* Packets, routed to internal net */
#define IPXLAN_OutInternal		(CALC_MET + 3760)
#define IPXLANR_OutInternal_TITLE	gettxt("RTPM:783", "ipxlan_OutInternal/s")
#define IPXLANR_OutInternal_IDX 	METSTART + 520

	/* Packets, router error, bad lan, dropped */
#define IPXLAN_OutBadLan		(CALC_MET + 3770)
#define IPXLAN_OutBadLan_TITLE	gettxt("RTPM:784", "ipxlan_OutBadLan")
#define IPXLAN_OutBadLan_IDX 	METSTART + 521
#define IPXLANR_OutBadLan_TITLE	gettxt("RTPM:785", "ipxlan_OutBadLan/s")
#define IPXLANR_OutBadLan_IDX 	METSTART + 522

	/* Packets, sent to lan */
#define IPXLAN_OutSent		(CALC_MET + 3780)
#define IPXLANR_OutSent_TITLE	gettxt("RTPM:786", "ipxlan_OutSent/s")
#define IPXLANR_OutSent_IDX 	METSTART + 523

	/* Packets, queued to lan */
#define IPXLAN_OutQueued		(CALC_MET + 3790)
#define IPXLANR_OutQueued_TITLE	gettxt("RTPM:787", "ipxlan_OutQueued/s")
#define IPXLANR_OutQueued_IDX 	METSTART + 524

	/* IOCTL packets total */
#define IPXLAN_Ioctl		(CALC_MET + 3800)
#define IPXLANR_Ioctl_TITLE	gettxt("RTPM:788", "ipxlan_Ioctl/s")
#define IPXLANR_Ioctl_IDX 	METSTART + 525

	/* Set Configured Lans */
#define IPXLAN_IoctlSetLans		(CALC_MET + 3810)
#define IPXLANR_IoctlSetLans_TITLE	gettxt("RTPM:789", "ipxlan_IoctlSetLans/s")
#define IPXLANR_IoctlSetLans_IDX 	METSTART + 526

	/* Get Configured Lans */
#define IPXLAN_IoctlGetLans		(CALC_MET + 3820)
#define IPXLANR_IoctlGetLans_TITLE	gettxt("RTPM:790", "ipxlan_IoctlGetLans/s")
#define IPXLANR_IoctlGetLans_IDX 	METSTART + 527

	/* Set SAP Queue */
#define IPXLAN_IoctlSetSapQ		(CALC_MET + 3830)
#define IPXLANR_IoctlSetSapQ_TITLE	gettxt("RTPM:791", "ipxlan_IoctlSetSapQ/s")
#define IPXLANR_IoctlSetSapQ_IDX 	METSTART + 528

	/* Set Lan Info */
#define IPXLAN_IoctlSetLanInfo		(CALC_MET + 3840)
#define IPXLANR_IoctlSetLanInfo_TITLE	gettxt("RTPM:792", "ipxlan_IoctlSetLanInfo/s")
#define IPXLANR_IoctlSetLanInfo_IDX 	METSTART + 529

	/* Get Lan Info */
#define IPXLAN_IoctlGetLanInfo		(CALC_MET + 3850)
#define IPXLANR_IoctlGetLanInfo_TITLE	gettxt("RTPM:793", "ipxlan_IoctlGetLanInfo/s")
#define IPXLANR_IoctlGetLanInfo_IDX 	METSTART + 530

	/* Get Node Addr */
#define IPXLAN_IoctlGetNodeAddr		(CALC_MET + 3860)
#define IPXLANR_IoctlGetNodeAddr_TITLE	gettxt("RTPM:794", "ipxlan_IoctlGetNodeAddr/s")
#define IPXLANR_IoctlGetNodeAddr_IDX 	METSTART + 531

	/* Get Net Addr */
#define IPXLAN_IoctlGetNetAddr		(CALC_MET + 3870)
#define IPXLANR_IoctlGetNetAddr_TITLE	gettxt("RTPM:795", "ipxlan_IoctlGetNetAddr/s")
#define IPXLANR_IoctlGetNetAddr_IDX 	METSTART + 532

	/* Get Statistics */
#define IPXLAN_IoctlGetStats		(CALC_MET + 3880)
#define IPXLANR_IoctlGetStats_TITLE	gettxt("RTPM:796", "ipxlan_IoctlGetStats/s")
#define IPXLANR_IoctlGetStats_IDX 	METSTART + 533

	/* Link */
#define IPXLAN_IoctlLink		(CALC_MET + 3890)
#define IPXLANR_IoctlLink_TITLE	gettxt("RTPM:797", "ipxlan_IoctlLink/s")
#define IPXLANR_IoctlLink_IDX 	METSTART + 534

	/* Unlink */
#define IPXLAN_IoctlUnlink		(CALC_MET + 3900)
#define IPXLANR_IoctlUnlink_TITLE	gettxt("RTPM:798", "ipxlan_IoctlUnlink/s")
#define IPXLANR_IoctlUnlink_IDX 	METSTART + 535

	/* Unknown type */
#define IPXLAN_IoctlUnknown		(CALC_MET + 3910)
#define IPXLAN_IoctlUnknown_TITLE	gettxt("RTPM:799", "ipxlan_IoctlUnknown")
#define IPXLAN_IoctlUnknown_IDX 	METSTART + 536
#define IPXLANR_IoctlUnknown_TITLE	gettxt("RTPM:800", "ipxlan_IoctlUnknown/s")
#define IPXLANR_IoctlUnknown_IDX 	METSTART + 537

	/* Sent to the socket multiplexor */
#define IPXSOCK_IpxInData		(CALC_MET + 3920)
#define IPXSOCKR_IpxInData_TITLE	gettxt("RTPM:801", "ipxsock_IpxInData/s")
#define IPXSOCKR_IpxInData_IDX 	METSTART + 538

	/* Total IPX data packets received from the stream head */
#define IPX_datapackets		(CALC_MET + 3930)
#define IPXR_datapackets_IDX 	METSTART + 539
#define IPXR_datapackets_TITLE	gettxt("RTPM:802", "ipx_datapackets/s")

	/* Non TLI IPX data packets */
#define IPXSOCK_IpxOutData		(CALC_MET + 3940)
#define IPXSOCKR_IpxOutData_TITLE	gettxt("RTPM:803", "ipxsock_IpxOutData/s")
#define IPXSOCKR_IpxOutData_IDX 	METSTART + 540

	/* Length less than IPX header size, dropped */
#define IPXSOCK_IpxOutBadSize		(CALC_MET + 3950)
#define IPXSOCK_IpxOutBadSize_TITLE	gettxt("RTPM:804", "ipxsock_IpxOutBadSize")
#define IPXSOCK_IpxOutBadSize_IDX 	METSTART + 541
#define IPXSOCKR_IpxOutBadSize_TITLE	gettxt("RTPM:805", "ipxsock_IpxOutBadSize/s")
#define IPXSOCKR_IpxOutBadSize_IDX 	METSTART + 542

	/* Sent to IPX switch */
#define IPXSOCK_IpxOutToSwitch		(CALC_MET + 3960)
#define IPXSOCKR_IpxOutToSwitch_TITLE	gettxt("RTPM:806", "ipxsock_IpxOutToSwitch/s")
#define IPXSOCKR_IpxOutToSwitch_IDX 	METSTART + 543

	/* TLI IPX data packets */
#define IPXSOCK_IpxTLIOutData		(CALC_MET + 3970)
#define IPXSOCKR_IpxTLIOutData_TITLE	gettxt("RTPM:807", "ipxsock_IpxTLIOutData/s")
#define IPXSOCKR_IpxTLIOutData_IDX 	METSTART + 544

	/* Bad TLI state, packet dropped */
#define IPXSOCK_IpxTLIOutBadState		(CALC_MET + 3980)
#define IPXSOCK_IpxTLIOutBadState_TITLE	gettxt("RTPM:808", "ipxsock_IpxTLIOutBadState")
#define IPXSOCK_IpxTLIOutBadState_IDX 	METSTART + 545
#define IPXSOCKR_IpxTLIOutBadState_TITLE	gettxt("RTPM:809", "ipxsock_IpxTLIOutBadState/s")
#define IPXSOCKR_IpxTLIOutBadState_IDX 	METSTART + 546

	/* Bad IPX address size, packet dropped */
#define IPXSOCK_IpxTLIOutBadSize		(CALC_MET + 3990)
#define IPXSOCK_IpxTLIOutBadSize_TITLE	gettxt("RTPM:810", "ipxsock_IpxTLIOutBadSize")
#define IPXSOCK_IpxTLIOutBadSize_IDX 	METSTART + 547
#define IPXSOCKR_IpxTLIOutBadSize_TITLE	gettxt("RTPM:811", "ipxsock_IpxTLIOutBadSize/s")
#define IPXSOCKR_IpxTLIOutBadSize_IDX 	METSTART + 548

	/* Bad TLI option size, packet dropped */
#define IPXSOCK_IpxTLIOutBadOpt		(CALC_MET + 4000)
#define IPXSOCK_IpxTLIOutBadOpt_TITLE	gettxt("RTPM:812", "ipxsock_IpxTLIOutBadOpt")
#define IPXSOCK_IpxTLIOutBadOpt_IDX 	METSTART + 549
#define IPXSOCKR_IpxTLIOutBadOpt_TITLE	gettxt("RTPM:813", "ipxsock_IpxTLIOutBadOpt/s")
#define IPXSOCKR_IpxTLIOutBadOpt_IDX 	METSTART + 550

	/* Allocation of IPX header failed, packet dropped */
#define IPXSOCK_IpxTLIOutHdrAlloc		(CALC_MET + 4010)
#define IPXSOCK_IpxTLIOutHdrAlloc_TITLE	gettxt("RTPM:814", "ipxsock_IpxTLIOutHdrAlloc")
#define IPXSOCK_IpxTLIOutHdrAlloc_IDX 	METSTART + 551
#define IPXSOCKR_IpxTLIOutHdrAlloc_TITLE	gettxt("RTPM:815", "ipxsock_IpxTLIOutHdrAlloc/s")
#define IPXSOCKR_IpxTLIOutHdrAlloc_IDX 	METSTART + 552

	/* Sent to IPX switch */
#define IPXSOCK_IpxTLIOutToSwitch		(CALC_MET + 4020)
#define IPXSOCKR_IpxTLIOutToSwitch_TITLE	gettxt("RTPM:816", "ipxsock_IpxTLIOutToSwitch/s")
#define IPXSOCKR_IpxTLIOutToSwitch_IDX 	METSTART + 553

	/* Sockets Bound */
#define IPXSOCK_IpxBoundSockets		(CALC_MET + 4030)
#define IPXSOCK_IpxBoundSockets_TITLE	gettxt("RTPM:817", "ipxsock_IpxBoundSockets")
#define IPXSOCK_IpxBoundSockets_IDX 	METSTART + 554
#define IPXSOCKR_IpxBoundSockets_TITLE	gettxt("RTPM:818", "ipxsock_IpxBoundSockets/s")
#define IPXSOCKR_IpxBoundSockets_IDX 	METSTART + 555

	/* Non TLI Bind Socket Requests */
#define IPXSOCK_IpxBind		(CALC_MET + 4040)
#define IPXSOCKR_IpxBind_TITLE	gettxt("RTPM:819", "ipxsock_IpxBind/s")
#define IPXSOCKR_IpxBind_IDX 	METSTART + 556

	/* TLI Bind Socket Requests */
#define IPXSOCK_IpxTLIBind		(CALC_MET + 4050)
#define IPXSOCKR_IpxTLIBind_TITLE	gettxt("RTPM:820", "ipxsock_IpxTLIBind/s")
#define IPXSOCKR_IpxTLIBind_IDX 	METSTART + 557

	/* TLI Option Management Requests */
#define IPXSOCK_IpxTLIOptMgt		(CALC_MET + 4060)
#define IPXSOCKR_IpxTLIOptMgt_TITLE	gettxt("RTPM:821", "ipxsock_IpxTLIOptMgt/s")
#define IPXSOCKR_IpxTLIOptMgt_IDX 	METSTART + 558

	/* TLI Unknown Requests */
#define IPXSOCK_IpxTLIUnknown		(CALC_MET + 4070)
#define IPXSOCK_IpxTLIUnknown_TITLE	gettxt("RTPM:822", "ipxsock_IpxTLIUnknown")
#define IPXSOCK_IpxTLIUnknown_IDX 	METSTART + 559
#define IPXSOCKR_IpxTLIUnknown_TITLE	gettxt("RTPM:823", "ipxsock_IpxTLIUnknown/s")
#define IPXSOCKR_IpxTLIUnknown_IDX 	METSTART + 560

	/* IPX TLI Output pkts with bad address size */
#define IPXSOCK_IpxTLIOutBadAddr	(CALC_MET + 4080)
#define IPXSOCK_IpxTLIOutBadAddr_TITLE	gettxt("RTPM:824", "ipxsock_IpxTLIOutBadAddr")
#define IPXSOCKR_IpxTLIOutBadAddr_TITLE	gettxt("RTPM:825", "ipxsock_IpxTLIOutBadAddr/s")
#define IPXSOCK_IpxTLIOutBadAddr_IDX 	METSTART + 561
#define IPXSOCKR_IpxTLIOutBadAddr_IDX 	METSTART + 562

	/* BIND_SOCKET user sent packet with zero socket, packet dropped */
#define IPXSOCK_IpxSwitchInvalSocket		(CALC_MET + 4100)
#define IPXSOCK_IpxSwitchInvalSocket_TITLE	gettxt("RTPM:826", "ipxsock_IpxSwitchInvalSocket")
#define IPXSOCK_IpxSwitchInvalSocket_IDX 	METSTART + 563
#define IPXSOCKR_IpxSwitchInvalSocket_TITLE	gettxt("RTPM:827", "ipxsock_IpxSwitchInvalSocket/s")
#define IPXSOCKR_IpxSwitchInvalSocket_IDX 	METSTART + 564

	/* Failure to generate checksum, packet dropped */
#define IPXSOCK_IpxSwitchSumFail		(CALC_MET + 4110)
#define IPXSOCK_IpxSwitchSumFail_TITLE	gettxt("RTPM:828", "ipxsock_IpxSwitchSumFail")
#define IPXSOCK_IpxSwitchSumFail_IDX 	METSTART + 565
#define IPXSOCKR_IpxSwitchSumFail_TITLE	gettxt("RTPM:829", "ipxsock_IpxSwitchSumFail/s")
#define IPXSOCKR_IpxSwitchSumFail_IDX 	METSTART + 566

	/* dropped, could not allocate block for padding */
#define IPXSOCK_IpxSwitchAllocFail		(CALC_MET + 4120)
#define IPXSOCK_IpxSwitchAllocFail_TITLE	gettxt("RTPM:830", "ipxsock_IpxSwitchAllocFail")
#define IPXSOCK_IpxSwitchAllocFail_IDX 	METSTART + 567
#define IPXSOCKR_IpxSwitchAllocFail_TITLE	gettxt("RTPM:831", "ipxsock_IpxSwitchAllocFail/s")
#define IPXSOCKR_IpxSwitchAllocFail_IDX 	METSTART + 568

	/* checksum generated */
#define IPXSOCK_IpxSwitchSum		(CALC_MET + 4130)
#define IPXSOCKR_IpxSwitchSum_TITLE	gettxt("RTPM:832", "ipxsock_IpxSwitchSum/s")
#define IPXSOCKR_IpxSwitchSum_IDX 	METSTART + 569

	/* padded to an even number of bytes */
#define IPXSOCK_IpxSwitchEven		(CALC_MET + 4140)
#define IPXSOCKR_IpxSwitchEven_TITLE	gettxt("RTPM:833", "ipxsock_IpxSwitchEven/s")
#define IPXSOCKR_IpxSwitchEven_IDX 	METSTART + 570

	/* padded, buffer alloc required */
#define IPXSOCK_IpxSwitchEvenAlloc		(CALC_MET + 4150)
#define IPXSOCKR_IpxSwitchEvenAlloc_TITLE	gettxt("RTPM:834", "ipxsock_IpxSwitchEvenAlloc/s")
#define IPXSOCKR_IpxSwitchEvenAlloc_IDX 	METSTART + 571

#define DUMMY_1					(CALC_MET + 4160)
#define DUMMY_1_TITLE				"dummy_1"
#define DUMMY_1_IDX			 	METSTART + 572

	/* Total data packets received by the socket multiplexor */
#define IPXSOCK_IpxDataToSocket		(CALC_MET + 4170)
#define IPXSOCKR_IpxDataToSocket_TITLE	gettxt("RTPM:836", "ipxsock_IpxDataToSocket/s")
#define IPXSOCKR_IpxDataToSocket_IDX 	METSTART + 573

	/* Data size trimmed to match IPX data size */
#define IPXSOCK_IpxTrimPacket		(CALC_MET + 4180)
#define IPXSOCKR_IpxTrimPacket_TITLE	gettxt("RTPM:837", "ipxsock_IpxTrimPacket/s")
#define IPXSOCKR_IpxTrimPacket_IDX 	METSTART + 574

	/* Ipx checksum invalid, drop packet */
#define IPXSOCK_IpxSumFail		(CALC_MET + 4190)
#define IPXSOCK_IpxSumFail_TITLE	gettxt("RTPM:838", "ipxsock_IpxSumFail")
#define IPXSOCK_IpxSumFail_IDX 	METSTART + 575
#define IPXSOCKR_IpxSumFail_TITLE	gettxt("RTPM:839", "ipxsock_IpxSumFail/s")
#define IPXSOCKR_IpxSumFail_IDX 	METSTART + 576

	/* Packets dropped because upper stream full */
#define IPXSOCK_IpxBusySocket		(CALC_MET + 4200)
#define IPXSOCK_IpxBusySocket_TITLE	gettxt("RTPM:840", "ipxsock_IpxBusySocket")
#define IPXSOCK_IpxBusySocket_IDX 	METSTART + 577
#define IPXSOCKR_IpxBusySocket_TITLE	gettxt("RTPM:841", "ipxsock_IpxBusySocket/s")
#define IPXSOCKR_IpxBusySocket_IDX 	METSTART + 578

	/* Packets dropped, socket not bound */
#define IPXSOCK_IpxSocketNotBound		(CALC_MET + 4210)
#define IPXSOCK_IpxSocketNotBound_TITLE	gettxt("RTPM:842", "ipxsock_IpxSocketNotBound")
#define IPXSOCK_IpxSocketNotBound_IDX 	METSTART + 579
#define IPXSOCKR_IpxSocketNotBound_TITLE	gettxt("RTPM:843", "ipxsock_IpxSocketNotBound/s")
#define IPXSOCKR_IpxSocketNotBound_IDX 	METSTART + 580

	/* Valid socket found */
#define IPXSOCK_IpxRouted		(CALC_MET + 4220)
#define IPXSOCKR_IpxRouted_TITLE	gettxt("RTPM:844", "ipxsock_IpxRouted/s")
#define IPXSOCKR_IpxRouted_IDX 	METSTART + 581

	/* Destined for TLI socket */
#define IPXSOCK_IpxRoutedTLI		(CALC_MET + 4230)
#define IPXSOCKR_IpxRoutedTLI_TITLE	gettxt("RTPM:845", "ipxsock_IpxRoutedTLI/s")
#define IPXSOCKR_IpxRoutedTLI_IDX 	METSTART + 582

	/* Alloc of TLI header failed, packet dropped */
#define IPXSOCK_IpxRoutedTLIAlloc		(CALC_MET + 4240)
#define IPXSOCK_IpxRoutedTLIAlloc_TITLE	gettxt("RTPM:846", "ipxsock_IpxRoutedTLIAlloc")
#define IPXSOCK_IpxRoutedTLIAlloc_IDX 	METSTART + 583
#define IPXSOCKR_IpxRoutedTLIAlloc_TITLE	gettxt("RTPM:847", "ipxsock_IpxRoutedTLIAlloc/s")
#define IPXSOCKR_IpxRoutedTLIAlloc_IDX 	METSTART + 584

	/* Sent to TLI socket */
#define IPX_sent_to_tli		(CALC_MET + 4250)
#define IPXR_sent_to_tli_IDX	METSTART + 585
#define IPXR_sent_to_tli_TITLE	gettxt("RTPM:848", "ipx_sent_to_tli/s")
	/* Total Ioctls processed */
#define IPX_total_ioctls	(CALC_MET + 4260)
#define IPXR_total_ioctls_IDX	METSTART + 586
#define IPXR_total_ioctls_TITLE	gettxt("RTPM:849", "ipx_total_ioctls/s")
	/* IOCTL requests SET_WATER */
#define IPXSOCK_IpxIoctlSetWater		(CALC_MET + 4270)
#define IPXSOCKR_IpxIoctlSetWater_TITLE	gettxt("RTPM:850", "ipxsock_IpxIoctlSetWater/s")
#define IPXSOCKR_IpxIoctlSetWater_IDX 	METSTART + 587

	/* IOCTL requests SET_SOCKET or BIND_SOCKET */
#define IPXSOCK_IpxIoctlBindSocket		(CALC_MET + 4280)
#define IPXSOCKR_IpxIoctlBindSocket_TITLE	gettxt("RTPM:851", "ipxsock_IpxIoctlBindSocket/s")
#define IPXSOCKR_IpxIoctlBindSocket_IDX 	METSTART + 588

	/* IOCTL requests UNBIND_SOCKET */
#define IPXSOCK_IpxIoctlUnbindSocket		(CALC_MET + 4290)
#define IPXSOCKR_IpxIoctlUnbindSocket_TITLE	gettxt("RTPM:852", "ipxsock_IpxIoctlUnbindSocket/s")
#define IPXSOCKR_IpxIoctlUnbindSocket_IDX 	METSTART + 589

	/* IOCTL requests STATS */
#define IPXSOCK_IpxIoctlStats		(CALC_MET + 4300)
#define IPXSOCKR_IpxIoctlStats_TITLE	gettxt("RTPM:853", "ipxsock_IpxIoctlStats/s")
#define IPXSOCKR_IpxIoctlStats_IDX 	METSTART + 590

	/* IOCTL requests Unknown, sent to lan router */
#define IPXSOCK_IpxIoctlUnknown		(CALC_MET + 4310)
#define IPXSOCK_IpxIoctlUnknown_TITLE	gettxt("RTPM:854", "ipxsock_IpxIoctlUnknown")
#define IPXSOCK_IpxIoctlUnknown_IDX 	METSTART + 591
#define IPXSOCKR_IpxIoctlUnknown_TITLE	gettxt("RTPM:855", "ipxsock_IpxIoctlUnknown/s")
#define IPXSOCKR_IpxIoctlUnknown_IDX 	METSTART + 592


	/* Total router packets received */
#define RIP_ReceivedPackets		(CALC_MET + 4320)
#define RIPR_ReceivedPackets_TITLE	gettxt("RTPM:856", "rip_ReceivedPackets/s")
#define RIPR_ReceivedPackets_IDX 	METSTART + 593

	/* Could not generate lan key, dropped */
#define RIP_ReceivedNoLanKey		(CALC_MET + 4330)
#define RIP_ReceivedNoLanKey_TITLE	gettxt("RTPM:857", "rip_ReceivedNoLanKey")
#define RIP_ReceivedNoLanKey_IDX 	METSTART + 594
#define RIPR_ReceivedNoLanKey_TITLE	gettxt("RTPM:858", "rip_ReceivedNoLanKey/s")
#define RIPR_ReceivedNoLanKey_IDX 	METSTART + 595

	/* Invalid router structure size, dropped */
#define RIP_ReceivedBadLength		(CALC_MET + 4340)
#define RIP_ReceivedBadLength_TITLE	gettxt("RTPM:859", "rip_ReceivedBadLength")
#define RIP_ReceivedBadLength_IDX 	METSTART + 596
#define RIPR_ReceivedBadLength_TITLE	gettxt("RTPM:860", "rip_ReceivedBadLength/s")
#define RIPR_ReceivedBadLength_IDX 	METSTART + 597

	/* Multiple message blocks coalesced */
#define RIP_ReceivedCoalesced		(CALC_MET + 4350)
#define RIPR_ReceivedCoalesced_TITLE	gettxt("RTPM:861", "rip_ReceivedCoalesced/s")
#define RIPR_ReceivedCoalesced_IDX 	METSTART + 598

	/* Coalesce Failure, dropped */
#define RIP_ReceivedNoCoalesce		(CALC_MET + 4360)
#define RIP_ReceivedNoCoalesce_TITLE	gettxt("RTPM:862", "rip_ReceivedNoCoalesce")
#define RIP_ReceivedNoCoalesce_IDX 	METSTART + 599
#define RIPR_ReceivedNoCoalesce_TITLE	gettxt("RTPM:863", "rip_ReceivedNoCoalesce/s")
#define RIPR_ReceivedNoCoalesce_IDX 	METSTART + 600

	/* Router request packets */
#define RIP_ReceivedRequestPackets		(CALC_MET + 4370)
#define RIPR_ReceivedRequestPackets_TITLE	gettxt("RTPM:864", "rip_ReceivedRequestPackets/s")
#define RIPR_ReceivedRequestPackets_IDX 	METSTART + 601

	/* Router response packets */
#define RIP_ReceivedResponsePackets		(CALC_MET + 4380)
#define RIPR_ReceivedResponsePackets_TITLE	gettxt("RTPM:865", "rip_ReceivedResponsePackets/s")
#define RIPR_ReceivedResponsePackets_IDX 	METSTART + 602

	/* Unknown request packets */
#define RIP_ReceivedUnknownRequest		(CALC_MET + 4390)
#define RIP_ReceivedUnknownRequest_TITLE	gettxt("RTPM:866", "rip_ReceivedUnknownRequest")
#define RIP_ReceivedUnknownRequest_IDX 	METSTART + 603
#define RIPR_ReceivedUnknownRequest_TITLE	gettxt("RTPM:867", "rip_ReceivedUnknownRequest/s")
#define RIPR_ReceivedUnknownRequest_IDX 	METSTART + 604

	/* Total router packets sent */
#define RIP_total_router_packets_sent		(CALC_MET + 4400)
#define RIPR_total_router_packets_sent_IDX	METSTART + 605
#define RIPR_total_router_packets_sent_TITLE	gettxt("RTPM:868","RIP_total_router_packets_sent/s")
	/* Could not allocate buffer for packet, ignored */
#define RIP_SentAllocFailed		(CALC_MET + 4410)
#define RIP_SentAllocFailed_TITLE	gettxt("RTPM:869", "rip_SentAllocFailed")
#define RIP_SentAllocFailed_IDX 	METSTART + 606
#define RIPR_SentAllocFailed_TITLE	gettxt("RTPM:870", "rip_SentAllocFailed/s")
#define RIPR_SentAllocFailed_IDX 	METSTART + 607

	/* Could not match destination with a net, ignored */
#define RIP_SentBadDestination		(CALC_MET + 4420)
#define RIP_SentBadDestination_TITLE	gettxt("RTPM:871", "rip_SentBadDestination")
#define RIP_SentBadDestination_IDX 	METSTART + 608
#define RIPR_SentBadDestination_TITLE	gettxt("RTPM:872", "rip_SentBadDestination/s")
#define RIPR_SentBadDestination_IDX 	METSTART + 609

	/* Router request packets sent */
#define RIP_SentRequestPackets		(CALC_MET + 4430)
#define RIPR_SentRequestPackets_TITLE	gettxt("RTPM:873", "rip_SentRequestPackets/s")
#define RIPR_SentRequestPackets_IDX 	METSTART + 610

	/* Router response packets sent */
#define RIP_SentResponsePackets		(CALC_MET + 4440)
#define RIPR_SentResponsePackets_TITLE	gettxt("RTPM:874", "rip_SentResponsePackets/s")
#define RIPR_SentResponsePackets_IDX 	METSTART + 611

	/* Total requests to build packets for the internal net, ignored */
#define RIP_SentLan0Dropped		(CALC_MET + 4450)
#define RIP_SentLan0Dropped_TITLE	gettxt("RTPM:875", "rip_SentLan0Dropped")
#define RIP_SentLan0Dropped_IDX 	METSTART + 612
#define RIPR_SentLan0Dropped_TITLE	gettxt("RTPM:876", "rip_SentLan0Dropped/s")
#define RIPR_SentLan0Dropped_IDX 	METSTART + 613

	/* Total router packets built for the internal net, routed to IPX */
#define RIP_SentLan0Routed		(CALC_MET + 4460)
#define RIPR_SentLan0Routed_TITLE	gettxt("RTPM:877", "rip_SentLan0Routed/s")
#define RIPR_SentLan0Routed_IDX 	METSTART + 614

	/* Ioctl requests processed */

#define RIP_ioctls_processed		(CALC_MET + 4470)
#define RIPR_ioctls_processed_IDX	METSTART + 615
#define RIPR_ioctls_processed_TITLE	gettxt("RTPM:878", "rip_ioctls_processed/s")
	/* ioctl RIPX_INITIALIZE */
#define RIP_RipxIoctlInitialize		(CALC_MET + 4480)
#define RIPR_RipxIoctlInitialize_TITLE	gettxt("RTPM:879", "rip_RipxIoctlInitialize/s")
#define RIPR_RipxIoctlInitialize_IDX 	METSTART + 616

	/* ioctl RIPX_GET_HASH_SIZE */
#define RIP_RipxIoctlGetHashSize		(CALC_MET + 4490)
#define RIPR_RipxIoctlGetHashSize_TITLE	gettxt("RTPM:880", "rip_RipxIoctlGetHashSize/s")
#define RIPR_RipxIoctlGetHashSize_IDX 	METSTART + 617

	/* ioctl RIPX_GET_HASH_STATS */
#define RIP_RipxIoctlGetHashStats		(CALC_MET + 4500)
#define RIPR_RipxIoctlGetHashStats_TITLE	gettxt("RTPM:881", "rip_RipxIoctlGetHashStats/s")
#define RIPR_RipxIoctlGetHashStats_IDX 	METSTART + 618

	/* ioctl RIPX_DUMP_HASH_TABLE */
#define RIP_RipxIoctlDumpHashTable		(CALC_MET + 4510)
#define RIPR_RipxIoctlDumpHashTable_TITLE	gettxt("RTPM:882", "rip_RipxIoctlDumpHashTable/s")
#define RIPR_RipxIoctlDumpHashTable_IDX 	METSTART + 619

	/* ioctl RIPX_GET_ROUTER_TABLE */
#define RIP_RipxIoctlGetRouterTable		(CALC_MET + 4520)
#define RIPR_RipxIoctlGetRouterTable_TITLE	gettxt("RTPM:883", "rip_RipxIoctlGetRouterTable/s")
#define RIPR_RipxIoctlGetRouterTable_IDX 	METSTART + 620

	/* ioctl RIPX_GET_NET_INFO */
#define RIP_RipxIoctlGetNetInfo		(CALC_MET + 4530)
#define RIPR_RipxIoctlGetNetInfo_TITLE	gettxt("RTPM:884", "rip_RipxIoctlGetNetInfo/s")
#define RIPR_RipxIoctlGetNetInfo_IDX 	METSTART + 621

	/* ioctl RIPX_CHECK_SAP_SOURCE */
#define RIP_RipxIoctlCheckSapSource		(CALC_MET + 4540)
#define RIPR_RipxIoctlCheckSapSource_TITLE	gettxt("RTPM:885", "rip_RipxIoctlCheckSapSource/s")
#define RIPR_RipxIoctlCheckSapSource_IDX 	METSTART + 622

	/* ioctl RIPX_RESET_ROUTER */
#define RIP_RipxIoctlResetRouter		(CALC_MET + 4550)
#define RIPR_RipxIoctlResetRouter_TITLE	gettxt("RTPM:886", "rip_RipxIoctlResetRouter/s")
#define RIPR_RipxIoctlResetRouter_IDX 	METSTART + 623

	/* ioctl RIPX_DOWN_ROUTER */
#define RIP_RipxIoctlDownRouter		(CALC_MET + 4560)
#define RIPR_RipxIoctlDownRouter_TITLE	gettxt("RTPM:887", "rip_RipxIoctlDownRouter/s")
#define RIPR_RipxIoctlDownRouter_IDX 	METSTART + 624

	/* ioctl RIPX_STATS */
#define RIP_RipxIoctlStats		(CALC_MET + 4570)
#define RIPR_RipxIoctlStats_TITLE	gettxt("RTPM:888", "rip_RipxIoctlStats/s")
#define RIPR_RipxIoctlStats_IDX 	METSTART + 625

	/* Unknown Ioctls */
#define RIP_RipxIoctlUnknown		(CALC_MET + 4580)
#define RIP_RipxIoctlUnknown_TITLE	gettxt("RTPM:889", "rip_RipxIoctlUnknown")
#define RIP_RipxIoctlUnknown_IDX 	METSTART + 626
#define RIPR_RipxIoctlUnknown_TITLE	gettxt("RTPM:890", "rip_RipxIoctlUnknown/s")
#define RIPR_RipxIoctlUnknown_IDX 	METSTART + 627

	/* Current SPX connections. */
#define SPX_current_connections		(CALC_MET + 4590)
#define SPX_current_connections_TITLE	gettxt("RTPM:891", "spx_current_connections")
#define SPX_current_connections_IDX 	METSTART + 628
#define SPXR_current_connections_TITLE	gettxt("RTPM:892", "spx_current_connections/s")
#define SPXR_current_connections_IDX 	METSTART + 629

	/* Stream message allocation failures. */
#define SPX_alloc_failures		(CALC_MET + 4600)
#define SPX_alloc_failures_TITLE	gettxt("RTPM:893", "spx_alloc_failures")
#define SPX_alloc_failures_IDX 	METSTART + 630
#define SPXR_alloc_failures_TITLE	gettxt("RTPM:894", "spx_alloc_failures/s")
#define SPXR_alloc_failures_IDX 	METSTART + 631

	/* Opens of SPX failed. */
#define SPX_open_failures		(CALC_MET + 4610)
#define SPX_open_failures_TITLE	gettxt("RTPM:895", "spx_open_failures")
#define SPX_open_failures_IDX 	METSTART + 632
#define SPXR_open_failures_TITLE	gettxt("RTPM:896", "spx_open_failures/s")
#define SPXR_open_failures_IDX 	METSTART + 633

	/* Ioctls received from applications. */
#define SPX_ioctls		(CALC_MET + 4620)
#define SPXR_ioctls_TITLE	gettxt("RTPM:897", "spx_ioctls/s")
#define SPXR_ioctls_IDX 	METSTART + 634

	/* Connect requests received from applications. */
#define SPX_connect_req_count		(CALC_MET + 4630)
#define SPXR_connect_req_count_TITLE	gettxt("RTPM:898", "spx_connect_req_count/s")
#define SPXR_connect_req_count_IDX 	METSTART + 635

	/* Connect requests from applications failed. */
#define SPX_connect_req_fails		(CALC_MET + 4640)
#define SPX_connect_req_fails_TITLE	gettxt("RTPM:899", "spx_connect_req_fails")
#define SPX_connect_req_fails_IDX 	METSTART + 636
#define SPXR_connect_req_fails_TITLE	gettxt("RTPM:900", "spx_connect_req_fails/s")
#define SPXR_connect_req_fails_IDX 	METSTART + 637

	/* Listens posted by applications. */
#define SPX_listen_req		(CALC_MET + 4650)
#define SPXR_listen_req_TITLE	gettxt("RTPM:901", "spx_listen_req/s")
#define SPXR_listen_req_IDX 	METSTART + 638

	/* Listens posted by applications failed. */
#define SPX_listen_req_fails		(CALC_MET + 4660)
#define SPX_listen_req_fails_TITLE	gettxt("RTPM:902", "spx_listen_req_fails")
#define SPX_listen_req_fails_IDX 	METSTART + 639
#define SPXR_listen_req_fails_TITLE	gettxt("RTPM:903", "spx_listen_req_fails/s")
#define SPXR_listen_req_fails_IDX 	METSTART + 640

	/* Stream messages sent to SPX from applications. */
#define SPX_send_mesg_count		(CALC_MET + 4670)
#define SPXR_send_mesg_count_TITLE	gettxt("RTPM:904", "spx_send_mesg_count/s")
#define SPXR_send_mesg_count_IDX 	METSTART + 641

	/* Unknown messages sent to SPX from applications. */
#define SPX_unknown_mesg_count		(CALC_MET + 4680)
#define SPX_unknown_mesg_count_TITLE	gettxt("RTPM:905", "spx_unknown_mesg_count")
#define SPX_unknown_mesg_count_IDX 	METSTART + 642
#define SPXR_unknown_mesg_count_TITLE	gettxt("RTPM:906", "spx_unknown_mesg_count/s")
#define SPXR_unknown_mesg_count_IDX 	METSTART + 643

	/* Bad messages sent to SPX from applications. */
#define SPX_send_bad_mesg		(CALC_MET + 4690)
#define SPX_send_bad_mesg_TITLE	gettxt("RTPM:907", "spx_send_bad_mesg")
#define SPX_send_bad_mesg_IDX 	METSTART + 644
#define SPXR_send_bad_mesg_TITLE	gettxt("RTPM:908", "spx_send_bad_mesg/s")
#define SPXR_send_bad_mesg_IDX 	METSTART + 645

	/* SPX packets sent to IPX. */
#define SPX_send_packet_count		(CALC_MET + 4700)
#define SPXR_send_packet_count_TITLE	gettxt("RTPM:909", "spx_send_packet_count/s")
#define SPXR_send_packet_count_IDX 	METSTART + 646

	/* SPX packets retransmitted due to timeouts. */
#define SPX_send_packet_timeout		(CALC_MET + 4710)
#define SPX_send_packet_timeout_TITLE	gettxt("RTPM:910", "spx_send_packet_timeout")
#define SPX_send_packet_timeout_IDX 	METSTART + 647
#define SPXR_send_packet_timeout_TITLE	gettxt("RTPM:911", "spx_send_packet_timeout/s")
#define SPXR_send_packet_timeout_IDX 	METSTART + 648

	/* SPX packets retransmitted due to NAKs received. */
#define SPX_send_packet_nak		(CALC_MET + 4720)
#define SPX_send_packet_nak_TITLE	gettxt("RTPM:912", "spx_send_packet_nak")
#define SPX_send_packet_nak_IDX 	METSTART + 649
#define SPXR_send_packet_nak_TITLE	gettxt("RTPM:913", "spx_send_packet_nak/s")
#define SPXR_send_packet_nak_IDX 	METSTART + 650

	/* Packets received from IPX. */
#define SPX_rcv_packet_count		(CALC_MET + 4730)
#define SPXR_rcv_packet_count_TITLE	gettxt("RTPM:914", "spx_rcv_packet_count/s")
#define SPXR_rcv_packet_count_IDX 	METSTART + 651

	/* Bad SPX packets received from IPX. */
#define SPX_rcv_bad_packet		(CALC_MET + 4740)
#define SPX_rcv_bad_packet_TITLE	gettxt("RTPM:915", "spx_rcv_bad_packet")
#define SPX_rcv_bad_packet_IDX 	METSTART + 652
#define SPXR_rcv_bad_packet_TITLE	gettxt("RTPM:916", "spx_rcv_bad_packet/s")
#define SPXR_rcv_bad_packet_IDX 	METSTART + 653

	/* Bad SPX DATA packets received from IPX. */
#define SPX_rcv_bad_data_packet		(CALC_MET + 4750)
#define SPX_rcv_bad_data_packet_TITLE	gettxt("RTPM:917", "spx_rcv_bad_data_packet")
#define SPX_rcv_bad_data_packet_IDX 	METSTART + 654
#define SPXR_rcv_bad_data_packet_TITLE	gettxt("RTPM:918", "spx_rcv_bad_data_packet/s")
#define SPXR_rcv_bad_data_packet_IDX 	METSTART + 655

	/* Duplicate SPX DATA packets received. */
#define SPX_rcv_dup_packet		(CALC_MET + 4760)
#define SPX_rcv_dup_packet_TITLE	gettxt("RTPM:919", "spx_rcv_dup_packet")
#define SPX_rcv_dup_packet_IDX 	METSTART + 656
#define SPXR_rcv_dup_packet_TITLE	gettxt("RTPM:920", "spx_rcv_dup_packet/s")
#define SPXR_rcv_dup_packet_IDX 	METSTART + 657

	/* Packets received that were sent up to applications. */
#define SPX_rcv_packet_sentup		(CALC_MET + 4770)
#define SPXR_rcv_packet_sentup_TITLE	gettxt("RTPM:921", "spx_rcv_packet_sentup/s")
#define SPXR_rcv_packet_sentup_IDX 	METSTART + 658

	/* Connect Request packets received from IPX. */
#define SPX_rcv_conn_req		(CALC_MET + 4780)
#define SPX_rcv_conn_req_TITLE	gettxt("RTPM:922", "spx_rcv_conn_req")
#define SPX_rcv_conn_req_IDX 	METSTART + 659
#define SPXR_rcv_conn_req_TITLE	gettxt("RTPM:923", "spx_rcv_conn_req/s")
#define SPXR_rcv_conn_req_IDX 	METSTART + 660

	/* Connection aborted. */
#define SPX_abort_connection		(CALC_MET + 4790)
#define SPX_abort_connection_TITLE	gettxt("RTPM:924", "spx_abort_connection")
#define SPX_abort_connection_IDX 	METSTART + 661
#define SPXR_abort_connection_TITLE	gettxt("RTPM:925", "spx_abort_connection/s")
#define SPXR_abort_connection_IDX 	METSTART + 662

	/* Connection aborted due to max retries exceeded. */
#define SPX_max_retries_abort		(CALC_MET + 4800)
#define SPX_max_retries_abort_TITLE	gettxt("RTPM:926", "spx_max_retries_abort")
#define SPX_max_retries_abort_IDX 	METSTART + 663
#define SPXR_max_retries_abort_TITLE	gettxt("RTPM:927", "spx_max_retries_abort/s")
#define SPXR_max_retries_abort_IDX 	METSTART + 664

	/* Connect Request received from IPX with no listeners. */
#define SPX_no_listeners		(CALC_MET + 4810)
#define SPX_no_listeners_TITLE	gettxt("RTPM:928", "spx_no_listeners")
#define SPX_no_listeners_IDX 	METSTART + 665
#define SPXR_no_listeners_TITLE	gettxt("RTPM:929", "spx_no_listeners/s")
#define SPXR_no_listeners_IDX 	METSTART + 666


	/* per Minor # (per connection) stats */
	/* Network Address:	 */
#define SPXCON_netaddr		(CALC_MET + 4820)
#define SPXCON_netaddr_TITLE	gettxt("RTPM:930", "spxcon_netaddr")
#define SPXCON_netaddr_IDX 	METSTART + 667

	/* NODE: */
#define SPXCON_nodeaddr		(CALC_MET + 4830)
#define SPXCON_nodeaddr_TITLE	gettxt("RTPM:931", "spxcon_nodeaddr")
#define SPXCON_nodeaddr_IDX 	METSTART + 668

	/* SOCKET: */
#define SPXCON_sockaddr		(CALC_MET + 4840)
#define SPXCON_sockaddr_TITLE	gettxt("RTPM:932", "spxcon_sockaddr")
#define SPXCON_sockaddr_IDX 	METSTART + 669

	/* SPX Connection Number. */
#define SPXCON_connection_id		(CALC_MET + 4850)
#define SPXCON_connection_id_TITLE	gettxt("RTPM:933", "spxcon_connection_id")
#define SPXCON_connection_id_IDX 	METSTART + 670

	/* Address: */
#define SPXCON_o_netaddr		(CALC_MET + 4860)
#define SPXCON_o_netaddr_TITLE	gettxt("RTPM:934", "spxcon_o_netaddr")
#define SPXCON_o_netaddr_IDX 	METSTART + 671

	/* NODE */
#define SPXCON_o_nodeaddr		(CALC_MET + 4870)
#define SPXCON_o_nodeaddr_TITLE	gettxt("RTPM:935", "spxcon_o_nodeaddr")
#define SPXCON_o_nodeaddr_IDX 	METSTART + 672

	/* SOCKET */
#define SPXCON_o_sockaddr		(CALC_MET + 4880)
#define SPXCON_o_sockaddr_TITLE	gettxt("RTPM:936", "spxcon_o_sockaddr")
#define SPXCON_o_sockaddr_IDX 	METSTART + 673

	/* SPX Connection Number. */
#define SPXCON_o_connection_id		(CALC_MET + 4890)
#define SPXCON_o_connection_id_TITLE	gettxt("RTPM:937", "spxcon_o_connection_id")
#define SPXCON_o_connection_id_IDX 	METSTART + 674

	/* State of SPX connection. */
#define SPXCON_con_state		(CALC_MET + 4900)
#define SPXCON_con_state_TITLE	gettxt("RTPM:938", "spxcon_con_state")
#define SPXCON_con_state_IDX 	METSTART + 675

	/* Maximum retries before disconnecting. */
#define SPXCON_con_retry_count		(CALC_MET + 4910)
#define SPXCON_con_retry_count_TITLE	gettxt("RTPM:939", "spxcon_con_retry_count")
#define SPXCON_con_retry_count_IDX 	METSTART + 676

	/* Milliseconds minimum between retries. */
#define SPXCON_con_retry_time		(CALC_MET + 4920)
#define SPXCON_con_retry_time_TITLE	gettxt("RTPM:940", "spxcon_con_retry_time")
#define SPXCON_con_retry_time_IDX 	METSTART + 677

	/* Conn type (2: SPXII endpoint, 1: SPX endpoint, other: unk endpoint) */
#define SPXCON_con_type		(CALC_MET + 4940)
#define SPXCON_con_type_TITLE	gettxt("RTPM:941", "spxcon_con_type")
#define SPXCON_con_type_IDX 	METSTART + 678

	/* Is Connection is using IPX CHECKSUMS. */
#define SPXCON_con_ipxChecksum		(CALC_MET + 4950)
#define SPXCON_con_ipxChecksum_TITLE	gettxt("RTPM:942", "spxcon_con_ipxChecksum")
#define SPXCON_con_ipxChecksum_IDX 	METSTART + 679

	/* Current receive window size. */
#define SPXCON_con_window_size		(CALC_MET + 4960)
#define SPXCON_con_window_size_TITLE	gettxt("RTPM:943", "spxcon_con_window_size")
#define SPXCON_con_window_size_IDX 	METSTART + 680

	/* Current transmit window size. */
#define SPXCON_con_remote_window_size		(CALC_MET + 4970)
#define SPXCON_con_remote_window_size_TITLE	gettxt("RTPM:944", "spxcon_con_remote_window_size")
#define SPXCON_con_remote_window_size_IDX 	METSTART + 681

	/* Current transmit packet size. */
#define SPXCON_con_send_packet_size		(CALC_MET + 4980)
#define SPXCON_con_send_packet_size_TITLE	gettxt("RTPM:945", "spxcon_con_send_packet_size")
#define SPXCON_con_send_packet_size_IDX 	METSTART + 682

	/* Current receive packet size. */
#define SPXCON_con_rcv_packet_size		(CALC_MET + 4990)
#define SPXCON_con_rcv_packet_size_TITLE	gettxt("RTPM:946", "spxcon_con_rcv_packet_size")
#define SPXCON_con_rcv_packet_size_IDX 	METSTART + 683

	/* Milliseconds was last round trip time. */
#define SPXCON_con_round_trip_time		(CALC_MET + 5000)
#define SPXCON_con_round_trip_time_TITLE	gettxt("RTPM:947", "spxcon_con_round_trip_time")
#define SPXCON_con_round_trip_time_IDX 	METSTART + 684

	/* Times transmit window was closed. */
#define SPXCON_con_window_choke		(CALC_MET + 5010)
#define SPXCON_con_window_choke_TITLE	gettxt("RTPM:948", "spxcon_con_window_choke")
#define SPXCON_con_window_choke_IDX 	METSTART + 685

	/* Messages sent to SPX from application. */
#define SPXCON_con_send_mesg_count		(CALC_MET + 5020)
#define SPXCONR_con_send_mesg_count_TITLE	gettxt("RTPM:949", "spxcon_con_send_mesg_count/s")
#define SPXCONR_con_send_mesg_count_IDX 	METSTART + 686

	/* Unknown messages sent to SPX from application. */
#define SPXCON_con_unknown_mesg_count		(CALC_MET + 5030)
#define SPXCON_con_unknown_mesg_count_TITLE	gettxt("RTPM:950", "spxcon_con_unknown_mesg_count")
#define SPXCON_con_unknown_mesg_count_IDX 	METSTART + 687
#define SPXCONR_con_unknown_mesg_count_TITLE	gettxt("RTPM:951", "spxcon_con_unknown_mesg_count/s")
#define SPXCONR_con_unknown_mesg_count_IDX 	METSTART + 688

	/* Bad messages sent to SPX from application. */
#define SPXCON_con_send_bad_mesg		(CALC_MET + 5040)
#define SPXCON_con_send_bad_mesg_TITLE	gettxt("RTPM:952", "spxcon_con_send_bad_mesg")
#define SPXCON_con_send_bad_mesg_IDX 	METSTART + 689
#define SPXCONR_con_send_bad_mesg_TITLE	gettxt("RTPM:953", "spxcon_con_send_bad_mesg/s")
#define SPXCONR_con_send_bad_mesg_IDX 	METSTART + 690

	/* Packets sent to IPX from SPX. */
#define SPXCON_con_send_packet_count		(CALC_MET + 5050)
#define SPXCONR_con_send_packet_count_TITLE	gettxt("RTPM:954", "spxcon_con_send_packet_count/s")
#define SPXCONR_con_send_packet_count_IDX 	METSTART + 691

	/* Packets Re-sent to IPX due to timeout. */
#define SPXCON_con_send_packet_timeout		(CALC_MET + 5060)
#define SPXCON_con_send_packet_timeout_TITLE	gettxt("RTPM:955", "spxcon_con_send_packet_timeout")
#define SPXCON_con_send_packet_timeout_IDX 	METSTART + 692
#define SPXCONR_con_send_packet_timeout_TITLE	gettxt("RTPM:956", "spxcon_con_send_packet_timeout/s")
#define SPXCONR_con_send_packet_timeout_IDX 	METSTART + 693

	/* Packets Re-sent to IPX due to NAK received. */
#define SPXCON_con_send_packet_nak		(CALC_MET + 5070)
#define SPXCON_con_send_packet_nak_TITLE	gettxt("RTPM:957", "spxcon_con_send_packet_nak")
#define SPXCON_con_send_packet_nak_IDX 	METSTART + 694
#define SPXCONR_con_send_packet_nak_TITLE	gettxt("RTPM:958", "spxcon_con_send_packet_nak/s")
#define SPXCONR_con_send_packet_nak_IDX 	METSTART + 695

	/* ACK packets sent to IPX. */
#define SPXCON_con_send_ack		(CALC_MET + 5080)
#define SPXCONR_con_send_ack_TITLE	gettxt("RTPM:959", "spxcon_con_send_ack/s")
#define SPXCONR_con_send_ack_IDX 	METSTART + 696

	/* NAK packets sent to IPX. */
#define SPXCON_con_send_nak		(CALC_MET + 5090)
#define SPXCON_con_send_nak_TITLE	gettxt("RTPM:960", "spxcon_con_send_nak")
#define SPXCON_con_send_nak_IDX 	METSTART + 697
#define SPXCONR_con_send_nak_TITLE	gettxt("RTPM:961", "spxcon_con_send_nak/s")
#define SPXCONR_con_send_nak_IDX 	METSTART + 698

	/* Watchdog packets sent to IPX. */
#define SPXCON_con_send_watchdog		(CALC_MET + 5100)
#define SPXCONR_con_send_watchdog_TITLE	gettxt("RTPM:962", "spxcon_con_send_watchdog/s")
#define SPXCONR_con_send_watchdog_IDX 	METSTART + 699

	/* SPX packets received from IPX. */
#define SPXCON_con_rcv_packet_count		(CALC_MET + 5110)
#define SPXCONR_con_rcv_packet_count_TITLE	gettxt("RTPM:963", "spxcon_con_rcv_packet_count/s")
#define SPXCONR_con_rcv_packet_count_IDX 	METSTART + 700

	/* Bad SPX packets received from IPX. */
#define SPXCON_con_rcv_bad_packet		(CALC_MET + 5120)
#define SPXCON_con_rcv_bad_packet_TITLE	gettxt("RTPM:964", "spxcon_con_rcv_bad_packet")
#define SPXCON_con_rcv_bad_packet_IDX 	METSTART + 701
#define SPXCONR_con_rcv_bad_packet_TITLE	gettxt("RTPM:965", "spxcon_con_rcv_bad_packet/s")
#define SPXCONR_con_rcv_bad_packet_IDX 	METSTART + 702

	/* Bad SPX DATA packets received from IPX. */
#define SPXCON_con_rcv_bad_data_packet		(CALC_MET + 5130)
#define SPXCON_con_rcv_bad_data_packet_TITLE	gettxt("RTPM:966", "spxcon_con_rcv_bad_data_packet")
#define SPXCON_con_rcv_bad_data_packet_IDX 	METSTART + 703
#define SPXCONR_con_rcv_bad_data_packet_TITLE	gettxt("RTPM:967", "spxcon_con_rcv_bad_data_packet/s")
#define SPXCONR_con_rcv_bad_data_packet_IDX 	METSTART + 704

	/* Duplicate SPX packets received. */
#define SPXCON_con_rcv_dup_packet		(CALC_MET + 5140)
#define SPXCON_con_rcv_dup_packet_TITLE	gettxt("RTPM:968", "spxcon_con_rcv_dup_packet")
#define SPXCON_con_rcv_dup_packet_IDX 	METSTART + 705
#define SPXCONR_con_rcv_dup_packet_TITLE	gettxt("RTPM:969", "spxcon_con_rcv_dup_packet/s")
#define SPXCONR_con_rcv_dup_packet_IDX 	METSTART + 706

	/* Out of sequence packets received. */
#define SPXCON_con_rcv_packet_outseq		(CALC_MET + 5150)
#define SPXCON_con_rcv_packet_outseq_TITLE	gettxt("RTPM:970", "spxcon_con_rcv_packet_outseq")
#define SPXCON_con_rcv_packet_outseq_IDX 	METSTART + 707
#define SPXCONR_con_rcv_packet_outseq_TITLE	gettxt("RTPM:971", "spxcon_con_rcv_packet_outseq/s")
#define SPXCONR_con_rcv_packet_outseq_IDX 	METSTART + 708

	/* SPX packets sent up to application. */
#define SPXCON_con_rcv_packet_sentup		(CALC_MET + 5160)
#define SPXCONR_con_rcv_packet_sentup_TITLE	gettxt("RTPM:972", "spxcon_con_rcv_packet_sentup/s")
#define SPXCONR_con_rcv_packet_sentup_IDX 	METSTART + 709

	/* Packets queued due to flow control upstream. */
#define SPXCON_con_rcv_packet_qued		(CALC_MET + 5170)
#define SPXCONR_con_rcv_packet_qued_TITLE	gettxt("RTPM:973", "spxcon_con_rcv_packet_qued/s")
#define SPXCONR_con_rcv_packet_qued_IDX 	METSTART + 710

	/* SPX ACKs received from IPX. */
#define SPXCON_con_rcv_ack		(CALC_MET + 5180)
#define SPXCONR_con_rcv_ack_TITLE	gettxt("RTPM:974", "spxcon_con_rcv_ack/s")
#define SPXCONR_con_rcv_ack_IDX 	METSTART + 711

	/* SPX NAKs received from IPX. */
#define SPXCON_con_rcv_nak		(CALC_MET + 5190)
#define SPXCON_con_rcv_nak_TITLE	gettxt("RTPM:975", "spxcon_con_rcv_nak")
#define SPXCON_con_rcv_nak_IDX 	METSTART + 712
#define SPXCONR_con_rcv_nak_TITLE	gettxt("RTPM:976", "spxcon_con_rcv_nak/s")
#define SPXCONR_con_rcv_nak_IDX 	METSTART + 713

	/* Watchdog packets received from IPX. */
#define SPXCON_con_rcv_watchdog		(CALC_MET + 5200)
#define SPXCONR_con_rcv_watchdog_TITLE	gettxt("RTPM:977", "spxcon_con_rcv_watchdog/s")
#define SPXCONR_con_rcv_watchdog_IDX 	METSTART + 714

	/* Total SAP activity */
#define SAP_total		(CALC_MET + 5210)
#define SAPR_total_TITLE	gettxt("RTPM:990", "sap_total/s")
#define SAPR_total_IDX 		METSTART + 715

	/* Total SPX activity */
#define SPX_total		(CALC_MET + 5220)
#define SPXR_total_TITLE	gettxt("RTPM:991", "spx_total/s")
#define SPXR_total_IDX 		METSTART + 716

	/* Total IPX activity */
#define IPX_total		(CALC_MET + 5230)
#define IPXR_total_TITLE	gettxt("RTPM:992", "ipx_total/s")
#define IPXR_total_IDX 		METSTART + 717

	/* Total RIP activity */
#define RIP_total		(CALC_MET + 5240)
#define RIPR_total_TITLE	gettxt("RTPM:993", "rip_total/s")
#define RIPR_total_IDX 		METSTART + 718

	/* Total netware errors */

#define NETWARE_errs		(CALC_MET + 5250)
#define NETWARE_errs_TITLE	gettxt("RTPM:994", "netware_errs")
#define NETWARE_errs_IDX 	METSTART + 719
#define NETWARER_errs_TITLE	gettxt("RTPM:995", "netware_err/s")
#define NETWARER_errs_IDX 	METSTART + 720

	/* Maximum simultaneous SPX connections. */
#define SPX_max_used_connections		(CALC_MET + 2970)
#define SPX_max_used_connections_TITLE	gettxt("RTPM:670", "spx_max_used_connections")
#define SPX_max_used_connections_IDX 	METSTART + 721

	/* Bad IPX address size, packet dropped */
#define IPXSOCK_IpxInBadSize		(CALC_MET + 5260)
#define IPXSOCK_IpxInBadSize_TITLE	gettxt("RTPM:1127", "ipxsock_IpxInBadSize")
#define IPXSOCK_IpxInBadSize_IDX 	METSTART + 722
#define IPXSOCKR_IpxInBadSize_TITLE	gettxt("RTPM:1128", "ipxsock_InBadSize/s")
#define IPXSOCKR_IpxInBadSize_IDX 	METSTART + 723

	/* eisa bus util - not on all platforms	*/	
#define EISA_BUS_UTIL_SUMCNT	(CALC_MET + 5270)
#define EISA_BUS_UTIL_SUMCNT_TITLE	gettxt("RTPM:1136", "eisa_cnt")
#define EISA_BUS_UTIL_SUMCNT_IDX	(METSTART + 724)
#define EISA_BUS_UTIL_PERCENT	(CALC_MET + 5280)
#define EISA_BUS_UTIL_PERCENT_TITLE	gettxt("RTPM:1135", "%%eisa")
#define EISA_BUS_UTIL_PERCENT_IDX	(METSTART + 725)

#define LBOLT	(CALC_MET + 5290)
#define LBOLT_TITLE	gettxt("RTPM:611", "lbolt")
#define LBOLT_IDX	(METSTART + 726)
/*
 * functions defined in mtbl.c
 */
void metalloc( struct metric *metp );
void check_resource( struct metric *metp );
void set_size( struct metric *metp );
void *histalloc( size_t sz );
