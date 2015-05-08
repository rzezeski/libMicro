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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include "libmicro.h"

typedef struct {
	pthread_t 		*ts_threads;
	pthread_attr_t		*ts_attr;
	pthread_mutex_t		ts_lock;
} tsd_t;

static int				opts = 0;

int
benchmark_init()
{
	lm_defN = "pthread";

	lm_tsdsize = sizeof (tsd_t);

	(void) sprintf(lm_usage,
	    "       [-s stacksize] (specify stacksize)\n"
	    "notes: measures pthread_create\n");

	(void) sprintf(lm_optstr, "s:");

	return (0);
}

int
benchmark_optswitch(int opt, char *optarg)
{
	switch (opt) {
	case 's':
		opts = sizetoll(optarg);
		break;
	default:
		return (-1);
	}

	return (0);
}

int
benchmark_initworker(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;

	LM_CHK((ts->ts_threads = calloc(1, sizeof (pthread_t))) != NULL);
	LM_CHK(pthread_mutex_init(&ts->ts_lock, NULL) == 0);

	if (opts) {
		ts->ts_attr = malloc(sizeof (pthread_attr_t));
		LM_CHK(pthread_attr_init(ts->ts_attr) == 0);
		LM_CHK(pthread_attr_setstacksize(ts->ts_attr, opts) == 0);
	} else {
		ts->ts_attr = NULL;
	}

	return (0);
}

int
benchmark_pre(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;

	LM_CHK(pthread_mutex_lock(&ts->ts_lock) == 0);

	return (0);
}


void *
func(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;

	(void) pthread_mutex_lock(&ts->ts_lock);
	(void) pthread_mutex_unlock(&ts->ts_lock);

	return (tsd);
}

int
benchmark(void *tsd, result_t *res)
{
	tsd_t			*ts = (tsd_t *)tsd;

	LM_CHK(pthread_create(ts->ts_threads, ts->ts_attr, func, tsd) == 0);
	return (0);
}

int
benchmark_post(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;

        LM_CHK(pthread_mutex_unlock(&ts->ts_lock) == 0);
	LM_CHK(pthread_join(*ts->ts_threads, NULL) == 0);

	return (0);
}
