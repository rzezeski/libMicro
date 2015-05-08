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
#include <signal.h>

#include "libmicro.h"

int
benchmark_init()
{
	lm_tsdsize = 0;

	(void) sprintf(lm_usage, "notes: measures sigprocmask()\n");

	return (0);
}

int
benchmark_initrun()
{
	sigset_t		iset;

	(void) sigemptyset(&iset);
	(void) sigprocmask(SIG_SETMASK, &iset, NULL);

	return (0);
}
/*ARGSUSED*/
int
benchmark(void *tsd, result_t *res)
{
	sigset_t		set0, set1;

	(void) sigemptyset(&set0);
	(void) sigaddset(&set0, SIGTERM);
	(void) sigprocmask(SIG_SETMASK, &set0, &set1);

	return (0);
}
