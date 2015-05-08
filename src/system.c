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

#include "libmicro.h"

#define	DEFB		10
#define	DEFC		"A=$$"

static char		*optc = DEFC;

int
benchmark_init()
{
	lm_tsdsize = 0;

	(void) sprintf(lm_optstr, "c:");

	(void) sprintf(lm_usage,
	    "       [-c command (default %s)]\n"
	    "notes: measures system()\n",
	    DEFC);

	(void) sprintf(lm_header, "%8s", "command");

	return (0);
}

int
benchmark_optswitch(int opt, char *optarg)
{
	switch (opt) {
	case 'c':
		optc = optarg;
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
	LM_CHK(system(optc) == 0);

	return (0);
}

char *
benchmark_result()
{
	static char	result[256];

	(void) sprintf(result, "%8s", optc);

	return (result);
}
