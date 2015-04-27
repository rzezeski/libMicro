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
 * exec benchmark
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#include "libmicro.h"

static char exec_path[1024];
static char *argv[3];

int
benchmark_init()
{
	lm_defB = 128;
	lm_tsdsize = 0;

	(void) sprintf(lm_usage,
	    "notes: measures execv time of simple process()\n");

	return (0);
}

/*ARGSUSED*/
int
benchmark_initbatch(void *tsd)
{
	char			buffer[80];

	(void) strcpy(exec_path, lm_procpath);
	(void) strcat(exec_path, "/exec_bin");

	(void) sprintf(buffer, "%d", lm_optB);
	argv[0] = exec_path;
	argv[1] = strdup(buffer);
	argv[2] = NULL;

	return (0);
}

/*ARGSUSED*/
int
benchmark(void *tsd, result_t *res)
{
	int c;
	int status;

	LM_CHK((c = fork()) != -1);

	if (c > 0) {
		/* Parent. */
		LM_CHK(waitpid(c, &status, 0) > 0);
		LM_CHK(WIFEXITED(status) == 1);
		LM_CHK(WEXITSTATUS(status) == 0);
	} else {
		/* Child. */
		LM_CHK(execv(exec_path, argv) != -1);
	}

	res->re_count = lm_optB;

	return (0);
}
