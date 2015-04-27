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
#include <signal.h>

#include "libmicro.h"

#ifdef __sun
static void
nop()
{
}
#else
static void
nop(int sig)
{
}
#endif

typedef struct {
	struct sigaction ts_act;
} tsd_t;

int
benchmark_init()
{
	lm_tsdsize = sizeof (tsd_t);

	(void) sprintf(lm_usage, "notes: measures sigaction()\n");

	return (0);
}

int
benchmark_initbatch(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;
	ts->ts_act.sa_handler = nop;
	ts->ts_act.sa_flags = 0;
	(void) sigemptyset(&ts->ts_act.sa_mask);

	return (0);
}

int
benchmark(void *tsd, result_t *res)
{
	int			i;
	tsd_t			*ts = (tsd_t *)tsd;
	struct sigaction	oact;

	for (i = 0; i < lm_optB; i++) {
		LM_CHK(sigaction(SIGUSR1, &ts->ts_act, &oact) == 0);
	}

	res->re_count += lm_optB;

	return (0);
}
