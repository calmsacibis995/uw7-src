#ident	"@(#)pcintf:bridge/p_server.c	1.1.1.2"
#include	"sccs.h"
SCCSID(@(#)p_server.c	6.7	LCC);	/* Modified: 12:03:23 12/3/91 */

/***********************************************************************

	Copyright (c) 1984, 1987 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

***********************************************************************/

#include "sysconfig.h"

#include <errno.h>
#include <memory.h>
#include <string.h>

#if defined(RLOCK)
#	include <rlock.h>
#	include <stdio.h>
#endif   /* RLOCK */

#include "pci_types.h"
#include "common.h"
#include "dossvr.h"
#include "flip.h"
#include "log.h"
#include "xdir.h"

#ifdef FAST_LSEEK
#include "version.h"
#endif	/* FAST_LSEEK */

#ifdef 	XENIX 	/* to avoid hard to find run-time barfs */
#define sio 	output
#endif	/* XENIX */

#ifdef	JANUS
#include	<janus.h>
#endif	/* JANUS */

/*  - - - - - - - - - - - - - - - External Routines - - - - - - - - - - */

#ifdef	JANUS
extern	int	fake_open();
extern	void	fake_lseek();
extern	void	fake_read();
extern	void	fake_close();
#endif /* JANUS */


/*  - - - - - - - - - - External Variables - - - - - - - - - - - - - -  */

extern unsigned char	err_code;	/* Saved DOS error code */
extern unsigned char	brg_seqnum;	/* Sequence number of last frame */
extern  int swap_how;           /* How to swap output packets */

extern  unsigned int entries;   /* Number of simultaneous search contexts */
extern  struct ni2 ndata;       /* Ethernet header */
#ifdef	JANUS
extern int  is_auto_exec;	/* running autoexec.bat or not */
extern char auto_exec_file[];	/* Autoexec file name to match */
extern unsigned int vmr_addr;	/* Base address of the VM86 memory. */
#endif	/* JANUS */

/*  - - - - - - - - - - - - Global Structures - - - - - - - - - - - - - */

unsigned char	request;                /* PCI request type */
int     outputframelength;      /* Length of last output frame */

struct output    *optr;         /* Pointer to the last frame sent */

int	print_desc[NPRINT];	/* Print file descriptors */
char	*print_name[NPRINT];	/* Filename of print spool files */

struct  output  *addr;          /* Points to general output buffer */
struct  output  out1;           /* Output frame buffers */
#ifdef	JANUS
static int  fakeautoexec = -1;	/* Fake file descriptor for autoexec */
#endif /* JANUS */


/*
 * server()
 */

void
server(in)
    register struct input *in;  /* Pointer to input buffer */
{
    register int stringlen;     /* Temporarily contains string length */
    register char *intext;	/* Pointer to the text in the input struct */
    int doStop = 0;		/* Stop after reply to current request */
    long tmpLong;		/* temp for a little byte flipping */
    int textcount;

#ifdef	JANUS
	/* With JANUS there is not need to test for or do retransmit. */
#else	/* not JANUS */
	/* Resend last message? */
    if (brg_seqnum == in->hdr.seq) {
	reXmt(optr, outputframelength);
	return;
    }
#endif /* not JANUS */

/* First clear output header; we will fill in as needed later */
    (void) memset(&out1.hdr, 0, sizeof(struct header));

/*
 * DOS side depends on always seeing a meaningful file descriptor
 * (either the same one passed in or a new one (from open, e.g.)
 */
    out1.hdr.fdsc = in->hdr.fdsc;

/* Record sequence number and request type  */
    request    = in->hdr.req;
    brg_seqnum = in->hdr.seq;

/* Flush error code from previous request(s) */
    errno = 0; 			

/*
 * Process the request specified by the "req" and "stat" fields of the header.
 */

	addr = &out1;
	intext = in->text;

	switch (in->hdr.stat) {
	case 1:			/* New code for NEW * nkp 5/14/85 */
	case NEW:
	    switch (request) {
	
#ifdef 	JANUS
	    case GET_INIT_DATA:
		Vlog(("get init data: %s mode %#x pid %#x attr %#x\n",
		    "39: %s %#x %#x %#x\n",
		    intext,in->hdr.mode,in->hdr.pid, 0x0ff & in->hdr.attr));
		get_init_data(in, addr);
		break;
#endif  /* JANUS */
	
	    case DEVICE_INFO_C:
		Vlog(("deviceinfo: fdsc %#x\n","40: %#x\n",in->hdr.fdsc));
		pci_devinfo(in->hdr.fdsc, addr);
		break;
	
	    case NEW_OPEN:
	    case OLD_OPEN:
		Vlog(("%s: %s mode %#x pid %#x attr %#x\n",
		    "%s: %s %#x %#x %#x\n",
		    (request == NEW_OPEN) ?
			(dbgCheck(DBG_VLOG) ? "new_open":"18") :
			(dbgCheck(DBG_VLOG) ? "old_open":" 4"),
		    intext,in->hdr.mode,in->hdr.pid, 0x0ff & in->hdr.attr));
#ifdef	JANUS
		if (is_auto_exec && (strcmp(intext, auto_exec_file) == 0)) {
			fakeautoexec = fake_open(addr);
		} else
#endif /* JANUS */
		{
			pci_open(intext, in->hdr.mode, in->hdr.attr,
				in->hdr.pid, addr, in->hdr.req);
		}
		break;
	
	    case PCI_CREATE:
		if (in->hdr.t_cnt == 0) *intext = '\0';
		Vlog(("create: %s pid %#x mode %#x attr %#x\n",
		    " 2: %s %#x %#x %#x\n",
		    intext,in->hdr.pid, in->hdr.mode, in->hdr.attr));
		pci_create(intext, in->hdr.mode, in->hdr.attr, in->hdr.pid,
			addr, in->hdr.req);
		break;
	
	    case READ_RAN:
		/* This is the DOS old style read rand or read seq */
		Vlog(("ran_read_new:fdsc %#x cnt %#x off %lx addr %lx\n",
		    " 7: %#x %#x %lx %lx\n",
		    in->hdr.fdsc, in->hdr.b_cnt,in->hdr.offset,in->hdr.f_size));
		if (pciran_read(in->hdr.fdsc, in->hdr.offset, in->hdr.mode,
		    in->hdr.inode, addr) == FALSE)
		    break;
#ifdef	JANUS
		if (is_auto_exec && (in->hdr.fdsc == fakeautoexec)) {
			fake_read(in->hdr.fdsc, in->hdr.b_cnt, in->hdr.offset,
				addr);
			break;
		}
		addr = pciseq_read(in->hdr.fdsc, in->hdr.b_cnt,
			(char *)(rtolin(in->hdr.f_size)+vmr_addr));
#else	/* not JANUS */
		addr = pciseq_read(in->hdr.fdsc, in->hdr.b_cnt);
#endif	/* not JANUS */
		if (addr == NULL)
		    return;
		break;
	
	    case READ_SEQ:
		/* This is the DOS new style read */
		Vlog(("read_new: fdsc %#x cnt %#x off %lx addr %lx\n",
		    " 6: %#x %#x %lx %lx\n",
		    in->hdr.fdsc, in->hdr.b_cnt,in->hdr.offset,in->hdr.f_size));
#ifdef FAST_LSEEK

		/*
		 * If offset is -1, don't do a FAST_LSEEK because this file
		 * is for print spooling.
		 */

		if ((bridge_ver_flags & V_FAST_LSEEK) &&
			in->hdr.offset != (long)(-1))
		{
#ifdef	JANUS
			if (in->hdr.fdsc == fakeautoexec) {
				fake_lseek(in->hdr.fdsc, in->hdr.offset, addr);
			} else
#endif	/* JANUS */
			{
				pci_lseek(in->hdr.fdsc, in->hdr.offset, 0,
					in->hdr.inode, addr, in->hdr.req);
			}
		}
#endif	/* FAST_LSEEK */

#ifdef	JANUS
		if (is_auto_exec && (in->hdr.fdsc == fakeautoexec)) {
			fake_read(in->hdr.fdsc, in->hdr.b_cnt, in->hdr.offset,
				addr);
			break;
		}
		addr = pciseq_read(in->hdr.fdsc, in->hdr.b_cnt,
			(char *)(rtolin(in->hdr.f_size)+vmr_addr));
#else	/* not JANUS */
		addr = pciseq_read(in->hdr.fdsc, in->hdr.b_cnt);
#endif	/* not JANUS */
		if (addr == NULL)
			return;
		break;
	
	    case WRITE_RAN:
		/* This is the DOS old style write rand or write seq */
	       Vlog(("ran_write_new:fdsc %#x cnt %#x off %lx addr %lx\n",
		    " 9: %#x %#x %lx %lx\n",
		    in->hdr.fdsc, in->hdr.t_cnt,in->hdr.offset,in->hdr.f_size));
		if (pci_ran_write(in->hdr.fdsc, in->hdr.offset, in->hdr.inode,
		    in->hdr.mode, addr, in->hdr.req) == FALSE)
		    break;
#ifndef	JANUS
		textcount = in->hdr.t_cnt;
#else	/* JANUS */
		intext = (char *)(rtolin(in->hdr.f_size)+vmr_addr);
		textcount = in->hdr.b_cnt;
#endif	/* JANUS */
		pci_seq_write(in->hdr.fdsc, intext, textcount,
			in->hdr.stat, in->hdr.offset, addr, in->hdr.req);
		break;
	
	    case WRITE_SEQ:
		/* This is the DOS new style write rand */
		Vlog(("write: fdsc %#x bcnt %d tcnt %d off %lx addr %lx\n",
		    " 8: %#x %#x %#x %lx %lx\n",
		    in->hdr.fdsc, in->hdr.b_cnt, in->hdr.t_cnt,in->hdr.offset,
		    in->hdr.f_size));
#ifdef FAST_LSEEK
		if ((bridge_ver_flags & V_FAST_LSEEK) &&
			in->hdr.offset != (long)(-1))
		{
			pci_lseek(in->hdr.fdsc, in->hdr.offset, 0,
				in->hdr.inode, addr, in->hdr.req);
		}
#endif	/* FAST_LSEEK */
#ifndef	JANUS
		textcount = in->hdr.t_cnt;
#else	/* JANUS */
		intext = (char *)(rtolin(in->hdr.f_size)+vmr_addr);
		textcount = in->hdr.b_cnt;
#endif	/* JANUS */
		pci_seq_write(in->hdr.fdsc, intext, textcount,
			in->hdr.stat, in->hdr.offset, addr, in->hdr.req);
		break;
	
	    case NEW_CLOSE:
	    case OLD_CLOSE:
		Vlog(("%s: fdsc %#x size %lx\n", "%s: %#x %lx\n",
		    (request == NEW_CLOSE) ?
			(dbgCheck(DBG_VLOG) ? "new_close":" 5") :
			(dbgCheck(DBG_VLOG) ? "old_close":"33"),
		    in->hdr.fdsc, in->hdr.f_size));
#ifdef	JANUS
		if (is_auto_exec && (in->hdr.fdsc == fakeautoexec)) {
			fake_close(in->hdr.fdsc, addr);
			is_auto_exec--;
			fakeautoexec = -1;
		} else
#endif	/* JANUS */
		{
			pci_close(in->hdr.fdsc, in->hdr.f_size, in->hdr.inode,
				in->hdr.mode, in->hdr.t_cnt, intext, addr,
				in->hdr.req);
		}
		break;
	
	    case PCI_DELETE:
		Vlog(("delete: %s attr %#x\n", " 3: %s %#x\n",
		    intext, 0x00ff & in->hdr.attr));
		pci_delete(intext, in->hdr.attr, in->hdr.pid,
		    addr, in->hdr.req);
		break;
	
	    case CHDIR:
		Vlog(("chdir: %s\n", "10: %s\n", intext));
		pci_chdir(intext, addr, in->hdr.req);
		break;
	
	    case MKDIR:
		Vlog(("mkdir: %s\n", "11: %s\n", intext));
		pci_mkdir(intext, addr, in->hdr.req);
		break;
	
	    case RMDIR:
		Vlog(("rmdir: %s\n", "12: %s\n", intext));
		pci_rmdir(intext, addr, in->hdr.req);
		break;
	
	    case GETWD:
		Vlog(("getwd:\n", "25:\n"));
		pci_pwd(in->hdr.mode, addr);
		break;
	
	    case RENAME:
	    case RENAME_NEW:
		stringlen = strlen(intext);
		Vlog(("rename: %s to %s %d \n", "14: %s %s %d \n",
		    intext, intext + stringlen + 1, in->hdr.mode));
		pci_rename(intext, intext + stringlen + 1, in->hdr.req,
		    in->hdr.mode, addr, in->hdr.pid);
		break;
	
	    case SEARCH:
	    case FIND_FIRST:
		Vlog(("search: %s mode %#x attr %#x pid %#x\n",
		    "15: %s %#x %#x %#x\n",
		    intext, in->hdr.mode, 0x0ff & in->hdr.attr, in->hdr.pid));
		if ((addr = (struct output *)pci_sfirst(intext,in->hdr.req,
		    in->hdr.mode, in->hdr.attr, in->hdr.pid
		   )) == NULL)
		    return;                 /* Pipeline output processing */
		else
		    break;
	
	    case NEXT_FIND:
	    case NEXT_SEARCH:
		Vlog(
      ("search/find next: ptrn %s id %#x off %lx mode %#x attr %#x pid %#x\n",
		    "16: %s %#x %lx %#x %#x %#x\n",
		    intext, in->hdr.fdsc, in->hdr.offset, in->hdr.mode,
		    0x00ff & in->hdr.attr, in->hdr.pid));
		if ((addr = (struct output *)pci_snext(intext,
		    in->hdr.t_cnt, in->hdr.req, in->hdr.fdsc, in->hdr.offset,
		    in->hdr.attr
		   )) == NULL)
		    return;
		else
		    break;
	
	    case FILE_SIZE:
		Vlog(("file_size: of %s rec size %#x\n", "21:%s %#x\n",
		    intext, in->hdr.b_cnt));
		pci_fsize(intext, in->hdr.b_cnt, addr, in->hdr.req);
		break;
	
	    case CHMOD:
		Vlog(("chmod:mode=%d. attr=%#x file=%s\n", "13: %#x %#x %s\n",
		    in->hdr.mode, in->hdr.attr, intext));
		pci_chmod(in, addr);
		break;
	
	    case SET_STATUS:
		Vlog(("set_status: file %s mode 0%o\n", "20: %s %o\n",
		    intext, in->hdr.mode));
		pci_setstatus(intext, in->hdr.mode, addr, in->hdr.req);
		break;
	
	    case EXIT:
		Vlog(("Exit on process id %#x mode %d.:\n", " 1: %#x %d.\n",
		    in->hdr.pid,in->hdr.mode));
		pci_exit(in->hdr.pid, addr);
#ifdef	JANUS
		track_exit(in->hdr.mode);
#endif	/* JANUS */
		break;
	
	    case FS_STATUS:
		Vlog(("fs_status:\n", "19:\n"));
		pci_fstatus( addr, in->hdr.req);
		break;
	
	    case L_SEEK:
		Vlog(("lseek: fdsc %#x off %lx whence %#x\n",
		    "22: %#x %lx %#x\n",
		    in->hdr.fdsc, in->hdr.offset, in->hdr.mode));
#ifdef	JANUS
		if (in->hdr.fdsc == fakeautoexec) {
			fake_lseek(in->hdr.fdsc, in->hdr.offset,addr);
		} else
#endif	/* JANUS */
		{
			pci_lseek(in->hdr.fdsc, in->hdr.offset, in->hdr.mode,
				in->hdr.inode, addr, in->hdr.req);
		}
		break;
	
	    case TIME_DATE:
		Vlog(("timeDate: fdsc %#x\n", "17: %#x\n", in->hdr.fdsc));
		pci_timedate(in->hdr.fdsc, in->hdr.t_cnt, in->hdr.time,
		    in->hdr.date, addr);
		break;
	
	    case LOCK_MODE:
		Vlog(("lockMode: %d\n", "24: %d\n", in->hdr.mode));

/* commented out...  
		vfLockMode(in->hdr.mode);
		*/
		addr->hdr.res = SUCCESS;
		break;
	
	    case DISCONNECT:
		Vlog(("disconnect\n", "32\n"));
		doStop = 1;
		break;
	
	    case LOCK:
		Vlog(("lock: %#x %d. %lx %lx\n", "41: %#x %d. %lx %lx\n",
		  in->hdr.fdsc, in->hdr.mode, in->hdr.offset, in->hdr.f_size));
		log("LOCK: fdsc: %d  mode: %d  offset: %ld  size: %ld\n",
		  in->hdr.fdsc, in->hdr.mode, in->hdr.offset, in->hdr.f_size);
		pci_lock(in->hdr.fdsc,in->hdr.mode,
#ifdef RLOCK  /* record locking */
			(unsigned short)(in->hdr.pid), 
#endif  /* RLOCK */
		in->hdr.offset, in->hdr.f_size, addr, in->hdr.req);
		break;

	    case GET_EXT_ERROR:
		Vlog(("get extended error: %#x\n","42: %#x\n", in->hdr.fdsc));
		pci_get_ext_err(addr);
		break;


	    case SET_SDEBUG:
		Vlog(("set debug: mode %d\n","44: %d\n", in->hdr.mode));
		lflipm(((struct dbg_struct*)intext)->change,tmpLong,swap_how);
		lflipm(((struct dbg_struct*)intext)->set, tmpLong, swap_how);
		lflipm(((struct dbg_struct*)intext)->on, tmpLong, swap_how);
		lflipm(((struct dbg_struct*)intext)->off, tmpLong, swap_how);
		lflipm(((struct dbg_struct*)intext)->flip, tmpLong, swap_how);
		newLogs(DOSSVR_LOG, getpid(), (long *)NULL,
						(struct dbg_struct *)intext);
		dump_dir();		/* Dump all existing dir contexts */
		dump_file();		/* Dump all open file contexts */
		addr->hdr.res = 0;
		break;

	    case LOG_CTRL:
		Vlog(("logging control: mode %d\n","44: %d\n", in->hdr.mode));
		logControl(in->hdr.mode);
		addr->hdr.res = 0;
		break;

	    case LOG_MSG:
		Vlog(("remote log: ", "45: "));
		logMessage((unsigned char *)intext, in->hdr.t_cnt);
		addr->hdr.res = 0;
		break;

	    case UEXEC:
		Vlog(("uexec: cmd %s ** msize %d flag %d asize %d evo %d\n",
		    "46: ",
		    intext, in->hdr.t_cnt, in->hdr.mode, in->hdr.f_size,
		    in->hdr.offset));
		addr->hdr.f_size = uexec(intext, 3, (int) in->hdr.t_cnt,
			(int) in->hdr.f_size, (int) in->hdr.offset,
			(int) in->hdr.mode);
		if(!addr->hdr.f_size) addr->hdr.res = EINVAL;
		else addr->hdr.res = 0;
		break;

            case IPC:
		Vlog(("IPC: ", "53: "));
#ifdef NOIPC
		goto invalid;
#else 
	        switch(in->hdr.mode)
	        {
                case X_MSGGET:
			p_msgget( intext , addr );
			break;
                case X_MSGSND:
			p_msgsnd( intext , addr );
			break;
                case X_MSGRCV:
			p_msgrcv( intext , addr );
			break;
                case X_MSGCTL:
			p_msgctl( intext , addr );
			break;
                case X_SEMGET:
			p_semget( intext , addr );
			break;
                case X_SEMOP:
			p_semop( intext , addr );
			break;
                case X_SEMCTL:
			p_semctl( intext , addr );
			break;
                default:
		    log("IPC-invalid subfunction %d\n",in->hdr.mode);
                    goto invalid;
                }
#endif /* NOIPC */
                break;	

	    case UWAIT:
		Vlog(("uwait: ", "47: "));
		uwait(in->hdr.mode, addr);
		break;
		
	    case UKILL:	
		Vlog(("ukill: ", "48: "));
		pci_ukill(in->hdr.f_size, in->hdr.fdsc, addr);
		break;
		
#ifdef	USIGNAL
	    case USIGNAL:
		break;
#endif	/* USIGNAL */

	    case CONSOLE_READ:
		Vlog(("console read:fdsc %#x dosfunc %#x flush %#x maxc %d\n",
		    "43: %#x %#x %#x %#x\n",
		    in->hdr.fdsc, in->hdr.mode, 0x00ff & in->hdr.attr,
		    in->hdr.b_cnt));
		pci_conread( in->hdr.fdsc, in->hdr.mode, in->hdr.attr,
		    in->hdr.b_cnt, addr, in->hdr.req);
		break;

	    case MAPNAME:
		Vlog(("mapname: in: %s  mode: %d", "51: %s %d",
		       intext, in->hdr.mode));
		pci_mapname(intext, in->hdr.mode, addr);   
		break;
		
	    case GET_SITE_ATTR:
		Vlog(("get version string: \"%s\" pid %#x\n",
					"64: %s %#x\n", intext, in->hdr.pid));
		get_version_string(intext, in->hdr.t_cnt, addr);
		break;

	    case DOS_NLS_INFO:
		dos_nls_info((unsigned short *)intext, in->hdr.t_cnt,
							in->hdr.b_cnt, addr);
		break;

	    default:
		goto invalid;
	    }
	    break;
	/*endcase stat==NEW*/

	case ACK:
	    switch (request) {
	    case READ_RAN:
	    case READ_SEQ:
		Vlog(("read_mtf(%s)\n", "%s:\n",
		    (request == READ_RAN) ?
			(dbgCheck(DBG_VLOG) ? "ran":" 7") :
			(dbgCheck(DBG_VLOG) ? "seq":" 6") ));
		if ((addr = pci_ack_read(in->hdr.fdsc)) == NULL)
		    return;
		break;
	
	    default:
		goto invalid;
	    }
	    break;

	case 2:			/* New code for NEW_MTF * nkp 5/14/85 */
	case NEW_MTF:
	    switch (request) {
	    case WRITE_RAN:
		Vlog(("ran_write_new: fdsc %#x cnt %#x off %lx\n",
		    " 9: %#x %#x %lx\n",
		    in->hdr.fdsc, in->hdr.b_cnt, in->hdr.offset));
		if (pci_ran_write(in->hdr.fdsc, in->hdr.offset, in->hdr.inode,
		    in->hdr.mode, addr, in->hdr.req) == FALSE)
		    break;
		pci_seq_write(in->hdr.fdsc, intext, in->hdr.t_cnt,
		    in->hdr.stat, in->hdr.offset, addr, in->hdr.req);
		break;
	
	    case WRITE_SEQ:
		Vlog(("write_st: fdsc %#x bcnt %#x tcnt %#x off %lx\n",
		    " 8: %#x %#x %#x %lx\n",
		    in->hdr.fdsc, in->hdr.b_cnt, in->hdr.t_cnt,in->hdr.offset));
#ifdef FAST_LSEEK
		if ((bridge_ver_flags & V_FAST_LSEEK) && in->hdr.offset!=(long)-1)
			pci_lseek(in->hdr.fdsc, in->hdr.offset, 0, in->hdr.inode,
				addr, in->hdr.req);
#endif	/* FAST_LSEEK */
		pci_seq_write(in->hdr.fdsc, intext, in->hdr.t_cnt,
		    in->hdr.stat, in->hdr.offset, addr, in->hdr.req);
		break;
	
	    case UEXEC:
		Vlog(("uexec_mtf\n", "46: "));
		addr->hdr.f_size = uexec(intext, 1, in->hdr.t_cnt,
			in->hdr.f_size, in->hdr.offset, in->hdr.mode);
		if(!addr->hdr.f_size) addr->hdr.res = EINVAL;
		else addr->hdr.res = 0;
		break;

            case IPC:
		Vlog(("ipc_new: ", "53: "));
#ifdef NOIPC
		goto invalid;
#else 
	        if (in->hdr.mode == X_MSGSND)
			p_msgsnd_new(intext, in->hdr.t_cnt, addr);
                else {
		    log("IPC-invalid subfunction %d\n",in->hdr.mode);
                    goto invalid;
                }
#endif /* NOIPC */
                break;	

	    default:
		goto invalid;
	    }
	    break;

	case 3:			/* New code for EXT_MTF * nkp 5/14/85 */
	case EXT_MTF:
	    switch (request) {
	    case WRITE_RAN:
	    case WRITE_SEQ:
		Vlog(("write_mtf: fdsc %#x bcnt %#x tcnt %#x off %lx\n",
		    " 8: %#x %#x %#x %lx\n",
		    in->hdr.fdsc, in->hdr.b_cnt, in->hdr.t_cnt,in->hdr.offset));
		pci_mid_write(in->hdr.fdsc, intext, in->hdr.t_cnt,
				addr, in->hdr.req);
		break;
	
	    case UEXEC:
		Vlog(("uexec_mtf\n", "46: "));
		addr->hdr.f_size = uexec(intext, 0, in->hdr.t_cnt,
			in->hdr.f_size, in->hdr.offset, in->hdr.mode);
		if(!addr->hdr.f_size) addr->hdr.res = EINVAL;
		else addr->hdr.res = 0;
		break;

            case IPC:
		Vlog(("ipc_ext: ", "53: "));
#ifdef NOIPC
		goto invalid;
#else 
	        if (in->hdr.mode == X_MSGSND)
			p_msgsnd_ext(intext, in->hdr.t_cnt, addr);
                else {
		    log("IPC-invalid subfunction %d\n",in->hdr.mode);
                    goto invalid;
                }
#endif /* NOIPC */
                break;	

	    default:
		goto invalid;
	    }
	    break;

	case 4:			/* New code for EXT * nkp 5/14/85 */
	case EXT:
	    switch (request) {
	    case WRITE_RAN:
	    case WRITE_SEQ:
		Vlog(("write_end: fdsc %#x bcnt %#x tcnt %#x off %lx\n",
		    " 8: %#x %#x %#x %lx\n",
		    in->hdr.fdsc, in->hdr.b_cnt, in->hdr.t_cnt,in->hdr.offset));
		pci_end_write(in->hdr.fdsc, intext, in->hdr.t_cnt,
				addr, in->hdr.req);
		break;
	
	    case UEXEC:
		Vlog(("uexec_mtf\n", "46: "));
		addr->hdr.f_size = uexec(intext, 2, in->hdr.t_cnt,
			in->hdr.f_size, in->hdr.offset, in->hdr.mode);
		if(!addr->hdr.f_size) addr->hdr.res = EINVAL;
		else addr->hdr.res = 0;
		break;


            case IPC:
		Vlog(("ipc_end: ", "53: "));
#ifdef NOIPC
		goto invalid;
#else 
	        if (in->hdr.mode == X_MSGSND)
			p_msgsnd_end(intext, in->hdr.t_cnt, addr);
                else {
		    log("IPC-invalid subfunction %d\n",in->hdr.mode);
                    goto invalid;
                }
#endif /* NOIPC */
                break;	

	    default:
		goto invalid;
	    }
	    break;

	default:
invalid:
	    Vlog(("invalid function: %d\n", "I:%d\n", in->hdr.req));
	    addr->hdr.res = INVALID_FUNCTION;
	    break;
	}

    if (addr->hdr.res != SUCCESS)
	err_code = addr->hdr.res;	/* save for extended err handling */

    optr = addr; 		/* Pointer to last frame sent */
    addr->pre.select = BRIDGE;
    addr->hdr.req = request;
    addr->hdr.seq = brg_seqnum;

    outputframelength = xmtPacket(optr, &ndata, swap_how);

    if (doStop) {
	stopService(0,0);         /* exit code 0 is `normal' exit */
	/*NOTREACHED*/
    }
}

/*	find_printx finds the index into the printer tables (print_desc and
	print_name) corresponding to the supplied file descriptor.  If no
	entry is found, -1 is returned.	 Fdes of -1 is used to allocate a
	new slot.
*/
find_printx(fdes)
int fdes;

{
int i;

	for (i=NPRINT-1; i >= 0 ; i--)
		if (print_desc[i] == fdes) return i;
	return -1;
}
