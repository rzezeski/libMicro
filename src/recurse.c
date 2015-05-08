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
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libmicro.h"

#define	DEFD		100

static int			optd = DEFD;

int recurse2(int x, int y, char *s);

/*ARGSUSED*/
int
recurse1(int x, int y, char *s)
{
	char			str[32];

	if (x < y) {
		return (recurse2(x + 1, y, str));
	}

	return (x);
}

int
benchmark_init()
{
	lm_tsdsize = 0;

	(void) sprintf(lm_optstr, "d:");

	(void) sprintf(lm_usage,
	    "       [-d depth-limit (default = %d)]\n"
	    "notes: measures recursion performance\n",
	    DEFD);

	return (0);
}

int
benchmark_optswitch(int opt, char *optarg)
{
	switch (opt) {
	case 'd':
		optd = atoi(optarg);
		break;
	default:
		return (-1);
	}
	return (0);
}

/*ARGSUSED*/
int
benchmark(void *tsd, result_t *res)
{
	(void) recurse1(0, optd, NULL);

	return (0);
}
