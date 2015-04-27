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
#include <sys/socket.h>

#include "libmicro.h"

typedef struct {
	int			ts_once;
	int			*ts_fds;
} tsd_t;

#define	DEFN		256


int
benchmark_init()
{
	lm_tsdsize = sizeof (tsd_t);

	lm_defB = 256;

	(void) sprintf(lm_usage,
	    "notes: measures socketpair\n");

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
	int			i;
	tsd_t			*ts = (tsd_t *)tsd;

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
	int			i;
	tsd_t			*ts = (tsd_t *)tsd;

	res->re_count = 0;

	for (i = 0; i < lm_optB; i += 2) {
		LM_CHK(socketpair(PF_UNIX, SOCK_STREAM,
			0, &ts->ts_fds[i]) == 0);
	}
	res->re_count = lm_optB / 2;

	return (0);
}

int
benchmark_finibatch(void *tsd)
{
	int			i;
	tsd_t			*ts = (tsd_t *)tsd;

	for (i = 0; i < lm_optB; i++) {
		LM_CHK(close(ts->ts_fds[i]) == 0);
	}

	return (0);
}
