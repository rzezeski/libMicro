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
 * benchmark open
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include "libmicro.h"

typedef struct {
	int			ts_once;
	int			*ts_fds;
} tsd_t;

#define	DEFF			"/dev/null"

static char			*optf = DEFF;

int
benchmark_init()
{
	lm_tsdsize = sizeof (tsd_t);

	lm_defB = 256;

	(void) sprintf(lm_usage,
	    "       [-f file-to-open (default %s)]\n"
	    "notes: measures open()\n",
	    DEFF);

	(void) sprintf(lm_optstr, "f:");

	return (0);
}

int
benchmark_optswitch(int opt, char *optarg)
{
	switch (opt) {
	case 'f':
		optf = optarg;
		break;
	default:
		return (-1);
	}
	return (0);
}

int
benchmark_initrun()
{
	(void) setfdlimit(lm_optB * lm_optT + 10);

	return (0);
}

int
benchmark_initbatch(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			i;

	if (ts->ts_once++ == 0) {
		ts->ts_fds = (int *)malloc(lm_optB * sizeof (int));
		LM_CHK(ts->ts_fds != NULL);
		for (i = 0; i < lm_optB; i++) {
			ts->ts_fds[i] = -1;
		}
	}

	return (0);
}

int
benchmark(void *tsd, result_t *res)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			i;

	for (i = 0; i < lm_optB; i++) {
		LM_CHK((ts->ts_fds[i] = open(optf, O_RDONLY)) != -1);
	}
	res->re_count = i;
	return (0);
}

int
benchmark_finibatch(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			i;

	for (i = 0; i < lm_optB; i++) {
		LM_CHK(close(ts->ts_fds[i]) == 0);
	}

	return (0);
}
