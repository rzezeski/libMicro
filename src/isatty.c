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
 * isatty test
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include "libmicro.h"

#define	DEFF			"/dev/tty"
static char			*optf = DEFF;
static int 			optb = 0;

typedef struct {
	int			ts_fd;
} tsd_t;

int
benchmark_init()
{
	lm_tsdsize = sizeof (tsd_t);

	(void) sprintf(lm_optstr, "f:b");

	(void) sprintf(lm_usage,
	    "       [-f file-to-isatty (default %s)]\n"
	    "       [-b] (try to isatty an unopened fd)\n"
	    "notes: measures isatty()",
	    DEFF);

	return (0);
}

int
benchmark_optswitch(int opt, char *optarg)
{
	switch (opt) {
	case 'f':
		optf = optarg;
		break;
	case 'b':
		optb = 1;
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

	ts->ts_fd = ((optb == 0) ?
	    open(optf, O_RDONLY) : 1024);
	LM_CHK(ts->ts_fd != -1);

	return (0);
}

int
benchmark(void *tsd, result_t *res)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			i;

	for (i = 0; i < lm_optB; i++) {
		LM_CHK(isatty(ts->ts_fd) != -1);
	}
	res->re_count = i;

	return (0);
}
