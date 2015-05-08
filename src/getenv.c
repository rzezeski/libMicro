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

/*
 * test getenv
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "libmicro.h"

#define	DEFS			100

static int			opts = DEFS;

int
benchmark_init()
{
	(void) sprintf(lm_optstr, "s:");

	lm_tsdsize = 0;

	(void) sprintf(lm_usage,
	    "       [-s search-size (default = %d)]\n"
	    "notes: measures time to search env for missing string\n",
	    DEFS);

	return (0);
}

int
benchmark_optswitch(int opt, char *optarg)
{
	switch (opt) {
	case 's':
		opts = atoi(optarg);
		break;
	default:
		return (-1);
	}
	return (0);
}

int
benchmark_initrun()
{
	extern char **		environ;
	int			i, j;

	/* count environment strings */

	for (i = 0; environ[i++]; )
		;

	/*
	 * pad to desired count
	 */

	if (opts < i)
		opts = i;

	for (j = i; j < opts; j++) {
		char buf[80];
		(void) sprintf(buf, "VAR_%d=%d", j, j);
		(void) putenv(strdup(buf));
	}

	return (0);
}

/*ARGSUSED*/
int
benchmark(void *tsd, result_t *res)
{
	char 			*search = "RUMPLSTILTSKIN";

	(void) getenv(search);
	return (0);
}
