#ident	"@(#)ksh93:src/lib/libast/conflim.h	1.1"
	/*
	 * some implementations (could it beee aix) think empty
	 * definitions constitute symbolic constants
	 */

	{
	long	num;
	char*	str;
	int	hit;
	long	lim[54];

	hit = 0;
#if _lib_sysconf && defined(_SC_AIO_LISTIO_MAX)
	if ((num = sysconf(_SC_AIO_LISTIO_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	AIO_LISTIO_MAX
	if (!hit && AIO_LISTIO_MAX > 0)
	{
		hit = 1;
		num = AIO_LISTIO_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 2;
	}
#endif
	}
	if (hit) printf("#define AIO_LISTIO_MAX		%ld\n", num);
	else num = -1;
	lim[0] = num;
#ifndef AIO_LISTIO_MAX
#define AIO_LISTIO_MAX	lim[0]
#endif
#if defined(_POSIX_AIO_LISTIO_MAX)
	{
		static long	x[] = { 2, _POSIX_AIO_LISTIO_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_AIO_LISTIO_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 2;
#endif
	printf("#define _POSIX_AIO_LISTIO_MAX	%ld\n", num);
	hit = 0;
#if _lib_sysconf && defined(_SC_AIO_MAX)
	if ((num = sysconf(_SC_AIO_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	AIO_MAX
	if (!hit && AIO_MAX > 0)
	{
		hit = 1;
		num = AIO_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 1;
	}
#endif
	}
	if (hit) printf("#define AIO_MAX		%ld\n", num);
	else num = -1;
	lim[1] = num;
#ifndef AIO_MAX
#define AIO_MAX	lim[1]
#endif
#if defined(_POSIX_AIO_MAX)
	{
		static long	x[] = { 1, _POSIX_AIO_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_AIO_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 1;
#endif
	printf("#define _POSIX_AIO_MAX	%ld\n", num);
	hit = 0;
#if _lib_sysconf && defined(_SC_AIO_PRIO_DELTA_MAX)
	if ((num = sysconf(_SC_AIO_PRIO_DELTA_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	AIO_PRIO_DELTA_MAX
	if (!hit && AIO_PRIO_DELTA_MAX > 0)
	{
		hit = 1;
		num = AIO_PRIO_DELTA_MAX;
	}
#else
#endif
	}
	if (hit) printf("#define AIO_PRIO_DELTA_MAX		%ld\n", num);
	else num = -1;
	lim[2] = num;
#ifndef AIO_PRIO_DELTA_MAX
#define AIO_PRIO_DELTA_MAX	lim[2]
#endif
#if defined(_POSIX_AIO_PRIO_DELTA_MAX)
	{
		static long	x[] = { 1, _POSIX_AIO_PRIO_DELTA_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_AIO_PRIO_DELTA_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
	printf("#define _POSIX_AIO_PRIO_DELTA_MAX	%ld\n", num);
#endif
	hit = 0;
#if _lib_sysconf && defined(_SC_ARG_MAX)
	if ((num = sysconf(_SC_ARG_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	ARG_MAX
	if (!hit && ARG_MAX > 0)
	{
		hit = 1;
		num = ARG_MAX;
	}
#else
#ifdef	NCARGS
	if (!hit && NCARGS > 0)
	{
		hit = 1;
		num = NCARGS;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 4096;
	}
#endif
#endif
	}
	if (hit) printf("#define ARG_MAX		%ld\n", num);
	else num = -1;
	lim[3] = num;
#ifndef ARG_MAX
#define ARG_MAX	lim[3]
#endif
#if defined(_POSIX_ARG_MAX)
	{
		static long	x[] = { 4096, _POSIX_ARG_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_ARG_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 4096;
#endif
	printf("#define _POSIX_ARG_MAX	%ld\n", num);
	hit = 0;
#if _lib_sysconf && defined(_SC_ATEXIT_MAX)
	if ((num = sysconf(_SC_ATEXIT_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	ATEXIT_MAX
	if (!hit && ATEXIT_MAX > 0)
	{
		hit = 1;
		num = ATEXIT_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 32;
	}
#endif
	}
	if (hit) printf("#define ATEXIT_MAX		%ld\n", num);
	else num = -1;
	lim[4] = num;
#ifndef ATEXIT_MAX
#define ATEXIT_MAX	lim[4]
#endif
#if defined(_XOPEN_ATEXIT_MAX)
	{
		static long	x[] = { 32, _XOPEN_ATEXIT_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_XOPEN_ATEXIT_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 32;
#endif
	printf("#define _XOPEN_ATEXIT_MAX	%ld\n", num);
	hit = 0;
#if _lib_sysconf && defined(_SC_BC_BASE_MAX)
	if ((num = sysconf(_SC_BC_BASE_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	BC_BASE_MAX
	if (!hit && BC_BASE_MAX > 0)
	{
		hit = 1;
		num = BC_BASE_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 99;
	}
#endif
	}
	if (hit) printf("#define BC_BASE_MAX		%ld\n", num);
	else num = -1;
	lim[5] = num;
#ifndef BC_BASE_MAX
#define BC_BASE_MAX	lim[5]
#endif
#if defined(_POSIX2_BC_BASE_MAX)
	{
		static long	x[] = { 99, _POSIX2_BC_BASE_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX2_BC_BASE_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 99;
#endif
	printf("#define _POSIX2_BC_BASE_MAX	%ld\n", num);
	hit = 0;
#if _lib_sysconf && defined(_SC_BC_DIM_MAX)
	if ((num = sysconf(_SC_BC_DIM_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	BC_DIM_MAX
	if (!hit && BC_DIM_MAX > 0)
	{
		hit = 1;
		num = BC_DIM_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 2048;
	}
#endif
	}
	if (hit) printf("#define BC_DIM_MAX		%ld\n", num);
	else num = -1;
	lim[6] = num;
#ifndef BC_DIM_MAX
#define BC_DIM_MAX	lim[6]
#endif
#if defined(_POSIX2_BC_DIM_MAX)
	{
		static long	x[] = { 2048, _POSIX2_BC_DIM_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX2_BC_DIM_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 2048;
#endif
	printf("#define _POSIX2_BC_DIM_MAX	%ld\n", num);
	hit = 0;
#if _lib_sysconf && defined(_SC_BC_SCALE_MAX)
	if ((num = sysconf(_SC_BC_SCALE_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	BC_SCALE_MAX
	if (!hit && BC_SCALE_MAX > 0)
	{
		hit = 1;
		num = BC_SCALE_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 99;
	}
#endif
	}
	if (hit) printf("#define BC_SCALE_MAX		%ld\n", num);
	else num = -1;
	lim[7] = num;
#ifndef BC_SCALE_MAX
#define BC_SCALE_MAX	lim[7]
#endif
#if defined(_POSIX2_BC_SCALE_MAX)
	{
		static long	x[] = { 99, _POSIX2_BC_SCALE_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX2_BC_SCALE_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 99;
#endif
	printf("#define _POSIX2_BC_SCALE_MAX	%ld\n", num);
	hit = 0;
#if _lib_sysconf && defined(_SC_BC_STRING_MAX)
	if ((num = sysconf(_SC_BC_STRING_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	BC_STRING_MAX
	if (!hit && BC_STRING_MAX > 0)
	{
		hit = 1;
		num = BC_STRING_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 1000;
	}
#endif
	}
	if (hit) printf("#define BC_STRING_MAX		%ld\n", num);
	else num = -1;
	lim[8] = num;
#ifndef BC_STRING_MAX
#define BC_STRING_MAX	lim[8]
#endif
#if defined(_POSIX2_BC_STRING_MAX)
	{
		static long	x[] = { 1000, _POSIX2_BC_STRING_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX2_BC_STRING_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 1000;
#endif
	printf("#define _POSIX2_BC_STRING_MAX	%ld\n", num);
	hit = 0;
	{
#ifdef	CHARCLASS_NAME_MAX
	if (!hit && CHARCLASS_NAME_MAX > 0)
	{
		hit = 1;
		num = CHARCLASS_NAME_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 14;
	}
#endif
	}
	if (hit) printf("#define CHARCLASS_NAME_MAX		%ld\n", num);
	else num = -1;
	lim[9] = num;
#ifndef CHARCLASS_NAME_MAX
#define CHARCLASS_NAME_MAX	lim[9]
#endif
	hit = 0;
#if _lib_sysconf && defined(_SC_CHILD_MAX)
	if ((num = sysconf(_SC_CHILD_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	CHILD_MAX
	if (!hit && CHILD_MAX > 0)
	{
		hit = 1;
		num = CHILD_MAX;
	}
#else
#ifdef	_LOCAL_CHILD_MAX
	if (!hit && _LOCAL_CHILD_MAX > 0)
	{
		hit = 1;
		num = _LOCAL_CHILD_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 6;
	}
#endif
#endif
	}
	if (hit) printf("#define CHILD_MAX		%ld\n", num);
	else num = -1;
	lim[10] = num;
#ifndef CHILD_MAX
#define CHILD_MAX	lim[10]
#endif
#if defined(_POSIX_CHILD_MAX)
	{
		static long	x[] = { 6, _POSIX_CHILD_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_CHILD_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 6;
#endif
	printf("#define _POSIX_CHILD_MAX	%ld\n", num);
#if defined(_AST_CLK_TCK)
	{
		static long	x[] = { 60, _AST_CLK_TCK };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_AST_CLK_TCK\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 60;
#endif
	printf("#define _AST_CLK_TCK	%ld\n", num);
#if defined(_POSIX_CLOCKRES_MIN)
	{
		static long	x[] = { 1, _POSIX_CLOCKRES_MIN };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_CLOCKRES_MIN\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 1;
#endif
	printf("#define _POSIX_CLOCKRES_MIN	%ld\n", num);
	hit = 0;
#if _lib_sysconf && defined(_SC_COLL_WEIGHTS_MAX)
	if ((num = sysconf(_SC_COLL_WEIGHTS_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	COLL_WEIGHTS_MAX
	if (!hit && COLL_WEIGHTS_MAX > 0)
	{
		hit = 1;
		num = COLL_WEIGHTS_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 2;
	}
#endif
	}
	if (hit) printf("#define COLL_WEIGHTS_MAX		%ld\n", num);
	else num = -1;
	lim[11] = num;
#ifndef COLL_WEIGHTS_MAX
#define COLL_WEIGHTS_MAX	lim[11]
#endif
#if defined(_POSIX2_COLL_WEIGHTS_MAX)
	{
		static long	x[] = { 2, _POSIX2_COLL_WEIGHTS_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX2_COLL_WEIGHTS_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 2;
#endif
	printf("#define _POSIX2_COLL_WEIGHTS_MAX	%ld\n", num);
	hit = 0;
#if _lib_sysconf && defined(_SC_DELAYTIMER_MAX)
	if ((num = sysconf(_SC_DELAYTIMER_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	DELAYTIMER_MAX
	if (!hit && DELAYTIMER_MAX > 0)
	{
		hit = 1;
		num = DELAYTIMER_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 32;
	}
#endif
	}
	if (hit) printf("#define DELAYTIMER_MAX		%ld\n", num);
	else num = -1;
	lim[12] = num;
#ifndef DELAYTIMER_MAX
#define DELAYTIMER_MAX	lim[12]
#endif
#if defined(_POSIX_DELAYTIMER_MAX)
	{
		static long	x[] = { 32, _POSIX_DELAYTIMER_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_DELAYTIMER_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 32;
#endif
	printf("#define _POSIX_DELAYTIMER_MAX	%ld\n", num);
	hit = 0;
#if _lib_sysconf && defined(_SC_EXPR_NEST_MAX)
	if ((num = sysconf(_SC_EXPR_NEST_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	EXPR_NEST_MAX
	if (!hit && EXPR_NEST_MAX > 0)
	{
		hit = 1;
		num = EXPR_NEST_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 32;
	}
#endif
	}
	if (hit) printf("#define EXPR_NEST_MAX		%ld\n", num);
	else num = -1;
	lim[13] = num;
#ifndef EXPR_NEST_MAX
#define EXPR_NEST_MAX	lim[13]
#endif
#if defined(_POSIX2_EXPR_NEST_MAX)
	{
		static long	x[] = { 32, _POSIX2_EXPR_NEST_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX2_EXPR_NEST_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 32;
#endif
	printf("#define _POSIX2_EXPR_NEST_MAX	%ld\n", num);
	hit = 0;
#if _lib_sysconf && defined(_SC_FCHR_MAX)
	if ((num = sysconf(_SC_FCHR_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	FCHR_MAX
	if (!hit && FCHR_MAX > 0)
	{
		hit = 1;
		num = FCHR_MAX;
	}
#else
#ifdef	LONG_MAX
	if (!hit && LONG_MAX > 0)
	{
		hit = 1;
		num = LONG_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 2147483647;
	}
#endif
#endif
	}
	if (hit) printf("#define FCHR_MAX		%ld\n", num);
	else num = -1;
	lim[14] = num;
#ifndef FCHR_MAX
#define FCHR_MAX	lim[14]
#endif
#if defined(_SVID_FCHR_MAX)
	{
		static long	x[] = { 2147483647, _SVID_FCHR_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_SVID_FCHR_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 2147483647;
#endif
	printf("#define _SVID_FCHR_MAX	%ld\n", num);
	hit = 0;
#if _lib_sysconf && defined(_SC_IOV_MAX)
	if ((num = sysconf(_SC_IOV_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	IOV_MAX
	if (!hit && IOV_MAX > 0)
	{
		hit = 1;
		num = IOV_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 16;
	}
#endif
	}
	if (hit) printf("#define IOV_MAX		%ld\n", num);
	else num = -1;
	lim[15] = num;
#ifndef IOV_MAX
#define IOV_MAX	lim[15]
#endif
#if defined(_XOPEN_IOV_MAX)
	{
		static long	x[] = { 16, _XOPEN_IOV_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_XOPEN_IOV_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 16;
#endif
	printf("#define _XOPEN_IOV_MAX	%ld\n", num);
	hit = 0;
#if _lib_sysconf && defined(_SC_LINE_MAX)
	if ((num = sysconf(_SC_LINE_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	LINE_MAX
	if (!hit && LINE_MAX > 0)
	{
		hit = 1;
		num = LINE_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 2048;
	}
#endif
	}
	if (hit) printf("#define LINE_MAX		%ld\n", num);
	else num = -1;
	lim[16] = num;
#ifndef LINE_MAX
#define LINE_MAX	lim[16]
#endif
#if defined(_POSIX2_LINE_MAX)
	{
		static long	x[] = { 2048, _POSIX2_LINE_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX2_LINE_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 2048;
#endif
	printf("#define _POSIX2_LINE_MAX	%ld\n", num);
	hit = 0;
#if _lib_pathconf && defined(_PC_LINK_MAX)
	if ((num = pathconf("/", _PC_LINK_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	LINK_MAX
	if (!hit && LINK_MAX > 0)
	{
		hit = 1;
		num = LINK_MAX;
	}
#else
#ifdef	MAXLINK
	if (!hit && MAXLINK > 0)
	{
		hit = 1;
		num = MAXLINK;
	}
#else
#ifdef	SHRT_MAX
	if (!hit && SHRT_MAX > 0)
	{
		hit = 1;
		num = SHRT_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 8;
	}
#endif
#endif
#endif
	}
	if (hit) printf("#define LINK_MAX		%ld\n", num);
	else num = -1;
	lim[17] = num;
#ifndef LINK_MAX
#define LINK_MAX	lim[17]
#endif
#if defined(_POSIX_LINK_MAX)
	{
		static long	x[] = { 8, _POSIX_LINK_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_LINK_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 8;
#endif
	printf("#define _POSIX_LINK_MAX	%ld\n", num);
#if defined(_SVID_LOGNAME_MAX)
	{
		static long	x[] = { 8, _SVID_LOGNAME_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_SVID_LOGNAME_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 8;
#endif
	printf("#define _SVID_LOGNAME_MAX	%ld\n", num);
	hit = 0;
	{
#ifdef	LONG_BIT
	if (!hit && LONG_BIT > 0)
	{
		hit = 1;
		num = LONG_BIT;
	}
#else
#ifdef	_LOCAL_LONG_BIT
	if (!hit && _LOCAL_LONG_BIT > 0)
	{
		hit = 1;
		num = _LOCAL_LONG_BIT;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 32;
	}
#endif
#endif
	}
	if (hit) printf("#define LONG_BIT		%ld\n", num);
	else num = -1;
	lim[18] = num;
#ifndef LONG_BIT
#define LONG_BIT	lim[18]
#endif
	hit = 0;
#if _lib_pathconf && defined(_PC_MAX_CANON)
	if ((num = pathconf("/", _PC_MAX_CANON)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	MAX_CANON
	if (!hit && MAX_CANON > 0)
	{
		hit = 1;
		num = MAX_CANON;
	}
#else
#ifdef	CANBSIZ
	if (!hit && CANBSIZ > 0)
	{
		hit = 1;
		num = CANBSIZ;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 255;
	}
#endif
#endif
	}
	if (hit) printf("#define MAX_CANON		%ld\n", num);
	else num = -1;
	lim[19] = num;
#ifndef MAX_CANON
#define MAX_CANON	lim[19]
#endif
#if defined(_POSIX_MAX_CANON)
	{
		static long	x[] = { 255, _POSIX_MAX_CANON };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_MAX_CANON\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 255;
#endif
	printf("#define _POSIX_MAX_CANON	%ld\n", num);
	hit = 0;
#if _lib_pathconf && defined(_PC_MAX_INPUT)
	if ((num = pathconf("/", _PC_MAX_INPUT)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	MAX_INPUT
	if (!hit && MAX_INPUT > 0)
	{
		hit = 1;
		num = MAX_INPUT;
	}
#else
#ifdef	MAX_CANON
	if (!hit && MAX_CANON > 0)
	{
		hit = 1;
		num = MAX_CANON;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 255;
	}
#endif
#endif
	}
	if (hit) printf("#define MAX_INPUT		%ld\n", num);
	else num = -1;
	lim[20] = num;
#ifndef MAX_INPUT
#define MAX_INPUT	lim[20]
#endif
#if defined(_POSIX_MAX_INPUT)
	{
		static long	x[] = { 255, _POSIX_MAX_INPUT };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_MAX_INPUT\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 255;
#endif
	printf("#define _POSIX_MAX_INPUT	%ld\n", num);
	hit = 0;
#if _lib_sysconf && defined(_SC_MQ_OPEN_MAX)
	if ((num = sysconf(_SC_MQ_OPEN_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	MQ_OPEN_MAX
	if (!hit && MQ_OPEN_MAX > 0)
	{
		hit = 1;
		num = MQ_OPEN_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 8;
	}
#endif
	}
	if (hit) printf("#define MQ_OPEN_MAX		%ld\n", num);
	else num = -1;
	lim[21] = num;
#ifndef MQ_OPEN_MAX
#define MQ_OPEN_MAX	lim[21]
#endif
#if defined(_POSIX_MQ_OPEN_MAX)
	{
		static long	x[] = { 8, _POSIX_MQ_OPEN_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_MQ_OPEN_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 8;
#endif
	printf("#define _POSIX_MQ_OPEN_MAX	%ld\n", num);
	hit = 0;
#if _lib_sysconf && defined(_SC_MQ_PRIO_MAX)
	if ((num = sysconf(_SC_MQ_PRIO_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	MQ_PRIO_MAX
	if (!hit && MQ_PRIO_MAX > 0)
	{
		hit = 1;
		num = MQ_PRIO_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 32;
	}
#endif
	}
	if (hit) printf("#define MQ_PRIO_MAX		%ld\n", num);
	else num = -1;
	lim[22] = num;
#ifndef MQ_PRIO_MAX
#define MQ_PRIO_MAX	lim[22]
#endif
#if defined(_POSIX_MQ_PRIO_MAX)
	{
		static long	x[] = { 32, _POSIX_MQ_PRIO_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_MQ_PRIO_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 32;
#endif
	printf("#define _POSIX_MQ_PRIO_MAX	%ld\n", num);
	hit = 0;
#if _lib_pathconf && defined(_PC_NAME_MAX)
	if ((num = pathconf("/", _PC_NAME_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	NAME_MAX
	if (!hit && NAME_MAX > 0)
	{
		hit = 1;
		num = NAME_MAX;
	}
#else
#ifdef	_LOCAL_NAME_MAX
	if (!hit && _LOCAL_NAME_MAX > 0)
	{
		hit = 1;
		num = _LOCAL_NAME_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 14;
	}
#endif
#endif
	}
	if (hit) printf("#define NAME_MAX		%ld\n", num);
	else num = -1;
	lim[23] = num;
#ifndef NAME_MAX
#define NAME_MAX	lim[23]
#endif
#if defined(_POSIX_NAME_MAX)
	{
		static long	x[] = { 14, _POSIX_NAME_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_NAME_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 14;
#endif
	printf("#define _POSIX_NAME_MAX	%ld\n", num);
	hit = 0;
#if _lib_sysconf && defined(_SC_NGROUPS_MAX)
	if ((num = sysconf(_SC_NGROUPS_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	NGROUPS_MAX
	if (!hit && NGROUPS_MAX > 0)
	{
		hit = 1;
		num = NGROUPS_MAX;
	}
#else
#ifdef	_LOCAL_NGROUPS_MAX
	if (!hit && _LOCAL_NGROUPS_MAX > 0)
	{
		hit = 1;
		num = _LOCAL_NGROUPS_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 0;
	}
#endif
#endif
	}
	if (hit) printf("#define NGROUPS_MAX		%ld\n", num);
	else num = -1;
	lim[24] = num;
#ifndef NGROUPS_MAX
#define NGROUPS_MAX	lim[24]
#endif
#if defined(_POSIX_NGROUPS_MAX)
	{
		static long	x[] = { 0, _POSIX_NGROUPS_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_NGROUPS_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 0;
#endif
	printf("#define _POSIX_NGROUPS_MAX	%ld\n", num);
	hit = 0;
	{
#ifdef	NL_ARGMAX
	if (!hit && NL_ARGMAX > 0)
	{
		hit = 1;
		num = NL_ARGMAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 9;
	}
#endif
	}
	if (hit) printf("#define NL_ARGMAX		%ld\n", num);
	else num = -1;
	lim[25] = num;
#ifndef NL_ARGMAX
#define NL_ARGMAX	lim[25]
#endif
	hit = 0;
	{
#ifdef	NL_LANGMAX
	if (!hit && NL_LANGMAX > 0)
	{
		hit = 1;
		num = NL_LANGMAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 14;
	}
#endif
	}
	if (hit) printf("#define NL_LANGMAX		%ld\n", num);
	else num = -1;
	lim[26] = num;
#ifndef NL_LANGMAX
#define NL_LANGMAX	lim[26]
#endif
	hit = 0;
	{
#ifdef	NL_MSGMAX
	if (!hit && NL_MSGMAX > 0)
	{
		hit = 1;
		num = NL_MSGMAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 32767;
	}
#endif
	}
	if (hit) printf("#define NL_MSGMAX		%ld\n", num);
	else num = -1;
	lim[27] = num;
#ifndef NL_MSGMAX
#define NL_MSGMAX	lim[27]
#endif
	hit = 0;
	{
#ifdef	NL_NMAX
	if (!hit && NL_NMAX > 0)
	{
		hit = 1;
		num = NL_NMAX;
	}
#else
#endif
	}
	if (hit) printf("#define NL_NMAX		%ld\n", num);
	else num = -1;
	lim[28] = num;
#ifndef NL_NMAX
#define NL_NMAX	lim[28]
#endif
	hit = 0;
	{
#ifdef	NL_SETMAX
	if (!hit && NL_SETMAX > 0)
	{
		hit = 1;
		num = NL_SETMAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 255;
	}
#endif
	}
	if (hit) printf("#define NL_SETMAX		%ld\n", num);
	else num = -1;
	lim[29] = num;
#ifndef NL_SETMAX
#define NL_SETMAX	lim[29]
#endif
	hit = 0;
	{
#ifdef	NL_TEXTMAX
	if (!hit && NL_TEXTMAX > 0)
	{
		hit = 1;
		num = NL_TEXTMAX;
	}
#else
#ifdef	LINE_MAX
	if (!hit && LINE_MAX > 0)
	{
		hit = 1;
		num = LINE_MAX;
	}
#else
#endif
#endif
	}
	if (hit) printf("#define NL_TEXTMAX		%ld\n", num);
	else num = -1;
	lim[30] = num;
#ifndef NL_TEXTMAX
#define NL_TEXTMAX	lim[30]
#endif
	hit = 0;
	{
#ifdef	NZERO
	if (!hit && NZERO > 0)
	{
		hit = 1;
		num = NZERO;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 20;
	}
#endif
	}
	if (hit) printf("#define NZERO		%ld\n", num);
	else num = -1;
	lim[31] = num;
#ifndef NZERO
#define NZERO	lim[31]
#endif
	hit = 0;
#if _lib_sysconf && defined(_SC_OPEN_MAX)
	if ((num = sysconf(_SC_OPEN_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	OPEN_MAX
	if (!hit && OPEN_MAX > 0)
	{
		hit = 1;
		num = OPEN_MAX;
	}
#else
#ifdef	_LOCAL_OPEN_MAX
	if (!hit && _LOCAL_OPEN_MAX > 0)
	{
		hit = 1;
		num = _LOCAL_OPEN_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 16;
	}
#endif
#endif
	}
	if (hit) printf("#define OPEN_MAX		%ld\n", num);
	else num = -1;
	lim[32] = num;
#ifndef OPEN_MAX
#define OPEN_MAX	lim[32]
#endif
#if defined(_POSIX_OPEN_MAX)
	{
		static long	x[] = { 16, _POSIX_OPEN_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_OPEN_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 16;
#endif
	printf("#define _POSIX_OPEN_MAX	%ld\n", num);
	hit = 0;
	{
#ifdef	OPEN_MAX_CEIL
	if (!hit && OPEN_MAX_CEIL > 0)
	{
		hit = 1;
		num = OPEN_MAX_CEIL;
	}
#else
#ifdef	OPEN_MAX
	if (!hit && OPEN_MAX > 0)
	{
		hit = 1;
		num = OPEN_MAX;
	}
#else
#endif
#endif
	}
	if (hit) printf("#define OPEN_MAX_CEIL		%ld\n", num);
	else num = -1;
	lim[33] = num;
#ifndef OPEN_MAX_CEIL
#define OPEN_MAX_CEIL	lim[33]
#endif
	hit = 0;
#if _lib_sysconf && defined(_SC_PAGESIZE)
	if ((num = sysconf(_SC_PAGESIZE)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	PAGESIZE
	if (!hit && PAGESIZE > 0)
	{
		hit = 1;
		num = PAGESIZE;
	}
#else
#ifndef	_SC_PAGESIZE
#ifdef	_LOCAL_PAGESIZE
	if (!hit && _LOCAL_PAGESIZE > 0)
	{
		hit = 1;
		num = _LOCAL_PAGESIZE;
	}
#else
#ifdef	PAGE_SIZE
	if (!hit && PAGE_SIZE > 0)
	{
		hit = 1;
		num = PAGE_SIZE;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 4096;
	}
#endif
#endif
#endif
#endif
	}
	if (hit) printf("#define PAGESIZE		%ld\n", num);
	else num = -1;
	lim[34] = num;
#ifndef PAGESIZE
#define PAGESIZE	lim[34]
#endif
	hit = 0;
#if _lib_sysconf && defined(_SC_PAGE_SIZE)
	if ((num = sysconf(_SC_PAGE_SIZE)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	PAGE_SIZE
	if (!hit && PAGE_SIZE > 0)
	{
		hit = 1;
		num = PAGE_SIZE;
	}
#else
#ifndef	_SC_PAGE_SIZE
#ifdef	PAGESIZE
	if (!hit && PAGESIZE > 0)
	{
		hit = 1;
		num = PAGESIZE;
	}
#else
#endif
#endif
#endif
	}
	if (hit) printf("#define PAGE_SIZE		%ld\n", num);
	else num = -1;
	lim[35] = num;
#ifndef PAGE_SIZE
#define PAGE_SIZE	lim[35]
#endif
#if defined(_SVID_PASS_MAX)
	{
		static long	x[] = { 8, _SVID_PASS_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_SVID_PASS_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 8;
#endif
	printf("#define _SVID_PASS_MAX	%ld\n", num);
#if defined(_POSIX_PATH)
	{
		static char*	x[] = { "/bin:/usr/bin", _POSIX_PATH };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_PATH\n");
			str = x[0];
		}
		else str = x[1];
	}
#else
	str = "/bin:/usr/bin";
#endif
	printf("#define _POSIX_PATH	\"%s\"\n", str);
	hit = 0;
#if _lib_pathconf && defined(_PC_PATH_MAX)
	if ((num = pathconf("/", _PC_PATH_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	PATH_MAX
	if (!hit && PATH_MAX > 0)
	{
		hit = 1;
		num = PATH_MAX;
	}
#else
#ifdef	MAXPATHLEN
	if (!hit && MAXPATHLEN > 0)
	{
		hit = 1;
		num = MAXPATHLEN;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 1024;
	}
#endif
#endif
	}
	if (hit) printf("#define PATH_MAX		%ld\n", num);
	else num = -1;
	lim[36] = num;
#ifndef PATH_MAX
#define PATH_MAX	lim[36]
#endif
#if defined(_POSIX_PATH_MAX)
	{
		static long	x[] = { 256, _POSIX_PATH_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_PATH_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 256;
#endif
	printf("#define _POSIX_PATH_MAX	%ld\n", num);
	hit = 0;
#if _lib_sysconf && defined(_SC_PID_MAX)
	if ((num = sysconf(_SC_PID_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	PID_MAX
	if (!hit && PID_MAX > 0)
	{
		hit = 1;
		num = PID_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 30000;
	}
#endif
	}
	if (hit) printf("#define PID_MAX		%ld\n", num);
	else num = -1;
	lim[37] = num;
#ifndef PID_MAX
#define PID_MAX	lim[37]
#endif
#if defined(_SVID_PID_MAX)
	{
		static long	x[] = { 30000, _SVID_PID_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_SVID_PID_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 30000;
#endif
	printf("#define _SVID_PID_MAX	%ld\n", num);
	hit = 0;
#if _lib_pathconf && defined(_PC_PIPE_BUF)
	if ((num = pathconf("/", _PC_PIPE_BUF)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	PIPE_BUF
	if (!hit && PIPE_BUF > 0)
	{
		hit = 1;
		num = PIPE_BUF;
	}
#else
#ifndef	_PC_PIPE_BUF
	if (!hit)
	{
		hit = 1;
		num = 512;
	}
#endif
#endif
	}
	if (hit) printf("#define PIPE_BUF		%ld\n", num);
	else num = -1;
	lim[38] = num;
#ifndef PIPE_BUF
#define PIPE_BUF	lim[38]
#endif
#if defined(_POSIX_PIPE_BUF)
	{
		static long	x[] = { 512, _POSIX_PIPE_BUF };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_PIPE_BUF\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 512;
#endif
	printf("#define _POSIX_PIPE_BUF	%ld\n", num);
	hit = 0;
#if _lib_sysconf && defined(_SC_RE_DUP_MAX)
	if ((num = sysconf(_SC_RE_DUP_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	RE_DUP_MAX
	if (!hit && RE_DUP_MAX > 0)
	{
		hit = 1;
		num = RE_DUP_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 255;
	}
#endif
	}
	if (hit) printf("#define RE_DUP_MAX		%ld\n", num);
	else num = -1;
	lim[39] = num;
#ifndef RE_DUP_MAX
#define RE_DUP_MAX	lim[39]
#endif
#if defined(_POSIX2_RE_DUP_MAX)
	{
		static long	x[] = { 255, _POSIX2_RE_DUP_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX2_RE_DUP_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 255;
#endif
	printf("#define _POSIX2_RE_DUP_MAX	%ld\n", num);
	hit = 0;
#if _lib_sysconf && defined(_SC_RTSIG_MAX)
	if ((num = sysconf(_SC_RTSIG_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	RTSIG_MAX
	if (!hit && RTSIG_MAX > 0)
	{
		hit = 1;
		num = RTSIG_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 8;
	}
#endif
	}
	if (hit) printf("#define RTSIG_MAX		%ld\n", num);
	else num = -1;
	lim[40] = num;
#ifndef RTSIG_MAX
#define RTSIG_MAX	lim[40]
#endif
#if defined(_POSIX_RTSIG_MAX)
	{
		static long	x[] = { 8, _POSIX_RTSIG_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_RTSIG_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 8;
#endif
	printf("#define _POSIX_RTSIG_MAX	%ld\n", num);
	hit = 0;
#if _lib_sysconf && defined(_SC_SEM_NSEMS_MAX)
	if ((num = sysconf(_SC_SEM_NSEMS_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	SEM_NSEMS_MAX
	if (!hit && SEM_NSEMS_MAX > 0)
	{
		hit = 1;
		num = SEM_NSEMS_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 256;
	}
#endif
	}
	if (hit) printf("#define SEM_NSEMS_MAX		%ld\n", num);
	else num = -1;
	lim[41] = num;
#ifndef SEM_NSEMS_MAX
#define SEM_NSEMS_MAX	lim[41]
#endif
#if defined(_POSIX_SEM_NSEMS_MAX)
	{
		static long	x[] = { 256, _POSIX_SEM_NSEMS_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_SEM_NSEMS_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 256;
#endif
	printf("#define _POSIX_SEM_NSEMS_MAX	%ld\n", num);
	hit = 0;
#if _lib_sysconf && defined(_SC_SEM_VALUE_MAX)
	if ((num = sysconf(_SC_SEM_VALUE_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	SEM_VALUE_MAX
	if (!hit && SEM_VALUE_MAX > 0)
	{
		hit = 1;
		num = SEM_VALUE_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 32767;
	}
#endif
	}
	if (hit) printf("#define SEM_VALUE_MAX		%ld\n", num);
	else num = -1;
	lim[42] = num;
#ifndef SEM_VALUE_MAX
#define SEM_VALUE_MAX	lim[42]
#endif
#if defined(_POSIX_SEM_VALUE_MAX)
	{
		static long	x[] = { 32767, _POSIX_SEM_VALUE_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_SEM_VALUE_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 32767;
#endif
	printf("#define _POSIX_SEM_VALUE_MAX	%ld\n", num);
#if defined(_AST_SHELL)
	{
		static char*	x[] = { "/bin/sh", _AST_SHELL };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_AST_SHELL\n");
			str = x[0];
		}
		else str = x[1];
	}
#else
	str = "/bin/sh";
#endif
	printf("#define _AST_SHELL	\"%s\"\n", str);
	hit = 0;
#if _lib_sysconf && defined(_SC_SIGQUEUE_MAX)
	if ((num = sysconf(_SC_SIGQUEUE_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	SIGQUEUE_MAX
	if (!hit && SIGQUEUE_MAX > 0)
	{
		hit = 1;
		num = SIGQUEUE_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 32;
	}
#endif
	}
	if (hit) printf("#define SIGQUEUE_MAX		%ld\n", num);
	else num = -1;
	lim[43] = num;
#ifndef SIGQUEUE_MAX
#define SIGQUEUE_MAX	lim[43]
#endif
#if defined(_POSIX_SIGQUEUE_MAX)
	{
		static long	x[] = { 32, _POSIX_SIGQUEUE_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_SIGQUEUE_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 32;
#endif
	printf("#define _POSIX_SIGQUEUE_MAX	%ld\n", num);
	hit = 0;
	{
#ifdef	SSIZE_MAX
	if (!hit && SSIZE_MAX > 0)
	{
		hit = 1;
		num = SSIZE_MAX;
	}
#else
#ifdef	INT_MAX
	if (!hit && INT_MAX > 0)
	{
		hit = 1;
		num = INT_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 32767;
	}
#endif
#endif
	}
	if (hit) printf("#define SSIZE_MAX		%ld\n", num);
	else num = -1;
	lim[44] = num;
#ifndef SSIZE_MAX
#define SSIZE_MAX	lim[44]
#endif
#if defined(_POSIX_SSIZE_MAX)
	{
		static long	x[] = { 32767, _POSIX_SSIZE_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_SSIZE_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 32767;
#endif
	printf("#define _POSIX_SSIZE_MAX	%ld\n", num);
	hit = 0;
#if _lib_sysconf && defined(_SC_STD_BLK)
	if ((num = sysconf(_SC_STD_BLK)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	STD_BLK
	if (!hit && STD_BLK > 0)
	{
		hit = 1;
		num = STD_BLK;
	}
#else
#ifndef	_SC_STD_BLK
	if (!hit)
	{
		hit = 1;
		num = 1024;
	}
#endif
#endif
	}
	if (hit) printf("#define STD_BLK		%ld\n", num);
	else num = -1;
	lim[45] = num;
#ifndef STD_BLK
#define STD_BLK	lim[45]
#endif
#if defined(_SVID_STD_BLK)
	{
		static long	x[] = { 1024, _SVID_STD_BLK };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_SVID_STD_BLK\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 1024;
#endif
	printf("#define _SVID_STD_BLK	%ld\n", num);
	hit = 0;
#if _lib_sysconf && defined(_SC_STREAM_MAX)
	if ((num = sysconf(_SC_STREAM_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	STREAM_MAX
	if (!hit && STREAM_MAX > 0)
	{
		hit = 1;
		num = STREAM_MAX;
	}
#else
#ifdef	OPEN_MAX
	if (!hit && OPEN_MAX > 0)
	{
		hit = 1;
		num = OPEN_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 8;
	}
#endif
#endif
	}
	if (hit) printf("#define STREAM_MAX		%ld\n", num);
	else num = -1;
	lim[46] = num;
#ifndef STREAM_MAX
#define STREAM_MAX	lim[46]
#endif
#if defined(_POSIX_STREAM_MAX)
	{
		static long	x[] = { 8, _POSIX_STREAM_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_STREAM_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 8;
#endif
	printf("#define _POSIX_STREAM_MAX	%ld\n", num);
	hit = 0;
#if _lib_pathconf && defined(_PC_SYMLINK_MAX)
	if ((num = pathconf("/", _PC_SYMLINK_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	SYMLINK_MAX
	if (!hit && SYMLINK_MAX > 0)
	{
		hit = 1;
		num = SYMLINK_MAX;
	}
#else
#ifdef	_LOCAL_SYMLINK_MAX
	if (!hit && _LOCAL_SYMLINK_MAX > 0)
	{
		hit = 1;
		num = _LOCAL_SYMLINK_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 255;
	}
#endif
#endif
	}
	if (hit) printf("#define SYMLINK_MAX		%ld\n", num);
	else num = -1;
	lim[47] = num;
#ifndef SYMLINK_MAX
#define SYMLINK_MAX	lim[47]
#endif
#if defined(_POSIX_SYMLINK_MAX)
	{
		static long	x[] = { 255, _POSIX_SYMLINK_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_SYMLINK_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 255;
#endif
	printf("#define _POSIX_SYMLINK_MAX	%ld\n", num);
	hit = 0;
#if _lib_pathconf && defined(_PC_SYMLOOP_MAX)
	if ((num = pathconf("/", _PC_SYMLOOP_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	SYMLOOP_MAX
	if (!hit && SYMLOOP_MAX > 0)
	{
		hit = 1;
		num = SYMLOOP_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 8;
	}
#endif
	}
	if (hit) printf("#define SYMLOOP_MAX		%ld\n", num);
	else num = -1;
	lim[48] = num;
#ifndef SYMLOOP_MAX
#define SYMLOOP_MAX	lim[48]
#endif
#if defined(_POSIX_SYMLOOP_MAX)
	{
		static long	x[] = { 8, _POSIX_SYMLOOP_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_SYMLOOP_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 8;
#endif
	printf("#define _POSIX_SYMLOOP_MAX	%ld\n", num);
	hit = 0;
#if _lib_sysconf && defined(_SC_SYSPID_MAX)
	if ((num = sysconf(_SC_SYSPID_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	SYSPID_MAX
	if (!hit && SYSPID_MAX > 0)
	{
		hit = 1;
		num = SYSPID_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 2;
	}
#endif
	}
	if (hit) printf("#define SYSPID_MAX		%ld\n", num);
	else num = -1;
	lim[49] = num;
#ifndef SYSPID_MAX
#define SYSPID_MAX	lim[49]
#endif
#if defined(_SVID_SYSPID_MAX)
	{
		static long	x[] = { 2, _SVID_SYSPID_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_SVID_SYSPID_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 2;
#endif
	printf("#define _SVID_SYSPID_MAX	%ld\n", num);
	hit = 0;
#if _lib_sysconf && defined(_SC_TIMER_MAX)
	if ((num = sysconf(_SC_TIMER_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	TIMER_MAX
	if (!hit && TIMER_MAX > 0)
	{
		hit = 1;
		num = TIMER_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 32;
	}
#endif
	}
	if (hit) printf("#define TIMER_MAX		%ld\n", num);
	else num = -1;
	lim[50] = num;
#ifndef TIMER_MAX
#define TIMER_MAX	lim[50]
#endif
#if defined(_POSIX_TIMER_MAX)
	{
		static long	x[] = { 32, _POSIX_TIMER_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_TIMER_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 32;
#endif
	printf("#define _POSIX_TIMER_MAX	%ld\n", num);
#if defined(_AST_TMP)
	{
		static char*	x[] = { "/tmp", _AST_TMP };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_AST_TMP\n");
			str = x[0];
		}
		else str = x[1];
	}
#else
	str = "/tmp";
#endif
	printf("#define _AST_TMP	\"%s\"\n", str);
	hit = 0;
#if _lib_sysconf && defined(_SC_TZNAME_MAX)
	if ((num = sysconf(_SC_TZNAME_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	TZNAME_MAX
	if (!hit && TZNAME_MAX > 0)
	{
		hit = 1;
		num = TZNAME_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 6;
	}
#endif
	}
	if (hit) printf("#define TZNAME_MAX		%ld\n", num);
	else num = -1;
	lim[51] = num;
#ifndef TZNAME_MAX
#define TZNAME_MAX	lim[51]
#endif
#if defined(_POSIX_TZNAME_MAX)
	{
		static long	x[] = { 6, _POSIX_TZNAME_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_POSIX_TZNAME_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 6;
#endif
	printf("#define _POSIX_TZNAME_MAX	%ld\n", num);
	hit = 0;
#if _lib_sysconf && defined(_SC_UID_MAX)
	if ((num = sysconf(_SC_UID_MAX)) != -1)
		hit = 1;
	else
#endif
	{
#ifdef	UID_MAX
	if (!hit && UID_MAX > 0)
	{
		hit = 1;
		num = UID_MAX;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 60002;
	}
#endif
	}
	if (hit) printf("#define UID_MAX		%ld\n", num);
	else num = -1;
	lim[52] = num;
#ifndef UID_MAX
#define UID_MAX	lim[52]
#endif
#if defined(_SVID_UID_MAX)
	{
		static long	x[] = { 60002, _SVID_UID_MAX };
		if ((sizeof(x)/sizeof(x[0])) == 1)
		{
			printf("#undef	_SVID_UID_MAX\n");
			num = x[0];
		}
		else num = x[1];
	}
#else
	num = 60002;
#endif
	printf("#define _SVID_UID_MAX	%ld\n", num);
	hit = 0;
	{
#ifdef	WORD_BIT
	if (!hit && WORD_BIT > 0)
	{
		hit = 1;
		num = WORD_BIT;
	}
#else
#ifdef	_LOCAL_WORD_BIT
	if (!hit && _LOCAL_WORD_BIT > 0)
	{
		hit = 1;
		num = _LOCAL_WORD_BIT;
	}
#else
	if (!hit)
	{
		hit = 1;
		num = 16;
	}
#endif
#endif
	}
	if (hit) printf("#define WORD_BIT		%ld\n", num);
	else num = -1;
	lim[53] = num;
#ifndef WORD_BIT
#define WORD_BIT	lim[53]
#endif
	}
