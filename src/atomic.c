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
 * benchmarks atomic add on illumos - useful for platform comparisons.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <atomic.h>

#include "libmicro.h"

int
benchmark_init()
{
	(void) sprintf(lm_usage, "note: measures atomic_add_32_nv()");

	lm_tsdsize = 0;

	return (0);
}

static unsigned int value = 0;

/*ARGSUSED*/
int
benchmark(void *tsd, result_t *res)
{
	(void) atomic_add_32_nv(&value, 1);

	return (0);
}
