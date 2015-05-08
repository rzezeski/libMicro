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

int
benchmark_init()
{
	lm_tsdsize = 0;

	(void) sprintf(lm_usage, "notes: measures signal()\n");

	return (0);
}

int
benchmark_initrun()
{
	struct sigaction act;

	act.sa_handler = nop;
	act.sa_flags = 0;

	(void) sigemptyset(&act.sa_mask);
	(void) sigaction(SIGUSR1, &act, NULL);

	return (0);
}

/*ARGSUSED*/
int
benchmark(void *tsd, result_t *res)
{
	int			pid;

	pid = getpid();
	(void) kill(pid, SIGUSR1);

	return (0);
}
