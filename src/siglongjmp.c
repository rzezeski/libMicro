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
#include <setjmp.h>

#include "libmicro.h"

typedef struct {
	jmp_buf			ts_env;
} tsd_t;

int
benchmark_init()
{
	lm_tsdsize = sizeof (tsd_t);

	lm_tsdsize = 0;

	(void) sprintf(lm_usage, "notes: measures siglongjmp()\n");

	return (0);
}

int
benchmark(void *tsd, result_t *res)
{
	tsd_t			*ts = (tsd_t *)tsd;

	volatile int i = 0;

	(void) sigsetjmp(ts->ts_env, 1);

	if (i++ < 1)
		siglongjmp(ts->ts_env, 0);

	return (0);
}
