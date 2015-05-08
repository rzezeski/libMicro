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
	pid_t			ts_pid;
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
	b = barrier_create(lm_optP * lm_optT * 1, 0);

	return (0);
}

int
benchmark_finirun()
{
	(void) barrier_destroy(b);

	return (0);
}

int
benchmark(void *tsd, result_t *res)
{
	tsd_t			*ts = (tsd_t *)tsd;

	LM_CHK((ts->ts_pid = fork()) != -1);
	switch (ts->ts_pid) {
	case 0:
		(void) barrier_queue(b, NULL);
		exit(0);
		break;
	default:
		break;
	}

	(void) barrier_queue(b, NULL);

	return (0);
}

int
benchmark_post(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;

	if (ts->ts_pid > 0) {
		(void) waitpid(ts->ts_pid, NULL, 0);
	}

	return (0);
}
