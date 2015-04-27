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
 * benchmark fork
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "libmicro.h"

static barrier_t		*b;

typedef struct {
	int			ts_once;
	int			*ts_pids;
} tsd_t;

int
benchmark_init()
{
	lm_tsdsize = sizeof (tsd_t);
	(void) sprintf(lm_usage, "notes: measures fork()\n");

	return (0);
}

int
benchmark_initrun()
{
	b = barrier_create(lm_optP * lm_optT * (lm_optB + 1), 0);

	return (0);
}

int
benchmark_finirun()
{
	(void) barrier_destroy(b);

	return (0);
}

int
benchmark_initbatch(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;

	if (ts->ts_once++ == 0) {
		ts->ts_pids = (int *)malloc(lm_optB * sizeof (pid_t));
		LM_CHK(ts->ts_pids != NULL);
	}

	return (0);
}

int
benchmark(void *tsd, result_t *res)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			i;

	for (i = 0; i < lm_optB; i++) {
		LM_CHK((ts->ts_pids[i] = fork()) != -1);
		switch (ts->ts_pids[i]) {
		case 0:
			(void) barrier_queue(b, NULL);
			exit(0);
			break;
		default:
			continue;
		}
	}

	res->re_count = lm_optB;
	(void) barrier_queue(b, NULL);

	return (0);
}

int
benchmark_finibatch(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			i;

	for (i = 0; i < lm_optB; i++) {
		if (ts->ts_pids[i] > 0) {
			(void) waitpid(ts->ts_pids[i], NULL, 0);
		}
	}

	return (0);
}
