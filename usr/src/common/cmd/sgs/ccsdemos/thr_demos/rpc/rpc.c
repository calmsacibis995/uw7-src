#ident	"@(#)ccsdemos:thr_demos/rpc/rpc.c	1.1"
/*
 * This is a simple program which creates a requested number of
 * threads. Each thread then makes Remote Procedure Calls to a
 * server specified by the environment variable SVC_HOST. The
 * transport used is specified by the environment variable NETID.
 *
 * The flag passed to "thr_create()" specifies that a new Light
 * Weight Process be created for each thread. This is exessive
 * for multiplexed threads, and only a limited number of LWPs
 * should normally be used.
 *
 * Note that the thread safe library call "rpc_call()" has not
 * been made available in the TLP3 release, so this program is
 * for source reference only.
 */

#include <thread.h>
#include <stdlib.h>
#include <rpc/rpc.h>
#include <rpc/rpcent.h>
#include <rpc/nettype.h>

#define EXP_INT_RSLT	1

static	int	success;
static	int	total;
char		*host;
int		loop_count;

/*
 * This function simply makes an rpc call to the program SVC_PROG on the
 * server specifies by the global string "host".
 */
int
test_func()
{
	enum	clnt_stat	stat;
        int			result;

	/*
	 * make the rpc call.
	 */
        stat = rpc_call(host, SVC_PROG, SVC_VERS, INT_PROC, xdr_void, 0,
                        xdr_int, (char *)&result, getenv("NETID"));

	/*
	 * check for success.
	 */
	if (stat != RPC_SUCCESS)
                return (1);

	/*
	 * check the result.
	 */
        if (result == EXP_INT_RSLT)
                return (0);
        else
                return (1);
}

/*
 * This is where each thread starts. Note that the global variable
 * "success" is not protected against simultaneous update, but this
 * should be done by using a spin lock.
 */
void *
template(void *arg)
{
	int	local_success = 0;
	int	i;

	for (i = 0; i < loop_count; i++) {
		if (test_func() == 0 && test_func() == 0)
			local_success++;
	}

	if (local_success == loop_count)
		success++;
	
        thr_exit(0);
}

main(int argc, char **argv)
{
	thread_t	thrid;
        static	char	hostname[100];
	int		thr_count;
	int		error;
	int		i;

	if (argc != 3) {
		printf("%s: USAGE: %s <thread#> <loop#>\n",
						argv[0], argv[0]);
		exit (1);
	}

	if ((thr_count = atoi(argv[1])) <= 0) {
		printf("%s: <thread#> should be > 0\n", argv[0]);
		exit (1);
	}

	if ((loop_count = atoi(argv[2])) <= 0) {
		printf("%s: <loop#> should be > 0\n", argv[0]);
		exit (1);
	}

        if ((host = getenv("SVC_HOST")) == NULL) {
                gethostname(hostname, 100);
                host = hostname;
        }
        if (!host || !(*host)) {
                printf("%s: can't get a host name\n", argv[0]);
                exit(1);
        }

	/*
	 * create the needed threads.
	 */
	for (i = thr_count ; i > 0; i--) {
                for (;;) {
                        error = thr_create(NULL, 0, template, NULL,
                                	THR_NEW_LWP, &thrid);
                        if (error) {
				printf("%s: could not create thread\n",
                                                                argv[0]);
                                _lwp_wait(0, 0);
                        } else {
                                break;
			}
                }
                total++;
	}

	/*
	 * wait for all threads to exit.
	 */
	for (i = thr_count; i > 0; i--) {
		thr_join(NULL, NULL, NULL);
	}

	if (success != thr_count) {
		printf("%s: %d (%d) Failed.\n", argv[0], total-success, total);
		exit (1);
	} else {
		printf("%s: Passed.\n", argv[0]);
		exit (0);
	}
}

