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
 * benchmark fcntl getfl
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>

#include "libmicro.h"

#define	DEFF			"/dev/null"

static char			*optf = DEFF;
static int			fd = -1;

int
benchmark_init()
{
	(void) sprintf(lm_optstr, "f:");
	lm_tsdsize = 0;

	(void) sprintf(lm_usage,
	    "       [-f file-to-fcntl (default %s)]\n"
	    "notes: measures fcntl()\n",
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
	default:
		return (-1);
	}
	return (0);
}

int
benchmark_initrun()
{
	LM_CHK((fd = open(optf, O_RDONLY)) != -1);
	return (0);
}

/*ARGSUSED*/
int
benchmark(void *tsd, result_t *res)
{
	int			flags;

	LM_CHK(fcntl(fd, F_GETFL, &flags) != -1);

	return (0);
}
