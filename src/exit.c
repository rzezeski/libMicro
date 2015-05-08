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
 * benchmark exit
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "libmicro.h"

typedef struct {
	int			ts_once;
	/* should be ts_pid */
	int			*ts_pids;
} tsd_t;

static int			opte = 0;
static barrier_t		*b;

int
benchmark_init()
{
	lm_tsdsize = sizeof (tsd_t);
	(void) sprintf(lm_optstr, "e");

	(void) sprintf(lm_usage,
	    "       [-e] (uses _exit() rather than exit())"
	    "notes: measures exit()\n");

	return (0);
}

/*ARGSUSED*/
int
benchmark_optswitch(int opt, char *optarg)
{
	switch (opt) {
	case 'e':
		opte = 1;
		break;
	default:
		return (-1);
	}

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
benchmark_pre(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;

	if (ts->ts_once++ == 0) {
		ts->ts_pids = (int *)malloc(sizeof (pid_t));
		LM_CHK(ts->ts_pids != NULL);
	}

	/*
	 * create processes to exit
	 */

	LM_CHK((ts->ts_pids[0] = fork()) != -1);
	switch (ts->ts_pids[0]) {
	case 0:
		(void) barrier_queue(b, NULL);
		if (opte)
			_exit(0);
		exit(0);
		break;
	default:
		break;
	}

	return (0);
}

/*ARGSUSED*/
int
benchmark(void *tsd, result_t *res)
{
	/*
	 * Start them all exiting.
	 */
	(void) barrier_queue(b, NULL);
	LM_CHK(waitpid((pid_t)-1, NULL, 0) != -1);

	return (0);
}
