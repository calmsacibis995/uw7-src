#ident	"@(#)ksh93:src/lib/libast/ast_lib.h	1.1"
/* : : generated from features/lib by iffe version 05/09/95 : : */
#ifndef _def_lib_ast
#define _def_lib_ast	1
#define _hdr_dirent	1	/* #include <dirent.h> ok */
#define _hdr_locale	1	/* #include <locale.h> ok */
#define _hdr_utime	1	/* #include <utime.h> ok */
#define _hdr_wchar	1	/* #include <wchar.h> ok */
#define _dat__tzname	1	/* _tzname in default lib(s) */
#define _dat_tzname	1	/* tzname in default lib(s) */
#define _lib__cleanup	1	/* _cleanup() in default lib(s) */
#define _lib_atexit	1	/* atexit() in default lib(s) */
#define _lib_confstr	1	/* confstr() in default lib(s) */
#define _lib_dup2	1	/* dup2() in default lib(s) */
#define _lib_fchmod	1	/* fchmod() in default lib(s) */
#define _lib_fcntl	1	/* fcntl() in default lib(s) */
#define _lib_fnmatch	1	/* fnmatch() in default lib(s) */
#define _lib_fork	1	/* fork() in default lib(s) */
#define _lib_fsync	1	/* fsync() in default lib(s) */
#define _lib_getdents	1	/* getdents() in default lib(s) */
#define _lib_getgroups	1	/* getgroups() in default lib(s) */
#define _lib_getrlimit	1	/* getrlimit() in default lib(s) */
#define _lib_link	1	/* link() in default lib(s) */
#define _lib_localeconv	1	/* localeconv() in default lib(s) */
#define _lib_lstat	1	/* lstat() in default lib(s) */
#define _lib_mbtowc	1	/* mbtowc() in default lib(s) */
#define _lib_memccpy	1	/* memccpy() in default lib(s) */
#define _lib_memchr	1	/* memchr() in default lib(s) */
#define _lib_memcmp	1	/* memcmp() in default lib(s) */
#define _lib_memcpy	1	/* memcpy() in default lib(s) */
#define _lib_memmove	1	/* memmove() in default lib(s) */
#define _lib_memset	1	/* memset() in default lib(s) */
#define _lib_mkdir	1	/* mkdir() in default lib(s) */
#define _lib_mkfifo	1	/* mkfifo() in default lib(s) */
#define _lib_mknod	1	/* mknod() in default lib(s) */
#define _lib_mktemp	1	/* mktemp() in default lib(s) */
#define _lib_mount	1	/* mount() in default lib(s) */
#define _lib_opendir	1	/* opendir() in default lib(s) */
#define _lib_pathconf	1	/* pathconf() in default lib(s) */
#define _lib_readlink	1	/* readlink() in default lib(s) */
#define _lib_remove	1	/* remove() in default lib(s) */
#define _lib_rename	1	/* rename() in default lib(s) */
#define _lib_rmdir	1	/* rmdir() in default lib(s) */
#define _lib_rewinddir	1	/* rewinddir() in default lib(s) */
#define _lib_setlocale	1	/* setlocale() in default lib(s) */
#define _lib_setpgid	1	/* setpgid() in default lib(s) */
#define _lib_setpgrp	1	/* setpgrp() in default lib(s) */
#define _lib_setsid	1	/* setsid() in default lib(s) */
#define _lib_setuid	1	/* setuid() in default lib(s) */
#define _lib_sigaction	1	/* sigaction() in default lib(s) */
#define _lib_sigprocmask	1	/* sigprocmask() in default lib(s) */
#define _lib_strchr	1	/* strchr() in default lib(s) */
#define _lib_strcoll	1	/* strcoll() in default lib(s) */
#define _lib_strdup	1	/* strdup() in default lib(s) */
#define _lib_strerror	1	/* strerror() in default lib(s) */
#define _lib_strrchr	1	/* strrchr() in default lib(s) */
#define _lib_strtod	1	/* strtod() in default lib(s) */
#define _lib_strtol	1	/* strtol() in default lib(s) */
#define _lib_strtoul	1	/* strtoul() in default lib(s) */
#define _lib_strxfrm	1	/* strxfrm() in default lib(s) */
#define _lib_symlink	1	/* symlink() in default lib(s) */
#define _lib_sysconf	1	/* sysconf() in default lib(s) */
#define _lib_telldir	1	/* telldir() in default lib(s) */
#define _lib_tmpnam	1	/* tmpnam() in default lib(s) */
#define _lib_tzset	1	/* tzset() in default lib(s) */
#define _lib_unlink	1	/* unlink() in default lib(s) */
#define _lib_utime	1	/* utime() in default lib(s) */
#define _lib_vfork	1	/* vfork() in default lib(s) */
#define _lib_wctype	1	/* wctype() in default lib(s) */
#define _lib_iswctype	1	/* iswctype() in default lib(s) */
#define _lib_execve	1	/* execve() in default lib(s) */
#define _sys_types	1	/* #include <sys/types.h> ok */
#define _sys_dir	1	/* #include <sys/dir.h> ok */
#define _hdr_dirent	1	/* #include <dirent.h> ok */
#define _mem_d_ino_dirent	1	/* d_ino is member of struct dirent */
#define _mem_d_off_dirent	1	/* d_off is member of struct dirent */
#define _mem_d_reclen_dirent	1	/* d_reclen is member of struct dirent */
#define _mem_dd_fd_DIR	1	/* dd_fd is member of DIR */
#define _sys_filio	1	/* #include <sys/filio.h> ok */
#define _sys_jioctl	1	/* #include <sys/jioctl.h> ok */
#define _sys_mman	1	/* #include <sys/mman.h> ok */
#define _sys_ptem	1	/* #include <sys/ptem.h> ok */
#define _sys_resource	1	/* #include <sys/resource.h> ok */
#define _sys_socket	1	/* #include <sys/socket.h> ok */
#define _sys_stream	1	/* #include <sys/stream.h> ok */
#define _tst_errno	1	/* errno can be assigned */
#define _lib_poll_fd_1	1	/* fd is first arg to poll() */

#if _lib_poll_fd_1 || _lib_poll_fd_2
#define _lib_poll	1
#endif
#if _lib_NutForkExecve
#define _map_spawnve	NutForkExecve
#else
#if _lib_pcreateve
#define _map_spawnve	pcreateve
#endif
#endif
#define _lib_select	1	/* select() has standard 5 arg interface */
#define _pipe_rw	1	/* full duplex pipes */
#define _real_vfork	1	/* vfork child shares data with parent */
#define _stream_peek	1	/* ioctl(I_PEEK) works */
#define _sys_stat	1	/* #include <sys/stat.h> ok */
#define _hdr_unistd	1	/* #include <unistd.h> ok */
#define _hdr_fcntl	1	/* #include <fcntl.h> ok */
#define _sys_mman	1	/* #include <sys/mman.h> ok */
#define _sys_times	1	/* #include <sys/times.h> ok */
#define _lib_mmap	1	/* standard mmap interface that works */
#define _ptr_dd_buf	1	/* DIR.dd_buf is a pointer */
#define _lib_utime_now	1	/* utime works with 0 time vector */
#define _UNIV_DEFAULT	"att"	/* default universe name */
#define _std_strcoll	1	/* standard strcoll works */
#endif
