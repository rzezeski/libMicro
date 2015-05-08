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
#include <fcntl.h>

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

#define	DEFD			"/tmp"

static char			*optd = DEFD;
static int 			file;
static int 			nlocks;

int
benchmark_init()
{
	lm_tsdsize = sizeof (tsd_t);

	(void) sprintf(lm_optstr, "d:");

	lm_defN = "cscd_fcntl";

	(void) sprintf(lm_usage,
	    "       [-d directory for temp file (default %s)]\n"
	    "notes: thread cascade using fcntl region locking\n",
	    DEFD);

	return (0);
}

int
benchmark_optswitch(int opt, char *optarg)
{
	switch (opt) {
	case 'd':
		optd = optarg;
		break;
	default:
		return (-1);
	}
	return (0);
}

int
benchmark_initrun()
{
	char			fname[1024];

	nthreads = lm_optP * lm_optT;
	nlocks = nthreads * 2;

	(void) sprintf(fname, "%s/cascade.%d", optd, getpid());

	LM_CHK((file = open(fname, O_CREAT | O_TRUNC | O_RDWR, 0600)) != -1);
	LM_CHK(unlink(fname) == 0);
	LM_CHK(ftruncate(file, nlocks * 3) == 0);

	return (0);
}

void
block(int index)
{
	struct flock		fl;

	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = index;
	fl.l_len = 1;
	LM_CHK(fcntl(file, F_SETLKW, &fl) == 0);
}

void
unblock(int index)
{
	struct flock	fl;

	fl.l_type = F_UNLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = index;
	fl.l_len = 1;
	LM_CHK(fcntl(file, F_SETLK, &fl) == 0);
}

/*
 * API-specific code ENDS here
 */

int
benchmark_pre(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;

	if (ts->ts_once == 0) {
		int		us, them;

		us = (getpindex() * lm_optT) + gettindex();
		them = (us + 1) % (lm_optP * lm_optT);

		ts->ts_id = us;

		/* lock index asignment for us and them */
		ts->ts_us0 = (us * 4);
		ts->ts_us1 = (us * 4) + 2;
		if (us < nthreads - 1) {
			/* straight-thru connection to them */
			ts->ts_them0 = (them * 4);
			ts->ts_them1 = (them * 4) + 2;
		} else {
			/* cross-over connection to them */
			ts->ts_them0 = (them * 4) + 2;
			ts->ts_them1 = (them * 4);
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
