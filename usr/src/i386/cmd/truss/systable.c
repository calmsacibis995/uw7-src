/*		copyright	"%c%" 	*/

#ident	"@(#)truss:i386/cmd/truss/systable.c	1.1.8.1"
#ident	"$Header$"

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/signal.h>
#include <sys/fault.h>
#include <sys/syscall.h>

#include "pcontrol.h"
#include "ramdata.h"
#include "systable.h"
#include "print.h"
#include "proto.h"


/* tables of information about system calls - read-only data */


static const char * const errcode[] = {	/* error code names */
	 NULL,			/*   0 */
	"EPERM",		/*   1 */
	"ENOENT",		/*   2 */
	"ESRCH",		/*   3 */
	"EINTR",		/*   4 */
	"EIO",			/*   5 */
	"ENXIO",		/*   6 */
	"E2BIG",		/*   7 */
	"ENOEXEC",		/*   8 */
	"EBADF",		/*   9 */
	"ECHILD",		/*  10 */
	"EAGAIN",		/*  11 */
	"ENOMEM",		/*  12 */
	"EACCES",		/*  13 */
	"EFAULT",		/*  14 */
	"ENOTBLK",		/*  15 */
	"EBUSY",		/*  16 */
	"EEXIST",		/*  17 */
	"EXDEV",		/*  18 */
	"ENODEV",		/*  19 */
	"ENOTDIR",		/*  20 */
	"EISDIR",		/*  21 */
	"EINVAL",		/*  22 */
	"ENFILE",		/*  23 */
	"EMFILE",		/*  24 */
	"ENOTTY",		/*  25 */
	"ETXTBSY",		/*  26 */
	"EFBIG",		/*  27 */
	"ENOSPC",		/*  28 */
	"ESPIPE",		/*  29 */
	"EROFS",		/*  30 */
	"EMLINK",		/*  31 */
	"EPIPE",		/*  32 */
	"EDOM",			/*  33 */
	"ERANGE",		/*  34 */
	"ENOMSG",		/*  35 */
	"EIDRM",		/*  36 */
	"ECHRNG",		/*  37 */
	"EL2NSYNC",		/*  38 */
	"EL3HLT",		/*  39 */
	"EL3RST",		/*  40 */
	"ELNRNG",		/*  41 */
	"EUNATCH",		/*  42 */
	"ENOCSI",		/*  43 */
	"EL2HLT",		/*  44 */
	"EDEADLK",		/*  45 */
	"ENOLCK",		/*  46 */
	 NULL,			/*  47 */
	 NULL,			/*  48 */
	 NULL,			/*  49 */
	"EBADE",		/*  50 */
	"EBADR",		/*  51 */
	"EXFULL",		/*  52 */
	"ENOANO",		/*  53 */
	"EBADRQC",		/*  54 */
	"EBADSLT",		/*  55 */
	"EDEADLOCK",		/*  56 */
	"EBFONT",		/*  57 */
	 NULL,			/*  58 */
	"ECLNRACE",		/*  59 */
	"ENOSTR",		/*  60 */
	"ENODATA",		/*  61 */
	"ETIME",		/*  62 */
	"ENOSR",		/*  63 */
	"ENONET",		/*  64 */
	"ENOPKG",		/*  65 */
	"EREMOTE",		/*  66 */
	"ENOLINK",		/*  67 */
	"EADV",			/*  68 */
	"ESRMNT",		/*  69 */
	"ECOMM",		/*  70 */
	"EPROTO",		/*  71 */
	 NULL,			/*  72 */
	 NULL,			/*  73 */
	"EMULTIHOP",		/*  74 */
	 NULL,			/*  75 */
	 NULL,			/*  76 */
	"EBADMSG",		/*  77 */
	"ENAMETOOLONG",		/*  78 */
	"EOVERFLOW",		/*  79 */
	"ENOTUNIQ",		/*  80 */
	"EBADFD",		/*  81 */
	"EREMCHG",		/*  82 */
	"ELIBACC",		/*  83 */
	"ELIBBAD",		/*  84 */
	"ELIBSCN",		/*  85 */
	"ELIBMAX",		/*  86 */
	"ELIBEXEC",		/*  87 */
	"EILSEQ",		/*  88 */
	"ENOSYS",		/*  89 */
	"ELOOP",		/*  90 */
	"ERESTART",		/*  91 */
	"ESTRPIPE",		/*  92 */
	"ENOTEMPTY",		/*  93 */
	"EUSERS",		/*  94 */
	"ENOTSOCK",		/*  95 */
	"EDESTADDRREQ",		/*  96 */
	"EMSGSIZE",		/*  97 */
	"EPROTOTYPE",		/*  98 */
	"ENOPROTOOPT",		/*  99 */
	 NULL,			/* 100 */
	 NULL,			/* 101 */
	 NULL,			/* 102 */
	 NULL,			/* 103 */
	 NULL,			/* 104 */
	 NULL,			/* 105 */
	 NULL,			/* 106 */
	 NULL,			/* 107 */
	 NULL,			/* 108 */
	 NULL,			/* 109 */
	 NULL,			/* 110 */
	 NULL,			/* 111 */
	 NULL,			/* 112 */
	 NULL,			/* 113 */
	 NULL,			/* 114 */
	 NULL,			/* 115 */
	 NULL,			/* 116 */
	 NULL,			/* 117 */
	 NULL,			/* 118 */
	 NULL,			/* 119 */
	"EPROTONOSUPPORT",	/* 120 */
	"ESOCKTNOSUPPORT",	/* 121 */
	"EOPNOTSUPP",		/* 122 */
	"EPFNOSUPPORT",		/* 123 */
	"EAFNOSUPPORT",		/* 124 */
	"EADDRINUSE",		/* 125 */
	"EADDRNOTAVAIL",	/* 126 */
	"ENETDOWN",		/* 127 */
	"ENETUNREACH",		/* 128 */
	"ENETRESET",		/* 129 */
	"ECONNABORTED",		/* 130 */
	"ECONNRESET",		/* 131 */
	"ENOBUFS",		/* 132 */
	"EISCONN",		/* 133 */
	"ENOTCONN",		/* 134 */
	"EUCLEAN",		/* 135 */
	 NULL,			/* 136 */
	"ENOTNAM",		/* 137 */
	"ENAVAIL",		/* 138 */
	"EISNAM",		/* 139 */
	"EREMOTEIO",		/* 140 */
	"EINIT",		/* 141 */
	"EREMDEV",		/* 142 */
	"ESHUTDOWN",		/* 143 */
	"ETOOMANYREFS",		/* 144 */
	"ETIMEDOUT",		/* 145 */
	"ECONNREFUSED",		/* 146 */
	"EHOSTDOWN",		/* 147 */
	"EHOSTUNREACH",		/* 148 */
	"EALREADY",		/* 149 */
	"EINPROGRESS",		/* 150 */
	"ESTALE",		/* 151 */
	"ENOLOAD",		/* 152 */
	"ERELOC",		/* 153 */
	"ENOMATCH",		/* 154 */
	 NULL,			/* 155 */
	"EBADVER",		/* 156 */
	"ECONFIG",		/* 157 */
	"ECANCELED",		/* 158 */
	 NULL,			/* 159 */
	"ENOTAUTH",		/* 160 */
	 NULL,			/* 161 */
	 NULL,			/* 162 */
	 NULL,			/* 163 */
	 NULL,			/* 164 */
	 NULL,			/* 165 */
	 NULL,			/* 166 */
	 NULL,			/* 167 */
	 NULL,			/* 168 */
	 NULL,			/* 169 */
	"ELKBUSY",		/* 170 */
	 NULL,			/* 171 */
	 NULL,			/* 172 */
	 NULL,			/* 173 */
	 NULL,			/* 174 */
	 NULL,			/* 175 */
	 NULL,			/* 176 */
	 NULL,			/* 177 */
	 NULL,			/* 178 */
	 NULL,			/* 179 */
	 NULL,			/* 180 */
	 NULL,			/* 181 */
	 NULL,			/* 182 */
	 NULL,			/* 183 */
	 NULL,			/* 184 */
	 NULL,			/* 185 */
	 NULL,			/* 186 */
	 NULL,			/* 187 */
	 NULL,			/* 188 */
	 NULL,			/* 189 */
	 NULL,			/* 190 */
	 NULL,			/* 191 */
	 NULL,			/* 192 */
	 NULL,			/* 193 */
	 NULL,			/* 194 */
	 NULL,			/* 195 */
	 NULL,			/* 196 */
	 NULL,			/* 197 */
	 NULL,			/* 198 */
	 NULL,			/* 199 */
	"ENOVERS",		/* 200 */
	"EVFILE",		/* 201 */
	"ENOCACHE",		/* 202 */
	"EBLANKCK",		/* 203 */
	 NULL,			/* 204 */
	 NULL,			/* 205 */
	 NULL,			/* 206 */
	 NULL,			/* 207 */
	 NULL,			/* 208 */
	 NULL,			/* 209 */
	"ENOSERVICE",		/* 210 */
	"ENOSTATISTIC",		/* 211 */
	"EARCHIVE",		/* 212 */
	"ENORESOURCE",		/* 213 */
	"ERESINUSE",		/* 214 */
	"EYETARCHIVED",		/* 215 */
	"ELIBOFFLINE",		/* 216 */
	"ENORCHANNEL",		/* 217 */
	"ESEQUENCE",		/* 218 */
	"EBADREPLY",		/* 219 */
	"EINVCORRUPTED",	/* 220 */
	"EBADLABEL",		/* 221 */
	"ENOHDCH",		/* 222 */
	"EOUTHDCH",		/* 223 */
};

#define	NERRCODE	(sizeof errcode/sizeof errcode[0])


/* return the error code name (NULL if none) */
const char *
errname(int err)
{
	if (err >= 0 && err < NERRCODE)
		return errcode[err];

	return NULL;
}


const struct systable systable[] = {
 { NULL,	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /*  0 */
 {"_exit",	1, DEC, NOV, DEC},				      /*  1 */
 {"fork",	0, DEC, NOV},					      /*  2 */
 {"read",	3, DEC, NOV, DEC, IOB, DEC},			      /*  3 */
 {"write",	3, DEC, NOV, DEC, IOB, DEC},			      /*  4 */
 {"open",	3, DEC, NOV, STG, OPN, OCT},			      /*  5 */
 {"close",	1, DEC, NOV, DEC},				      /*  6 */
 {"wait",	0, DEC, WST},					      /*  7 */
 {"creat",	2, DEC, NOV, STG, OCT},				      /*  8 */
 {"link",	2, DEC, NOV, STG, STG},				      /*  9 */
 {"unlink",	1, DEC, NOV, STG},				      /* 10 */
 {"exec",	2, DEC, NOV, STG, DEC},				      /* 11 */
 {"chdir",	1, DEC, NOV, STG},				      /* 12 */
 {"time",	0, DEC, NOV},					      /* 13 */
 {"mknod",	3, DEC, NOV, STG, OCT, HEX},			      /* 14 */
 {"chmod",	2, DEC, NOV, STG, OCT},				      /* 15 */
 {"chown",	3, DEC, NOV, STG, DEC, DEC},			      /* 16 */
 {"brk",	1, DEC, NOV, HEX},				      /* 17 */
 {"stat",	2, DEC, NOV, STG, HEX},				      /* 18 */
 {"lseek",	3, DEX, NOV, DEC, DEX, DEC},			      /* 19 */
 {"getpid",	0, DEC, DC2},					      /* 20 */
 {"mount",	6, DEC, NOV, STG, STG, MTF, MFT, HEX, DEC},	      /* 21 */
 {"umount",	1, DEC, NOV, STG},				      /* 22 */
 {"setuid",	1, DEC, NOV, DEC},				      /* 23 */
 {"getuid",	0, DEC, DC2},					      /* 24 */
 {"stime",	1, DEC, NOV, DEC},				      /* 25 */
 {"ptrace",	4, HEX, NOV, DEC, DEC, HEX, HEX},		      /* 26 */
 {"alarm",	1, DEC, NOV, DEC},				      /* 27 */
 {"fstat",	2, DEC, NOV, DEC, HEX},				      /* 28 */
 {"pause",	0, DEC, NOV},					      /* 29 */
 {"utime",	2, DEC, NOV, STG, HEX},				      /* 30 */
 {"stty",	2, DEC, NOV, DEC, DEC},				      /* 31 */
 {"gtty",	2, DEC, NOV, DEC, DEC},				      /* 32 */
 {"access",	2, DEC, NOV, STG, DEC},				      /* 33 */
 {"nice",	1, DEC, NOV, DEC},				      /* 34 */
 {"statfs",	4, DEC, NOV, STG, HEX, DEC, DEC},		      /* 35 */
 {"sync",	0, DEC, NOV},					      /* 36 */
 {"kill",	2, DEC, NOV, DEC, SIG},				      /* 37 */
 {"fstatfs",	4, DEC, NOV, DEC, HEX, DEC, DEC},		      /* 38 */
 {"pgrpsys",	3, DEC, NOV, DEC, DEC, DEC},			      /* 39 */
 {"xenix",	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 40 */
 {"dup",	1, DEC, NOV, DEC},				      /* 41 */
 {"pipe",	0, DEC, DC2},					      /* 42 */
 {"times",	1, DEC, NOV, HEX},				      /* 43 */
 {"profil",	4, DEC, NOV, HEX, DEC, HEX, OCT},		      /* 44 */
 {"plock",	1, DEC, NOV, PLK},				      /* 45 */
 {"setgid",	1, DEC, NOV, DEC},				      /* 46 */
 {"getgid",	0, DEC, DC2},					      /* 47 */
 {"signal",	2, ACT, NOV, SIG, ACT},				      /* 48 */
 {"msgsys",	6, DEC, NOV, DEC, DEC, DEC, DEC, DEC, DEC},	      /* 49 */
 {"sysi86",	4, DEC, NOV, PSYS, HEX, HEX, HEX},		      /* 50 */
 {"acct",	1, DEC, NOV, STG},				      /* 51 */
 {"shmsys",	4, DEC, NOV, DEC, HEX, HEX, HEX},		      /* 52 */
 {"semsys",	5, DEC, NOV, DEC, HEX, HEX, HEX, HEX},		      /* 53 */
 {"ioctl",	3, DEC, NOV, DEC, IOC, IOA},			      /* 54 */
 {"uadmin",	3, DEC, NOV, DEC, DEC, DEC},			      /* 55 */
 { NULL,	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 56 */
 {"utssys",	4, DEC, NOV, HEX, DEC, UTS, HEX},		      /* 57 */
 {"fsync",	1, DEC, NOV, DEC},				      /* 58 */
 {"execve",	3, DEC, NOV, STG, HEX, HEX},			      /* 59 */
 {"umask",	1, OCT, NOV, OCT},				      /* 60 */
 {"chroot",	1, DEC, NOV, STG},				      /* 61 */
 {"fcntl",	3, DEC, NOV, DEC, FCN, HEX},			      /* 62 */
 {"ulimit",	2, DEX, NOV, ULM, DEC},				      /* 63 */
 {"cg_ids",	3, DEC, NOV, CGS, DEC, HEX},			      /* 64 */
 {"cg_processors",5,DEC,NOV, CGL, CGH, CGP, DEC, HEX},		      /* 65 */
 {"cg_info",	3, DEC, NOV, CGL, CGH, HEX},			      /* 66 */
 {"cg_bind",	7, DEC, NOV, IDT, DEC, CGL, CGH, CGF, HEX, HEX},      /* 67 */
 {"cg_current",	2, CGL, CGH, IDT, DEC},				      /* 68 */
 {"cg_memloc",	3, DEC, NOV, DEX, DEC, HEX},			      /* 69 */
	/* Obsolete RFS-specific entries */
 { NULL,	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 70 */
 { NULL,	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 71 */
 { NULL,	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 72 */
 { NULL,	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 73 */
 { NULL,	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 74 */
 { NULL,	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 75 */
 { NULL,	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 76 */
 { NULL,	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 77 */

#ifdef RFS_SUPPORT
 {"rfsys",	5, DEC, NOV, RFS, HEX, HEX, HEX, HEX},		      /* 78 */
#else
 { NULL,	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 78 */
#endif /* RFS_SUPPORT */
 {"rmdir",	1, DEC, NOV, STG},				      /* 79 */
 {"mkdir",	2, DEC, NOV, STG, OCT},				      /* 80 */
 {"getdents",	3, DEC, NOV, DEC, HEX, DEC},			      /* 81 */
 { NULL,	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 82 */
 { NULL,	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 83 */
 {"sysfs",	3, DEC, NOV, SFS, DEX, DEX},			      /* 84 */
 {"getmsg",	4, DEC, NOV, DEC, HEX, HEX, HEX},		      /* 85 */
 {"putmsg",	4, DEC, NOV, DEC, HEX, HEX, SMF},		      /* 86 */
 {"poll",	3, DEC, NOV, HEX, DEC, DEC},			      /* 87 */
 {"lstat",	2, DEC, NOV, STG, HEX},				      /* 88 */
 {"symlink",	2, DEC, NOV, STG, STG},				      /* 89 */
 {"readlink",	3, DEC, NOV, STG, RLK, DEC},			      /* 90 */
 {"setgroups",	2, DEC, NOV, DEC, HEX},				      /* 91 */
 {"getgroups",	2, DEC, NOV, DEC, HEX},				      /* 92 */
 {"fchmod",	2, DEC, NOV, DEC, OCT},				      /* 93 */
 {"fchown",	3, DEC, NOV, DEC, DEC, DEC},			      /* 94 */
 {"sigprocmask",3, DEC, NOV, SPM, HEX, HEX},			      /* 95 */
 {"sigsuspend",	1, DEC, NOV, HEX},				      /* 96 */
 {"sigaltstack",2, DEC, NOV, HEX, HEX},				      /* 97 */
 {"sigaction",	3, DEC, NOV, SIG, HEX, HEX},			      /* 98 */
 {"sigpending",	2, DEC, NOV, DEC, HEX},				      /* 99 */
 {"context",	2, DEC, NOV, DEC, HEX},				      /* 100 */
 {"evsys",	3, DEC, NOV, DEC, DEC, HEX},			      /* 101 */
 {"evtrapret",	0, DEC, NOV},					      /* 102 */
 {"statvfs",	2, DEC, NOV, STG, HEX},				      /* 103 */
 {"fstatvfs",	2, DEC, NOV, DEC, HEX},				      /* 104 */
 { NULL,	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 105 */
 {"nfssys",	2, DEC, NOV, DEC, HEX},				      /* 106 */
 {"waitsys",	4, DEC, NOV, IDT, DEC, HEX, WOP},		      /* 107 */
 {"sigsendsys",	2, DEC, NOV, HEX, SIG},				      /* 108 */
 {"hrtsys",	5, DEC, NOV, DEC, HEX, HEX, HEX, HEX},		      /* 109 */
 {"acancel",	3, DEC, NOV, DEC, HEX, DEC},			      /* 110 */
 {"async",	3, DEC, NOV, DEC, HEX, DEC},			      /* 111 */
 {"priocntlsys",4, DEC, NOV, DEC, HEX, DEC, HEX},		      /* 112 */
 {"pathconf",	2, DEC, NOV, STG, PTC},				      /* 113 */
 {"mincore",	3, DEC, NOV, HEX, DEC, HEX},			      /* 114 */
 {"mmap",	6, HEX, NOV, HEX, DEC, MPR, MTY, DEC, DEC},	      /* 115 */
 {"mprotect",	3, DEC, NOV, HEX, DEC, MPR},			      /* 116 */
 {"munmap",	2, DEC, NOV, HEX, DEC},				      /* 117 */
 {"fpathconf",	2, DEC, NOV, DEC, PTC},				      /* 118 */
 {"vfork",	0, DEC, NOV},					      /* 119 */
 {"fchdir",	1, DEC, NOV, DEC},				      /* 120 */
 {"readv",	3, DEC, NOV, DEC, IOV, DEC},			      /* 121 */
 {"writev",	3, DEC, NOV, DEC, IOV, DEC},			      /* 122 */
 {"xstat",	3, DEC, NOV, DEC, STG, HEX},			      /* 123 */
 {"lxstat",	3, DEC, NOV, DEC, STG, HEX},			      /* 124 */
 {"fxstat",	3, DEC, NOV, DEC, DEC, HEX},			      /* 125 */
 {"xmknod",	4, DEC, NOV, DEC, STG, OCT, HEX},		      /* 126 */
 {"clocal",	5, HEX, HX2, DEC, HEX, HEX, HEX, HEX},		      /* 127 */
 {"setrlimit",	2, DEC, NOV, RLM, HEX},				      /* 128 */
 {"getrlimit",	2, DEC, NOV, RLM, HEX},				      /* 129 */
 {"lchown",	3, DEC, NOV, STG, DEC, DEC},			      /* 130 */
 {"memcntl",	6, DEC, NOV, HEX, DEC, MCF, MC4, MC5, DEC},	      /* 131 */
 {"getpmsg",	5, DEC, NOV, DEC, HEX, HEX, HEX, HEX},		      /* 132 */
 {"putpmsg",	5, DEC, NOV, DEC, HEX, HEX, DEC, HHX},		      /* 133 */
 {"rename",	2, DEC, NOV, STG, STG},				      /* 134 */
 {"nuname",	1, DEC, NOV, HEX},				      /* 135 */
 {"setegid",	1, DEC, NOV, DEC},				      /* 136 */
 {"sysconfig",	1, DEC, NOV, CNF},				      /* 137 */
 {"adjtime",	2, DEC, NOV, HEX, HEX},				      /* 138 */
 {"systeminfo",	3, DEC, NOV, INF, RST, DEC},			      /* 139 */
 { NULL,	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 140 */
 {"seteuid",	1, DEC, NOV, DEC},				      /* 141 */
 { NULL,	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 142 */
 {"keyctl",	3, DEC, DC2, DEC, HEX, DEC},			      /* 143 */
 {"secsys",	2, DEC, NOV, DEC, DEC},				      /* 144 */
 {"filepriv",	4, DEC, NOV, STG, DEC, HEX, DEC},		      /* 145 */
 {"procpriv",	3, DEC, NOV, DEC, HEX, DEC},			      /* 146 */
 {"devstat",	3, DEC, NOV, STG, DEC, HEX},			      /* 147 */
 {"aclipc",	5, DEC, NOV, DEC, DEC, DEC, DEC, HEX},		      /* 148 */
 {"fdevstat",	3, DEC, NOV, DEC, DEC, HEX},			      /* 149 */
 {"flvlfile",	3, DEC, NOV, DEC, DEC, HEX},			      /* 150 */
 {"lvlfile",	3, DEC, NOV, STG, DEC, HEX},			      /* 151 */
 {"sendv",	3, DEC, NOV, DEC, HEX, DEC},			      /* 152 */
 {"lvlequal",	2, DEC, NOV, HEX, HEX},				      /* 153 */
 {"lvlproc",	2, DEC, NOV, DEC, HEX},				      /* 154 */
 { NULL,	1, DEC, NOV, STG},				      /* 155 */
 {"lvlipc",	4, DEC, NOV, DEC, DEC, DEC, HEX},		      /* 156 */
 {"acl",	4, DEC, NOV, STG, DEC, DEC, HEX},		      /* 157 */
 {"auditevt",	3, DEC, NOV, DEC, HEX, DEC},			      /* 158 */
 {"auditctl",	3, DEC, NOV, DEC, HEX, DEC},			      /* 159 */
 {"auditdmp",	2, DEC, NOV, HEX, DEC},				      /* 160 */
 {"auditlog",	3, DEC, NOV, DEC, HEX, DEC},			      /* 161 */
 {"auditbuf",	3, DEC, NOV, DEC, HEX, DEC},			      /* 162 */
 {"lvldom",	2, DEC, NOV, HEX, HEX},				      /* 163 */
 {"lvlvfs",	4, DEC, NOV, DEC, STG, DEC, HEX, HEX},		      /* 164 */
 {"mkmld",	1, DEC, NOV, STG},				      /* 165 */
 {"mldmode",	1, DEC, NOV, DEC},				      /* 166 */
 {"secadvise",	3, DEC, NOV, HEX, DEC, HEX},			      /* 167 */
 {"online",	2, DEC, NOV, DEC, DEC},				      /* 168 */
 {"setitimer",	3, DEC, NOV, DEC, HEX, HEX},			      /* 169 */
 {"getitimer",	2, DEC, NOV, DEC, HEX},				      /* 170 */
 {"gettimeofday",2,DEC, NOV, HEX, HEX},				      /* 171 */
 {"settimeofday",2,DEC, NOV, HEX, HEX},				      /* 172 */
 {"_lwp_create",2, DEC, NOV, HEX, HEX},				      /* 173 */
 {"_lwp_exit",	0, DEC, NOV},					      /* 174 */
 {"_lwp_wait",	1, DEC, DC2, DEC}, 				      /* 175 */
 {"__lwp_self",	0, DEC, NOV},					      /* 176 */
 {"_lwp_info",	1, DEC, NOV, HEX},				      /* 177 */
 {"__lwp_private",	1, HEX, NOV, HEX},			      /* 178 */
 {"processor_bind",	4, DEC, NOV, IDT, DEC, DEC, HEX},	      /* 179 */
 {"processor_exbind",	5, DEC, NOV, IDT, HEX, DEC, DEC, HEX},	      /* 180 */
 { NULL,	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 181 */
 {"sendv64",	3, DLX, DHX, DEC, HEX, DEC},			      /* 182 */
 {"prepblock",	3, DEC, NOV, HEX, HEX, DEC},			      /* 183 */
 {"block",	1, DEC, NOV, HEX},				      /* 184 */
 {"rdblock",	1, DEC, NOV, HEX},				      /* 185 */
 {"unblock",	3, DEC, NOV, HEX, HEX, DEC},			      /* 186 */
 {"cancelblock",0, DEC, NOV},					      /* 187 */
 { NULL,	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 188 */
 {"pread",	4, DEC, NOV, DEC, IOB, DEC, DEX},		      /* 189 */
 {"pwrite",	4, DEC, NOV, DEC, IOB, DEC, DEX},		      /* 190 */
 {"truncate",	2, DEC, NOV, STG, DEX},				      /* 191 */
 {"ftruncate",	2, DEC, NOV, DEC, DEX},				      /* 192 */
 {"_lwp_kill",	2, DEC, NOV, DEC, SIG},				      /* 193 */
 {"sigwait",	1, SIG, NOV, HEX},				      /* 194 */
 {"fork1",	0, DEC, NOV},					      /* 195 */
 {"forkall",	0, DEC, NOV},					      /* 196 */
 {"modload",	1, DEC, NOV, STG},				      /* 197 */
 {"moduload",	1, DEC, NOV, DEC},				      /* 198 */
 {"modpath",	1, DEC, NOV, STG},				      /* 199 */
 {"modstat",	3, DEC, NOV, DEC, HEX, DEC},			      /* 200 */
 {"modadm",	3, DEC, NOV, DEC, DEC, HEX},			      /* 201 */
 {"getksym",	3, DEC, NOV, STG, HEX, HEX},			      /* 202 */
 {"_lwp_suspend",	1, DEC, NOV, DEC},			      /* 203 */
 {"_lwp_continue",	1, DEC, NOV, DEC},			      /* 204 */
 {"priocntllist",	5, HEX, NOV, DEC, HEX, DEC, DEC, HEX},	      /* 205 */
 {"sleep",		1, DEC, NOV, DEC},			      /* 206 */
 {"_lwp_sema_wait",	1, DEC, NOV, HEX},			      /* 207 */
 {"_lwp_sema_post",	1, DEC, NOV, HEX},			      /* 208 */
 {"_lwp_sema_trywait",	1, DEC, NOV, HEX},			      /* 209 */
 { NULL,	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 210 */
 { NULL,	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 211 */
 { NULL,	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 212 */
 { NULL,	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 213 */
 { NULL,	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 214 */
 { NULL,	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 215 */
 {"fstatvfs64", 2, DEC, NOV, DEC, HEX},				      /* 216 */
 {"statvfs64",	2, DEC, NOV, STG, HEX},				      /* 217 */
 {"ftruncate64",3, DEC, NOV, DEC, DLX, DHX},			      /* 218 */
 {"truncate64",	3, DEC, NOV, STG, DLX, DHX},			      /* 219 */
 {"getrlimit64",2, DEC, NOV, RLM, HEX},				      /* 220 */
 {"setrlimit64",2, DEC, NOV, RLM, HEX},				      /* 221 */
 {"lseek64",	4, DLX, DHX, DEC, DLX, DHX, DEC},		      /* 222 */
 {"mmap64",	7, HEX, NOV, HEX, DEC, MPR, MTY, DEC, DLX, DHX},      /* 223 */
 {"pread64",	5, DEC, NOV, DEC, IOB, DEC, DLX, DHX},		      /* 224 */
 {"pwrite64",	5, DEC, NOV, DEC, IOB, DEC, DLX, DHX},		      /* 225 */
 {"creat64",	2, DEC, NOV, STG, OCT},				      /* 226 */
 {"dshmsys",	6, DEC, NOV, DEC, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 227 */
 {"invlpg",	2, HEX, HX2, HEX, HEX}, 			      /* 228 */
 { NULL,       -1, DEC, NOV},					      /* end */
};

/* SYSEND == max syscall number + 1 */
#define	SYSEND	((sizeof systable/sizeof systable[0])-1)


/* The following are for interpreting syscalls with sub-codes */

static const struct systable sigtable[] = {
 {"signal",	2, ACT, NOV, SIG, ACT},				      /* 0 */
 {"sigset",	2, ACT, NOV, SIX, ACT},				      /* 1 */
 {"sighold",	1, DEC, NOV, SIX},				      /* 2 */
 {"sigrelse",	1, DEC, NOV, SIX},				      /* 3 */
 {"sigignore",	1, DEC, NOV, SIX},				      /* 4 */
 {"sigpause",	1, DEC, NOV, SIX},				      /* 5 */
};
#define	NSIGCODE	(sizeof sigtable/sizeof systable[0])

static const struct systable msgtable[] = {
 {"msgget",	3, DEC, NOV, HID, DEC, MSF},			      /* 0 */
 {"msgctl",	4, DEC, NOV, HID, DEC, MSC, HEX},		      /* 1 */
 {"msgrcv",	6, DEC, NOV, HID, DEC, HEX, DEC, DEC, MSF},	      /* 2 */
 {"msgsnd",	5, DEC, NOV, HID, DEC, HEX, DEC, MSF},		      /* 3 */
};
#define	NMSGCODE	(sizeof msgtable/sizeof systable[0])

static const struct systable semtable[] = {
 {"semctl",	5, DEC, NOV, HID, DEC, DEC, SEC, DEX},		      /* 0 */
 {"semget",	4, DEC, NOV, HID, DEC, DEC, SEF},		      /* 1 */
 {"semop",	4, DEC, NOV, HID, DEC, HEX, DEC},		      /* 2  */
};
#define	NSEMCODE	(sizeof semtable/sizeof systable[0])

static const struct systable shmtable[] = {
 {"shmat",	4, HEX, NOV, HID, DEC, DEX, SHF},		      /* 0 */
 {"shmctl",	4, DEC, NOV, HID, DEC, SHC, DEX},		      /* 1 */
 {"shmdt",	2, DEC, NOV, HID, HEX},				      /* 2 */
 {"shmget",	4, DEC, NOV, HID, DEC, DEC, SHF},		      /* 3 */
};
#define	NSHMCODE	(sizeof shmtable/sizeof systable[0])

static const struct systable dshmtable[] = {
 {"dshmat",	3, HEX, NOV, HID, DEC, DSHF},		              /* 0 */
 {"dshmctl",	4, DEC, NOV, HID, DEC, DSHC, DEX},		      /* 1 */
 {"dshmdt",	2, DEC, NOV, HID, HEX},				      /* 2 */
 {"dshmget", 	8, DEC, NOV, HID, HEX, DEC, DEC, DEC, HEX, DEC, DSHF},/* 3 */
 {"dshm_kalignment",	1, HEX, NOV, HID}, 			      /* 4 */
 {"dshm_remap",	4, DEC, NOV, HID, DEC, HEX, DEC} 		      /* 5 */
};
#define	NDSHMCODE	(sizeof dshmtable/sizeof systable[0])

static const struct systable pidtable[] = {
 {"getpgrp",	1, DEC, NOV, HID},				      /* 0 */
 {"setpgrp",	1, DEC, NOV, HID},				      /* 1 */
 {"getsid",	2, DEC, NOV, HID, DEC},				      /* 2 */
 {"setsid",	1, DEC, NOV, HID},				      /* 3 */
 {"getpgid",	2, DEC, NOV, HID, DEC},				      /* 4 */
 {"setpgid",	3, DEC, NOV, HID, DEC, DEC},			      /* 5 */
};
#define	NPIDCODE	(sizeof pidtable/sizeof systable[0])

static const struct systable sfstable[] = {
 {"sysfs",	3, DEC, NOV, SFS, DEX, DEX},			      /* 0 */
 {"sysfs",	2, DEC, NOV, SFS, STG},				      /* 1 */
 {"sysfs",	3, DEC, NOV, SFS, DEC, RST},			      /* 2 */
 {"sysfs",	1, DEC, NOV, SFS},				      /* 3 */
};
#define	NSFSCODE	(sizeof sfstable/sizeof systable[0])

#ifdef RFS_SUPPORT
static const struct systable rfstable[] = {
 {"rfsys",	5, DEC, NOV, RFS, STG, DEC, DEC, DEC},		      /* 0 */
 {"rfsys",	2, DEC, NOV, RFS, STG},				      /* 1 */
 {"rfsys",	4, DEC, NOV, RFS, HEX, STG, DEC},		      /* 2 */
 {"rfsys",	3, DEC, NOV, RFS, RST, DEC},			      /* 3 */
 {"rfsys",	1, DEC, NOV, RFS},				      /* 4 */
 {"rfsys",	3, DEC, NOV, RFS, STG, DEC},			      /* 5 */
 {"rfsys",	3, DEC, NOV, RFS, RST, DEC},			      /* 6 */
 {"rfsys",	4, DEC, NOV, RFS, STG, DEC, HEX},		      /* 7 */
 {"rfsys",	4, DEC, NOV, RFS, DEC, HEX, HEX},		      /* 8 */
 {"rfsys",	2, DEC, NOV, RFS, RV1},				      /* 9 */
 {"rfsys",	4, DEC, NOV, RFS, RV2, HEX, HEX},		      /* 10 */
 {"rfsys",	1, DEC, NOV, RFS},				      /* 11 */
 {"rfsys",	2, DEC, NOV, RFS, RV3},				      /* 12 */
 {"rfsys",	3, DEC, NOV, RFS, STG, HEX},			      /* 13 */
 {"rfsys",	2, DEC, NOV, RFS, HEX},				      /* 14 */
 {"rfsys",	5, DEC, NOV, RFS, STG, STG, DEC, HEX, HEX},	      /* 15 */
 {"rfsys",	2, DEC, NOV, RFS, STG},				      /* 16 */
 {"rfsys",	1, DEC, NOV, RFS},				      /* 17 */
 {"rfsys",	1, DEC, NOV, RFS},				      /* 18 */
 {"rfsys",	2, DEC, NOV, RFS, HEX},				      /* 19 */
};
#define	NRFSCODE	(sizeof rfstable/sizeof systable[0])
#endif /* RFS_SUPPORT */

static const struct systable utstable[] = {
 {"utssys",	3, DEC, NOV, HEX, DEC, UTS},			      /* 0 */
 {"utssys",	4, DEC, NOV, HEX, HEX, HEX, HEX},		      /* err */
 {"utssys",	3, DEC, NOV, HEX, HHX, UTS},			      /* 2 */
 {"utssys",	4, DEC, NOV, STG, FUI, UTS, HEX}		      /* 3 */
};
#define	NUTSCODE	(sizeof utstable/sizeof systable[0])

static const struct systable sgptable[] = {
 {"sigpending",	2, DEC, NOV, DEC, HEX},				      /* err */
 {"sigpending",	2, DEC, NOV, HID, HEX},				      /* 1 */
 {"sigfillset",	2, DEC, NOV, HID, HEX},				      /* 2 */
};
#define	NSGPCODE	(sizeof sgptable/sizeof systable[0])

static const struct systable ctxtable[] = {
 {"getcontext",	2, DEC, NOV, HID, HEX},				      /* 0 */
 {"setcontext",	2, DEC, NOV, HID, HEX},				      /* 1 */
};
#define	NCTXCODE	(sizeof ctxtable/sizeof systable[0])

static const struct systable hrttable[] = {
 {"hrtcntl",	5, DEC, NOV, HID, DEC, DEC, HEX, HEX},		      /* 0 */
 {"hrtalarm",	3, DEC, NOV, HID, HEX, DEC},			      /* 1 */
 {"hrtsleep",	2, DEC, NOV, HID, HEX},				      /* 2 */
 {"hrtcancel",	3, DEC, NOV, HID, HEX, DEC},			      /* 3 */
};
#define	NHRTCODE	(sizeof hrttable/sizeof systable[0])

static const struct systable xentable[] = {
 {"Xshutdown",	8, DEC, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 0 */
 {"locking",	3, DEC, DC2, DEC, DEC, DEC},			      /* 1 */
 {"creatsem",	2, DEC, DC2, STG, OCT},				      /* 2 */
 {"opensem",	1, DEC, NOV, STG},				      /* 3 */
 {"sigsem",	1, DEC, NOV, DEC},				      /* 4 */
 {"waitsem",	1, DEC, NOV, DEC},				      /* 5 */
 {"nbwaitsem",	1, DEC, NOV, DEC},				      /* 6 */
 {"rdchk",	1, DEC, NOV, DEC},				      /* 7 */
 {"Xstkgrow",	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 8 */
 {"Xptrace",	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 9 */
 {"chsize",	2, DEC, NOV, DEC, DEC},				      /* 10 */
 {"ftime",	1, DEC, NOV, HEX},				      /* 11 */
 {"nap",	1, DEC, NOV, DEC},				      /* 12 */
 {"sdget",	4, DEC, NOV, STG, HEX, HEX, OCT},		      /* 13 */
 {"xsdfree",	1, DEC, NOV, HEX},				      /* 14 */
 {"sdenter",	2, DEC, NOV, HEX, HEX},				      /* 15 */
 {"sdleave",	1, DEC, NOV, HEX},				      /* 16 */
 {"sdgetv",	1, DEC, NOV, HEX},				      /* 17 */
 {"sdwaitv",	2, DEC, NOV, HEX, DEC},				      /* 18 */
 {"Xbrkctl",	8, DEC, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 19 */
 {"X20",	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 20 */
 {"Xnfs_sys",	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 21 */
 {"Xmsgctl",	8, DEC, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 22 */
 {"Xmsgget",	8, DEC, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 23 */
 {"Xmsgsnd",	8, DEC, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 24 */
 {"Xmsgrcv",	8, DEC, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 25 */
 {"Xsemctl",	8, DEC, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 26 */
 {"Xsemget",	8, DEC, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 27 */
 {"Xsemop",	8, DEC, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 28 */
 {"Xshmctl",	8, DEC, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 29 */
 {"Xshmget",	8, DEC, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 30 */
 {"Xshmat",	8, DEC, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 31 */
 {"Xproctl",	3, DEC, NOV, DEC, DEC, HEX},			      /* 32 */
 {"execseg",	0, HEX, HX2},					      /* 33 */
 {"unexecseg",	0, DEC, NOV},					      /* 34 */
 {"Xswapadd",	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 35 */
 {"Xselect",	5, HEX, NOV, DEC, HEX, HEX, HEX, HEX},		      /* 36 */
 {"Xeaccess",	2, HEX, NOV, STG, OCT},				      /* 37 */
 {"X38",	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 38 */
 {"Xsigaction",	3, DEC, NOV, SIG, HEX, HEX},			      /* 39 */
 {"Xsigprocmask",3,DEC, NOV, DEC, HEX, HEX},			      /* 40 */
 {"Xsigpending",1, DEC, NOV, HEX},				      /* 41 */
 {"Xsigsuspend",1, DEC, NOV, HEX},				      /* 42 */
 {"Xgetgroups",	2, DEC, NOV, DEC, HEX},				      /* 43 */
 {"Xsetgroups",	2, DEC, NOV, DEC, HEX},				      /* 44 */
 {"Xsysconf",	1, DEC, NOV, DEC},				      /* 45 */
 {"Xpathconf",	2, DEC, NOV, STG, DEC},				      /* 46 */
 {"Xfpathconf",	2, DEC, NOV, DEC, DEC},				      /* 47 */
 {"Xrename",	2, DEC, NOV, STG, STG},				      /* 48 */
 {"X49",	8, HEX, HX2, HEX, HEX, HEX, HEX, HEX, HEX, HEX, HEX}, /* 49 */
 {"scoinfo",	2, DEC, NOV, HEX, DEC},				      /* 50 */
};
#define	NXENCODE	(sizeof xentable/sizeof systable[0])

const struct sysalias sysalias[] = {
	{ "exit",	SYS_exit	},
	{ "sbrk",	SYS_brk		},
	{ "getppid",	SYS_getpid	},
	{ "geteuid",	SYS_getuid	},
	{ "getpgrp",	SYS_pgrpsys	},
	{ "setpgrp",	SYS_pgrpsys	},
	{ "getsid",	SYS_pgrpsys	},
	{ "setsid",	SYS_pgrpsys	},
	{ "getpgid",	SYS_pgrpsys	},
	{ "setpgid",	SYS_pgrpsys	},
	{ "getegid",	SYS_getgid	},
	{ "sigset",	SYS_signal	},
	{ "sighold",	SYS_signal	},
	{ "sigrelse",	SYS_signal	},
	{ "sigignore",	SYS_signal	},
	{ "sigpause",	SYS_signal	},
	{ "msgctl",	SYS_msgsys	},
	{ "msgget",	SYS_msgsys	},
	{ "msgsnd",	SYS_msgsys	},
	{ "msgrcv",	SYS_msgsys	},
	{ "msgop",	SYS_msgsys	},
	{ "shmctl",	SYS_shmsys	},
	{ "shmget",	SYS_shmsys	},
	{ "shmat",	SYS_shmsys	},
	{ "shmdt",	SYS_shmsys	},
	{ "shmop",	SYS_shmsys	},
	{ "semctl",	SYS_semsys	},
	{ "semget",	SYS_semsys	},
	{ "semop",	SYS_semsys	},
	{ "uname",	SYS_utssys	},
	{ "ustat",	SYS_utssys	},
	{ "fusers",	SYS_utssys	},
	{ "exec",	SYS_execve	},
	{ "execl",	SYS_execve	},
	{ "execv",	SYS_execve	},
	{ "execle",	SYS_execve	},
	{ "execlp",	SYS_execve	},
	{ "execvp",	SYS_execve	},
	{ "sigfillset",	SYS_sigpending	},
	{ "getcontext",	SYS_context	},
	{ "setcontext",	SYS_context	},
	{ "hrtcntl",	SYS_hrtsys	},
	{ "hrtalarm",	SYS_hrtsys	},
	{ "hrtsleep",	SYS_hrtsys	},
	{ "hrtcancel",	SYS_hrtsys	},
	{ "locking",	SYS_xenix	},
	{ "creatsem",	SYS_xenix	},
	{ "opensem",	SYS_xenix	},
	{ "sigsem",	SYS_xenix	},
	{ "waitsem",	SYS_xenix	},
	{ "nbwaitsem",	SYS_xenix	},
	{ "rdchk",	SYS_xenix	},
	{ "chsize",	SYS_xenix	},
	{ "ftime",	SYS_xenix	},
	{ "nap",	SYS_xenix	},
	{ "sdget",	SYS_xenix	},
	{ "sdfree",	SYS_xenix	},
	{ "sdenter",	SYS_xenix	},
	{ "sdleave",	SYS_xenix	},
	{ "sdgetv",	SYS_xenix	},
	{ "sdwaitv",	SYS_xenix	},
	{ "proctl",	SYS_xenix	},
	{ "select",	SYS_xenix	},
	{  NULL,	0	},	/* end-of-list */
};


/* return structure to interpret system call with sub-codes */
const struct systable *
subsys(int syscall, int subcode)
{
	register const struct systable *stp = NULL;

	if (subcode != -1) {
		switch (syscall) {
		case SYS_signal:
			/* signal() + sigset() family */
			switch(subcode & ~SIGNO_MASK) {
			default:	subcode = 0;	break;
			case SIGDEFER:	subcode = 1;	break;
			case SIGHOLD:	subcode = 2;	break;
			case SIGRELSE:	subcode = 3;	break;
			case SIGIGNORE:	subcode = 4;	break;
			case SIGPAUSE:	subcode = 5;	break;
			}
			if ((unsigned)subcode < NSIGCODE)
				stp = &sigtable[subcode];
			break;
		case SYS_msgsys:	/* msgsys() */
			if ((unsigned)subcode < NMSGCODE)
				stp = &msgtable[subcode];
			break;
		case SYS_semsys:	/* semsys() */
			if ((unsigned)subcode < NSEMCODE)
				stp = &semtable[subcode];
			break;
		case SYS_shmsys:	/* shmsys() */
			if ((unsigned)subcode < NSHMCODE)
				stp = &shmtable[subcode];
			break;
		case SYS_dshmsys:	/* dshmsys() */
			if ((unsigned)subcode < NDSHMCODE)
				stp = &dshmtable[subcode];
			break;
		case SYS_pgrpsys:	/* pgrpsys() */
			if ((unsigned)subcode < NPIDCODE)
				stp = &pidtable[subcode];
			break;
		case SYS_utssys:	/* utssys() */
			if ((unsigned)subcode < NUTSCODE)
				stp = &utstable[subcode];
			break;
		case SYS_sysfs:		/* sysfs() */
			if ((unsigned)subcode < NSFSCODE)
				stp = &sfstable[subcode];
			break;
#ifdef RFS_SUPPORT
		case SYS_rfsys:		/* rfsys() */
			if ((unsigned)subcode < NRFSCODE)
				stp = &rfstable[subcode];
			break;
#endif /* RFS_SUPPORT */
		case SYS_sigpending:	/* sigpending()/sigfillset() */
			if ((unsigned)subcode < NSGPCODE)
				stp = &sgptable[subcode];
			break;
		case SYS_context:	/* [get|set]context() */
			if ((unsigned)subcode < NCTXCODE)
				stp = &ctxtable[subcode];
			break;
		case SYS_hrtsys:	/* hrtsys() */
			if ((unsigned)subcode < NHRTCODE)
				stp = &hrttable[subcode];
			break;
		case SYS_xenix:		/* xenix() */
			if ((unsigned)subcode < NXENCODE)
				stp = &xentable[subcode];
			break;
		}
	}

	if (stp == NULL)
		stp = &systable[((unsigned)syscall < SYSEND)? syscall : 0];

	return stp;
}


/* return the name of the system call */
const char *
sysname(int syscall, int subcode)
{
	const struct systable * stp = subsys(syscall, subcode);
	const char * name = stp->name;	/* may be NULL */

	if (name == NULL) {		/* manufacture a name */
		(void) sprintf(sys_name, "sys#%d", syscall);
		name = sys_name;
	}

	return name;
}

/* return the name of the signal */
/* return NULL if unknown signal */
const char *
rawsigname(int sig)
{
	const char * name;

	switch (sig) {
	case SIGHUP:		name = "SIGHUP";	break;
	case SIGINT:		name = "SIGINT";	break;
	case SIGQUIT:		name = "SIGQUIT";	break;
	case SIGILL:		name = "SIGILL";	break;
	case SIGTRAP:		name = "SIGTRAP";	break;
	case SIGABRT:		name = "SIGABRT";	break;
	case SIGEMT:		name = "SIGEMT";	break;
	case SIGFPE:		name = "SIGFPE";	break;
	case SIGKILL:		name = "SIGKILL";	break;
	case SIGBUS:		name = "SIGBUS";	break;
	case SIGSEGV:		name = "SIGSEGV";	break;
	case SIGSYS:		name = "SIGSYS";	break;
	case SIGPIPE:		name = "SIGPIPE";	break;
	case SIGALRM:		name = "SIGALRM";	break;
	case SIGTERM:		name = "SIGTERM";	break;
	case SIGUSR1:		name = "SIGUSR1";	break;
	case SIGUSR2:		name = "SIGUSR2";	break;
	case SIGCLD:		name = "SIGCLD";	break;
	case SIGPWR:		name = "SIGPWR";	break;
	case SIGWINCH:		name = "SIGWINCH";	break;
	case SIGURG:		name = "SIGURG";	break;
	case SIGPOLL:		name = "SIGPOLL";	break;
	case SIGSTOP:		name = "SIGSTOP";	break;
	case SIGTSTP:		name = "SIGTSTP";	break;
	case SIGCONT:		name = "SIGCONT";	break;
	case SIGTTIN:		name = "SIGTTIN";	break;
	case SIGTTOU:		name = "SIGTTOU";	break;
	case SIGVTALRM:		name = "SIGVTALRM";	break;
	case SIGPROF:		name = "SIGPROF";	break;
	case SIGXCPU:		name = "SIGXCPU";	break;
	case SIGXFSZ:		name = "SIGXFSZ";	break;
	case SIGWAITING:	name = "SIGWAITING";	break;
	case SIGLWP:		name = "SIGLWP";	break;
	case SIGAIO:		name = "SIGAIO";	break;
	default:		name = NULL;		break;
	}

	return name;
}

/* return the name of the signal */
/* manufacture a name for unknown signal */
const char *
signame(int sig)
{
	const char * name = rawsigname(sig);

	if (name == NULL) {			/* manufacture a name */
		(void) sprintf(sig_name, "SIG#%d", sig);
		name = sig_name;
	}

	return name;
}

/* return the name of the fault */
/* return NULL if unknown fault */
const char *
rawfltname(int flt)
{
	const char * name;

	switch (flt) {
	case FLTILL:	name = "FLTILL";	break;
	case FLTPRIV:	name = "FLTPRIV";	break;
	case FLTBPT:	name = "FLTBPT";	break;
	case FLTTRACE:	name = "FLTTRACE";	break;
	case FLTACCESS:	name = "FLTACCESS";	break;
	case FLTBOUNDS:	name = "FLTBOUNDS";	break;
	case FLTIOVF:	name = "FLTIOVF";	break;
	case FLTIZDIV:	name = "FLTIZDIV";	break;
	case FLTFPE:	name = "FLTFPE";	break;
	case FLTSTACK:	name = "FLTSTACK";	break;
	case FLTPAGE:	name = "FLTPAGE";	break;
	default:	name = NULL;		break;
	}

	return name;
}

/* return the name of the fault */
/* manufacture a name if fault unknown */
const char *
fltname(int flt)
{
	const char * name = rawfltname(flt);

	if (name == NULL) {			/* manufacture a name */
		(void) sprintf(flt_name, "FLT#%d", flt);
		name = flt_name;
	}

	return name;
}
