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
 * mutex
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/mman.h>

#include "libmicro.h"

static int			optt = 0;
static int			optp = 0;
static int			opth = 0;
static int			opto = 0;

pthread_mutex_t			*lock;

typedef struct {
	int			ts_once;
	pthread_mutex_t		*ts_lock;
} tsd_t;

int
benchmark_init()
{
	lm_tsdsize = sizeof (tsd_t);

	(void) sprintf(lm_usage,
	    "       [-t] (create dummy thread so we are multithreaded)\n"
	    "       [-p] (use inter-process mutex (not support everywhere))\n"
	    "       [-h usecs] (specify mutex hold time (default 0)\n"
	    "notes: measures uncontended pthread_mutex_[un,]lock\n");

	(void) sprintf(lm_optstr, "tph:o:");

	(void) sprintf(lm_header, "%8s", "holdtime");

	return (0);
}

/*ARGSUSED*/
int
benchmark_optswitch(int opt, char *optarg)
{
	switch (opt) {
	case 'p':
		optp = 1;
		break;

	case 't':
		optt = 1;
		break;

	case 'h':
		opth = sizetoint(optarg);
		break;

	case 'o':
		opto = sizetoint(optarg);
		break;

	default:
		return (-1);
	}
	return (0);
}

void *
dummy(void *arg)
{
	(void) pause();
	return (arg);
}

int
benchmark_initrun()
{
	pthread_mutexattr_t	attr;

	lock = (pthread_mutex_t *)mmap(NULL,
	    getpagesize(),
	    PROT_READ | PROT_WRITE,
	    optp?(MAP_ANON | MAP_SHARED):MAP_ANON|MAP_PRIVATE,
	    -1, 0L) + opto;

	LM_CHK(lock != MAP_FAILED);

	(void) pthread_mutexattr_init(&attr);
	if (optp)
		(void) pthread_mutexattr_setpshared(&attr,
		    PTHREAD_PROCESS_SHARED);

	LM_CHK(pthread_mutex_init(lock, &attr) == 0);

	return (0);
}

int
benchmark_initworker(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;

	if (optt) {
		pthread_t	tid;

		LM_CHK(pthread_create(&tid, NULL, dummy, NULL) == 0);
	}

	ts->ts_lock = lock;

	return (0);
}

void
spinme(uint64_t usecs)
{
	uint64_t s = getusecs();

	while (getusecs() - s < usecs)
		;
}

int
benchmark(void *tsd, result_t *res)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			i;

	for (i = 0; i < lm_optB; i ++) {

		(void) pthread_mutex_lock(ts->ts_lock);
		if (opth)
			spinme(opth);
		(void) pthread_mutex_unlock(ts->ts_lock);

	}

	res->re_count = lm_optB;

	return (0);
}

char *
benchmark_result()
{
	static char		result[256];

	(void) sprintf(result, "%8d", opth);

	return (result);
}
