/*
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.illumos.org/license/CDDL.
 */

/*
 * Copyright 2015 Ryan Zezeski <ryan@zinascii.com>
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*
 * The "cascade" test case is a multiprocess/multithread batten-passing model
 * using lock primitives alone for synchronisation. Threads are arranged in a
 * ring. Each thread has two locks of its own on which it blocks, and is able
 * to manipulate the two locks belonging to the thread which follows it in the
 * ring.
 *
 * The number of threads (nthreads) is specified by the generic libMicro -P/-T
 * options. With nthreads == 1 (the default) the uncontended case can be timed.
 *
 * The main logic is generic and allows any simple blocking API to be tested.
 * The API-specific component is clearly indicated.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/mman.h>

#include "libmicro.h"

typedef struct {
	int			ts_once;
	int			ts_id;
	int			ts_us0;		/* our lock indices */
	int			ts_us1;
	int			ts_them0;	/* their lock indices */
	int			ts_them1;
} tsd_t;

static int			nthreads;

/*
 * API-specific code BEGINS here
 */

static int			opts = 0;
static int			nlocks;
static pthread_mutex_t		*locks;

int
benchmark_init()
{
	lm_tsdsize = sizeof (tsd_t);

	(void) sprintf(lm_optstr, "s");

	lm_defN = "cscd_mutex";

	(void) sprintf(lm_usage,
	    "       [-s] (force PTHREAD_PROCESS_SHARED)\n"
	    "notes: thread cascade using pthread_mutexes\n");

	return (0);
}

/*ARGSUSED*/
int
benchmark_optswitch(int opt, char *optarg)
{
	switch (opt) {
	case 's':
		opts = 1;
		break;
	default:
		return (-1);
	}
	return (0);
}

int
benchmark_initrun()
{
	int			i;
	pthread_mutexattr_t	ma;

	nthreads = lm_optP * lm_optT;
	nlocks = nthreads * 2;

	locks = (pthread_mutex_t *)mmap(NULL,
	    nlocks * sizeof (pthread_mutex_t),
	    PROT_READ | PROT_WRITE,
	    MAP_ANON | MAP_SHARED,
	    -1, 0L);
	LM_CHK(locks != MAP_FAILED);

	(void) pthread_mutexattr_init(&ma);
	if (lm_optP > 1 || opts) {
		(void) pthread_mutexattr_setpshared(&ma,
		    PTHREAD_PROCESS_SHARED);
	} else {
		(void) pthread_mutexattr_setpshared(&ma,
		    PTHREAD_PROCESS_PRIVATE);
	}

	for (i = 0; i < nlocks; i++) {
		(void) pthread_mutex_init(&locks[i], &ma);
	}

	return (0);
}

void
block(int index)
{
	LM_CHK(pthread_mutex_lock(&locks[index]) == 0);
}

void
unblock(int index)
{
	LM_CHK(pthread_mutex_unlock(&locks[index]) == 0);
}

/*
 * API-specific code ENDS here
 */

int
benchmark_pre(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;

	if (ts->ts_once == 0) {
		int us, them;

		us = (getpindex() * lm_optT) + gettindex();
		them = (us + 1) % (lm_optP * lm_optT);

		ts->ts_id = us;

		/* lock index asignment for us and them */
		ts->ts_us0 = (us * 2);
		ts->ts_us1 = (us * 2) + 1;
		if (us < nthreads - 1) {
			/* straight-thru connection to them */
			ts->ts_them0 = (them * 2);
			ts->ts_them1 = (them * 2) + 1;
		} else {
			/* cross-over connection to them */
			ts->ts_them0 = (them * 2) + 1;
			ts->ts_them1 = (them * 2);
		}

		ts->ts_once = 1;
	}

	/* block their first move */
	block(ts->ts_them0);
	return (0);
}

int
benchmark(void *tsd, result_t *res)
{
	tsd_t			*ts = (tsd_t *)tsd;

	/* wait to be unblocked (id == 0 will not block) */
	block(ts->ts_us0);

	/* allow them to block us again */
	unblock(ts->ts_us0);

	/* block their next + 1 move */
	block(ts->ts_them1);

	/* unblock their next move */
	unblock(ts->ts_them0);

	/* wait for them to unblock us */
	block(ts->ts_us1);

	/* repeat with locks reversed */
	unblock(ts->ts_us1);
	block(ts->ts_them0);
	unblock(ts->ts_them1);
	block(ts->ts_us0);

	/* finish batch with nothing blocked */
	unblock(ts->ts_them0);
	unblock(ts->ts_us0);
	return (0);
}
