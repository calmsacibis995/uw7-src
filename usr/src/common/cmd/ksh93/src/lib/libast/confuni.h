#ident	"@(#)ksh93:src/lib/libast/confuni.h	1.1"
	/*
	 * some implementations (could it beee aix) think empty
	 * definitions constitute symbolic constants
	 */

	{
	long	num;
	char*	str;
	int	hit;
	printf("#undef	_SC_AIO_LISTIO_MAX\n");
	printf("#define _SC_AIO_LISTIO_MAX	(-1)\n");
	printf("#undef	_SC_AIO_MAX\n");
	printf("#define _SC_AIO_MAX	(-2)\n");
	printf("#undef	_SC_AIO_PRIO_DELTA_MAX\n");
	printf("#define _SC_AIO_PRIO_DELTA_MAX	(-3)\n");
	printf("#undef	_SC_ARG_MAX\n");
	printf("#define _SC_ARG_MAX	(-4)\n");
	printf("#undef	_SC_ASYNCHRONOUS_IO\n");
	printf("#define _SC_ASYNCHRONOUS_IO	(-5)\n");
#if defined(_POSIX_ASYNCHRONOUS_IO)
	{
		static long	x[] = { 1, _POSIX_ASYNCHRONOUS_IO };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_ASYNCHRONOUS_IO\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX_ASYNCHRONOUS_IO	%ld\n", num);
#endif
	printf("#undef	_PC_ASYNC_IO\n");
	printf("#define _PC_ASYNC_IO	(-6)\n");
#if defined(_POSIX_ASYNC_IO)
	{
		static long	x[] = { 1, _POSIX_ASYNC_IO };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_ASYNC_IO\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX_ASYNC_IO	%ld\n", num);
#endif
	printf("#undef	_SC_ATEXIT_MAX\n");
	printf("#define _SC_ATEXIT_MAX	(-82)\n");
	printf("#undef	_SC_AVPHYS_PAGES\n");
	printf("#define _SC_AVPHYS_PAGES	(-121)\n");
	printf("#undef	_SC_BC_BASE_MAX\n");
	printf("#define _SC_BC_BASE_MAX	(-7)\n");
	printf("#undef	_SC_BC_DIM_MAX\n");
	printf("#define _SC_BC_DIM_MAX	(-8)\n");
	printf("#undef	_SC_BC_SCALE_MAX\n");
	printf("#define _SC_BC_SCALE_MAX	(-9)\n");
	printf("#undef	_SC_BC_STRING_MAX\n");
	printf("#define _SC_BC_STRING_MAX	(-10)\n");
	printf("#undef	_SC_2_CHAR_TERM\n");
	printf("#define _SC_2_CHAR_TERM	(-16)\n");
#if defined(_POSIX2_CHAR_TERM)
	{
		static long	x[] = { 1, _POSIX2_CHAR_TERM };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX2_CHAR_TERM\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX2_CHAR_TERM	%ld\n", num);
#endif
	printf("#undef	_SC_CHILD_MAX\n");
	printf("#define _SC_CHILD_MAX	(-17)\n");
	printf("#undef	_PC_CHOWN_RESTRICTED\n");
	printf("#define _PC_CHOWN_RESTRICTED	(-18)\n");
#if defined(_POSIX_CHOWN_RESTRICTED)
	{
		static long	x[] = { 1, _POSIX_CHOWN_RESTRICTED };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_CHOWN_RESTRICTED\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX_CHOWN_RESTRICTED	%ld\n", num);
#endif
	printf("#undef	_SC_CKPT\n");
	printf("#define _SC_CKPT	(-97)\n");
#if defined(_POSIX_CKPT)
	{
		static long	x[] = { 1, _POSIX_CKPT };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_CKPT\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX_CKPT	%ld\n", num);
#endif
	printf("#undef	_SC_CLK_TCK\n");
	printf("#define _SC_CLK_TCK	(-19)\n");
	printf("#undef	_SC_CLOCKRES_MIN\n");
	printf("#define _SC_CLOCKRES_MIN	(-20)\n");
	printf("#undef	_SC_COLL_WEIGHTS_MAX\n");
	printf("#define _SC_COLL_WEIGHTS_MAX	(-21)\n");
	printf("#undef	_SC_XOPEN_CRYPT\n");
	printf("#define _SC_XOPEN_CRYPT	(-116)\n");
#if defined(_XOPEN_CRYPT)
	{
		static long	x[] = { 1, _XOPEN_CRYPT };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_XOPEN_CRYPT\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _XOPEN_CRYPT	%ld\n", num);
#endif
	printf("#undef	_SC_2_C_BIND\n");
	printf("#define _SC_2_C_BIND	(-11)\n");
#if defined(_POSIX2_C_BIND)
	{
		static long	x[] = { 1, _POSIX2_C_BIND };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX2_C_BIND\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX2_C_BIND	%ld\n", num);
#endif
	printf("#undef	_SC_2_C_DEV\n");
	printf("#define _SC_2_C_DEV	(-12)\n");
#if defined(_POSIX2_C_DEV)
	{
		static long	x[] = { 1, _POSIX2_C_DEV };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX2_C_DEV\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX2_C_DEV	%ld\n", num);
#endif
	printf("#undef	_SC_2_C_VERSION\n");
	printf("#define _SC_2_C_VERSION	(-98)\n");
#if defined(_POSIX2_C_VERSION)
	{
		static long	x[] = { 1, _POSIX2_C_VERSION };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX2_C_VERSION\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX2_C_VERSION	%ld\n", num);
#endif
	printf("#undef	_SC_DELAYTIMER_MAX\n");
	printf("#define _SC_DELAYTIMER_MAX	(-22)\n");
	printf("#undef	_SC_XOPEN_ENH_I18N\n");
	printf("#define _SC_XOPEN_ENH_I18N	(-117)\n");
#if defined(_XOPEN_ENH_I18N)
	{
		static long	x[] = { 1, _XOPEN_ENH_I18N };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_XOPEN_ENH_I18N\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _XOPEN_ENH_I18N	%ld\n", num);
#endif
	printf("#undef	_SC_EXPR_NEST_MAX\n");
	printf("#define _SC_EXPR_NEST_MAX	(-23)\n");
	printf("#undef	_SC_FCHR_MAX\n");
	printf("#define _SC_FCHR_MAX	(-24)\n");
	printf("#undef	_SC_2_FORT_DEV\n");
	printf("#define _SC_2_FORT_DEV	(-25)\n");
#if defined(_POSIX2_FORT_DEV)
	{
		static long	x[] = { 1, _POSIX2_FORT_DEV };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX2_FORT_DEV\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX2_FORT_DEV	%ld\n", num);
#endif
	printf("#undef	_SC_2_FORT_RUN\n");
	printf("#define _SC_2_FORT_RUN	(-26)\n");
#if defined(_POSIX2_FORT_RUN)
	{
		static long	x[] = { 1, _POSIX2_FORT_RUN };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX2_FORT_RUN\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX2_FORT_RUN	%ld\n", num);
#endif
	printf("#undef	_SC_FSYNC\n");
	printf("#define _SC_FSYNC	(-27)\n");
#if defined(_POSIX_FSYNC)
	{
		static long	x[] = { 1, _POSIX_FSYNC };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_FSYNC\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX_FSYNC	%ld\n", num);
#endif
	printf("#undef	_SC_IOV_MAX\n");
	printf("#define _SC_IOV_MAX	(-100)\n");
	printf("#undef	_SC_JOB_CONTROL\n");
	printf("#define _SC_JOB_CONTROL	(-30)\n");
#if defined(_POSIX_JOB_CONTROL)
	{
		static long	x[] = { 1, _POSIX_JOB_CONTROL };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_JOB_CONTROL\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX_JOB_CONTROL	%ld\n", num);
#endif
	printf("#undef	_SC_LINE_MAX\n");
	printf("#define _SC_LINE_MAX	(-31)\n");
	printf("#undef	_PC_LINK_MAX\n");
	printf("#define _PC_LINK_MAX	(-32)\n");
	printf("#undef	_SC_LOCALEDEF\n");
	printf("#define _SC_LOCALEDEF	(-99)\n");
#if defined(_POSIX_LOCALEDEF)
	{
		static long	x[] = { 1, _POSIX_LOCALEDEF };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_LOCALEDEF\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX_LOCALEDEF	%ld\n", num);
#endif
	printf("#undef	_SC_2_LOCALEDEF\n");
	printf("#define _SC_2_LOCALEDEF	(-33)\n");
#if defined(_POSIX2_LOCALEDEF)
	{
		static long	x[] = { 1, _POSIX2_LOCALEDEF };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX2_LOCALEDEF\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX2_LOCALEDEF	%ld\n", num);
#endif
	printf("#undef	_SC_LOGNAME_MAX\n");
	printf("#define _SC_LOGNAME_MAX	(-129)\n");
	printf("#undef	_SC_MAPPED_FILES\n");
	printf("#define _SC_MAPPED_FILES	(-36)\n");
#if defined(_POSIX_MAPPED_FILES)
	{
		static long	x[] = { 1, _POSIX_MAPPED_FILES };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_MAPPED_FILES\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX_MAPPED_FILES	%ld\n", num);
#endif
	printf("#undef	_PC_MAX_CANON\n");
	printf("#define _PC_MAX_CANON	(-37)\n");
	printf("#undef	_PC_MAX_INPUT\n");
	printf("#define _PC_MAX_INPUT	(-38)\n");
	printf("#undef	_SC_MEMLOCK\n");
	printf("#define _SC_MEMLOCK	(-40)\n");
#if defined(_POSIX_MEMLOCK)
	{
		static long	x[] = { 1, _POSIX_MEMLOCK };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_MEMLOCK\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX_MEMLOCK	%ld\n", num);
#endif
	printf("#undef	_SC_MEMLOCK_RANGE\n");
	printf("#define _SC_MEMLOCK_RANGE	(-41)\n");
#if defined(_POSIX_MEMLOCK_RANGE)
	{
		static long	x[] = { 1, _POSIX_MEMLOCK_RANGE };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_MEMLOCK_RANGE\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX_MEMLOCK_RANGE	%ld\n", num);
#endif
	printf("#undef	_SC_MEMORY_PROTECTION\n");
	printf("#define _SC_MEMORY_PROTECTION	(-42)\n");
#if defined(_POSIX_MEMORY_PROTECTION)
	{
		static long	x[] = { 1, _POSIX_MEMORY_PROTECTION };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_MEMORY_PROTECTION\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX_MEMORY_PROTECTION	%ld\n", num);
#endif
	printf("#undef	_SC_MESSAGE_PASSING\n");
	printf("#define _SC_MESSAGE_PASSING	(-43)\n");
#if defined(_POSIX_MESSAGE_PASSING)
	{
		static long	x[] = { 1, _POSIX_MESSAGE_PASSING };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_MESSAGE_PASSING\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX_MESSAGE_PASSING	%ld\n", num);
#endif
	printf("#undef	_SC_MQ_OPEN_MAX\n");
	printf("#define _SC_MQ_OPEN_MAX	(-44)\n");
	printf("#undef	_SC_MQ_PRIO_MAX\n");
	printf("#define _SC_MQ_PRIO_MAX	(-45)\n");
	printf("#undef	_SC_NACLS_MAX\n");
	printf("#define _SC_NACLS_MAX	(-122)\n");
	printf("#undef	_PC_NAME_MAX\n");
	printf("#define _PC_NAME_MAX	(-46)\n");
	printf("#undef	_SC_NGROUPS_MAX\n");
	printf("#define _SC_NGROUPS_MAX	(-47)\n");
	printf("#undef	_PC_NO_TRUNC\n");
	printf("#define _PC_NO_TRUNC	(-48)\n");
#if defined(_POSIX_NO_TRUNC)
	{
		static long	x[] = { 1, _POSIX_NO_TRUNC };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_NO_TRUNC\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX_NO_TRUNC	%ld\n", num);
#endif
	printf("#undef	_SC_NPROCESSORS_CONF\n");
	printf("#define _SC_NPROCESSORS_CONF	(-123)\n");
	printf("#undef	_SC_NPROCESSORS_ONLN\n");
	printf("#define _SC_NPROCESSORS_ONLN	(-124)\n");
	printf("#undef	_SC_OPEN_MAX\n");
	printf("#define _SC_OPEN_MAX	(-49)\n");
	printf("#undef	_SC_AES_OS_VERSION\n");
	printf("#define _SC_AES_OS_VERSION	(-119)\n");
#if defined(_AES_OS_VERSION)
	{
		static long	x[] = { 1, _AES_OS_VERSION };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_AES_OS_VERSION\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _AES_OS_VERSION	%ld\n", num);
#endif
	printf("#undef	_SC_PAGESIZE\n");
	printf("#define _SC_PAGESIZE	(-51)\n");
	printf("#undef	_SC_PAGE_SIZE\n");
	printf("#define _SC_PAGE_SIZE	(-101)\n");
	printf("#undef	_SC_PASS_MAX\n");
	printf("#define _SC_PASS_MAX	(-130)\n");
	printf("#undef	_CS_PATH\n");
	printf("#define _CS_PATH	(-52)\n");
	printf("#undef	_PC_PATH_MAX\n");
	printf("#define _PC_PATH_MAX	(-53)\n");
	printf("#undef	_SC_PHYS_PAGES\n");
	printf("#define _SC_PHYS_PAGES	(-125)\n");
	printf("#undef	_SC_PID_MAX\n");
	printf("#define _SC_PID_MAX	(-54)\n");
	printf("#undef	_PC_PIPE_BUF\n");
	printf("#define _PC_PIPE_BUF	(-55)\n");
	printf("#undef	_SC_PRIORITIZED_IO\n");
	printf("#define _SC_PRIORITIZED_IO	(-57)\n");
#if defined(_POSIX_PRIORITIZED_IO)
	{
		static long	x[] = { 1, _POSIX_PRIORITIZED_IO };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_PRIORITIZED_IO\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX_PRIORITIZED_IO	%ld\n", num);
#endif
	printf("#undef	_SC_PRIORITY_SCHEDULING\n");
	printf("#define _SC_PRIORITY_SCHEDULING	(-58)\n");
#if defined(_POSIX_PRIORITY_SCHEDULING)
	{
		static long	x[] = { 1, _POSIX_PRIORITY_SCHEDULING };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_PRIORITY_SCHEDULING\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX_PRIORITY_SCHEDULING	%ld\n", num);
#endif
	printf("#undef	_PC_PRIO_IO\n");
	printf("#define _PC_PRIO_IO	(-59)\n");
#if defined(_POSIX_PRIO_IO)
	{
		static long	x[] = { 1, _POSIX_PRIO_IO };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_PRIO_IO\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX_PRIO_IO	%ld\n", num);
#endif
	printf("#undef	_SC_REALTIME_SIGNALS\n");
	printf("#define _SC_REALTIME_SIGNALS	(-60)\n");
#if defined(_POSIX_REALTIME_SIGNALS)
	{
		static long	x[] = { 1, _POSIX_REALTIME_SIGNALS };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_REALTIME_SIGNALS\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX_REALTIME_SIGNALS	%ld\n", num);
#endif
	printf("#undef	_SC_REGEXP\n");
	printf("#define _SC_REGEXP	(-95)\n");
#if defined(_POSIX_REGEXP)
	{
		static long	x[] = { 1, _POSIX_REGEXP };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_REGEXP\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX_REGEXP	%ld\n", num);
#endif
	printf("#undef	_SC_RESOURCE_LIMITS\n");
	printf("#define _SC_RESOURCE_LIMITS	(-96)\n");
#if defined(_POSIX_RESOURCE_LIMITS)
	{
		static long	x[] = { 1, _POSIX_RESOURCE_LIMITS };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_RESOURCE_LIMITS\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX_RESOURCE_LIMITS	%ld\n", num);
#endif
	printf("#undef	_SC_RE_DUP_MAX\n");
	printf("#define _SC_RE_DUP_MAX	(-61)\n");
	printf("#undef	_SC_RTSIG_MAX\n");
	printf("#define _SC_RTSIG_MAX	(-62)\n");
	printf("#undef	_SC_SAVED_IDS\n");
	printf("#define _SC_SAVED_IDS	(-63)\n");
#if defined(_POSIX_SAVED_IDS)
	{
		static long	x[] = { 1, _POSIX_SAVED_IDS };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_SAVED_IDS\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX_SAVED_IDS	%ld\n", num);
#endif
	printf("#undef	_SC_SEMAPHORES\n");
	printf("#define _SC_SEMAPHORES	(-66)\n");
#if defined(_POSIX_SEMAPHORES)
	{
		static long	x[] = { 1, _POSIX_SEMAPHORES };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_SEMAPHORES\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX_SEMAPHORES	%ld\n", num);
#endif
	printf("#undef	_SC_SEM_NSEMS_MAX\n");
	printf("#define _SC_SEM_NSEMS_MAX	(-67)\n");
	printf("#undef	_SC_SEM_VALUE_MAX\n");
	printf("#define _SC_SEM_VALUE_MAX	(-68)\n");
	printf("#undef	_SC_SHARED_MEMORY_OBJECTS\n");
	printf("#define _SC_SHARED_MEMORY_OBJECTS	(-69)\n");
#if defined(_POSIX_SHARED_MEMORY_OBJECTS)
	{
		static long	x[] = { 1, _POSIX_SHARED_MEMORY_OBJECTS };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_SHARED_MEMORY_OBJECTS\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX_SHARED_MEMORY_OBJECTS	%ld\n", num);
#endif
	printf("#undef	_CS_SHELL\n");
	printf("#define _CS_SHELL	(-120)\n");
	printf("#undef	_SC_XOPEN_SHM\n");
	printf("#define _SC_XOPEN_SHM	(-118)\n");
#if defined(_XOPEN_SHM)
	{
		static long	x[] = { 1, _XOPEN_SHM };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_XOPEN_SHM\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _XOPEN_SHM	%ld\n", num);
#endif
	printf("#undef	_SC_SIGQUEUE_MAX\n");
	printf("#define _SC_SIGQUEUE_MAX	(-72)\n");
	printf("#undef	_SC_SIGRT_MAX\n");
	printf("#define _SC_SIGRT_MAX	(-126)\n");
	printf("#undef	_SC_SIGRT_MIN\n");
	printf("#define _SC_SIGRT_MIN	(-127)\n");
	printf("#undef	_SC_STD_BLK\n");
	printf("#define _SC_STD_BLK	(-74)\n");
	printf("#undef	_SC_STREAM_MAX\n");
	printf("#define _SC_STREAM_MAX	(-75)\n");
	printf("#undef	_SC_2_SW_DEV\n");
	printf("#define _SC_2_SW_DEV	(-76)\n");
#if defined(_POSIX2_SW_DEV)
	{
		static long	x[] = { 1, _POSIX2_SW_DEV };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX2_SW_DEV\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX2_SW_DEV	%ld\n", num);
#endif
	printf("#undef	_PC_SYMLINK_MAX\n");
	printf("#define _PC_SYMLINK_MAX	(-93)\n");
	printf("#undef	_PC_SYMLOOP_MAX\n");
	printf("#define _PC_SYMLOOP_MAX	(-94)\n");
	printf("#undef	_SC_SYNCHRONIZED_IO\n");
	printf("#define _SC_SYNCHRONIZED_IO	(-77)\n");
#if defined(_POSIX_SYNCHRONIZED_IO)
	{
		static long	x[] = { 1, _POSIX_SYNCHRONIZED_IO };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_SYNCHRONIZED_IO\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX_SYNCHRONIZED_IO	%ld\n", num);
#endif
	printf("#undef	_PC_SYNC_IO\n");
	printf("#define _PC_SYNC_IO	(-78)\n");
#if defined(_POSIX_SYNC_IO)
	{
		static long	x[] = { 1, _POSIX_SYNC_IO };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_SYNC_IO\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX_SYNC_IO	%ld\n", num);
#endif
	printf("#undef	_SC_SYSPID_MAX\n");
	printf("#define _SC_SYSPID_MAX	(-79)\n");
	printf("#undef	_SC_TIMERS\n");
	printf("#define _SC_TIMERS	(-80)\n");
#if defined(_POSIX_TIMERS)
	{
		static long	x[] = { 1, _POSIX_TIMERS };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_TIMERS\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX_TIMERS	%ld\n", num);
#endif
	printf("#undef	_SC_TIMER_MAX\n");
	printf("#define _SC_TIMER_MAX	(-81)\n");
	printf("#undef	_CS_TMP\n");
	printf("#define _CS_TMP	(-131)\n");
	printf("#undef	_SC_TMP_MAX\n");
	printf("#define _SC_TMP_MAX	(-128)\n");
	printf("#undef	_SC_TZNAME_MAX\n");
	printf("#define _SC_TZNAME_MAX	(-83)\n");
	printf("#undef	_SC_UID_MAX\n");
	printf("#define _SC_UID_MAX	(-85)\n");
	printf("#undef	_SC_XOPEN_UNIX\n");
	printf("#define _SC_XOPEN_UNIX	(-115)\n");
#if defined(_XOPEN_UNIX)
	{
		static long	x[] = { 1, _XOPEN_UNIX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_XOPEN_UNIX\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _XOPEN_UNIX	%ld\n", num);
#endif
	printf("#undef	_SC_2_UPE\n");
	printf("#define _SC_2_UPE	(-89)\n");
#if defined(_POSIX2_UPE)
	{
		static long	x[] = { 1, _POSIX2_UPE };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX2_UPE\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX2_UPE	%ld\n", num);
#endif
	printf("#undef	_PC_VDISABLE\n");
	printf("#define _PC_VDISABLE	(-90)\n");
#if defined(_POSIX_VDISABLE)
	{
		static long	x[] = { 1, _POSIX_VDISABLE };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_VDISABLE\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX_VDISABLE	%ld\n", num);
#endif
	printf("#undef	_SC_VERSION\n");
	printf("#define _SC_VERSION	(-91)\n");
#if defined(_POSIX_VERSION)
	{
		static long	x[] = { 199009, _POSIX_VERSION };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_VERSION\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 199009;
#endif
	printf("#define _POSIX_VERSION	%ld\n", num);
	printf("#undef	_SC_2_VERSION\n");
	printf("#define _SC_2_VERSION	(-92)\n");
#if defined(_POSIX2_VERSION)
	{
		static long	x[] = { 199209, _POSIX2_VERSION };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX2_VERSION\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 199209;
#endif
	printf("#define _POSIX2_VERSION	%ld\n", num);
	printf("#undef	_SC_XOPEN_VERSION\n");
	printf("#define _SC_XOPEN_VERSION	(-110)\n");
#if defined(_XOPEN_VERSION)
	{
		static long	x[] = { 4, _XOPEN_VERSION };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_XOPEN_VERSION\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 4;
#endif
	printf("#define _XOPEN_VERSION	%ld\n", num);
	printf("#undef	_SC_XOPEN_XCU_VERSION\n");
	printf("#define _SC_XOPEN_XCU_VERSION	(-111)\n");
#if defined(_XOPEN_XCU_VERSION)
	{
		static long	x[] = { 1, _XOPEN_XCU_VERSION };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_XOPEN_XCU_VERSION\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _XOPEN_XCU_VERSION	%ld\n", num);
#endif
#if defined(_XOPEN_XPG2)
	{
		static long	x[] = { 1, _XOPEN_XPG2 };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_XOPEN_XPG2\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _XOPEN_XPG2	%ld\n", num);
#endif
#if defined(_XOPEN_XPG3)
	{
		static long	x[] = { 1, _XOPEN_XPG3 };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_XOPEN_XPG3\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _XOPEN_XPG3	%ld\n", num);
#endif
#if defined(_XOPEN_XPG4)
	{
		static long	x[] = { 1, _XOPEN_XPG4 };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_XOPEN_XPG4\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _XOPEN_XPG4	%ld\n", num);
#endif
	}
