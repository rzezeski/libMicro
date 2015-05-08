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
 * getrusage
 */


#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/resource.h>

#include "libmicro.h"

int
benchmark_init()
{
	(void) sprintf(lm_usage, "notes: measures getrusage(RUSAGE_SELF)\n");
	lm_tsdsize = 0;
	return (0);
}

/*ARGSUSED*/
int
benchmark(void *tsd, result_t *res)
{
	struct rusage		u;

	(void) getrusage(RUSAGE_SELF, &u);

	return (0);
}
