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
 * test file locking
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "libmicro.h"

static int			file;

void
block(int index)
{
	struct flock		fl;

	fl.l_type = F_WRLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = index;
	fl.l_len = 1;
	LM_CHK(fcntl(file, F_SETLKW, &fl) != -1);
}

void
unblock(int index)
{
	struct flock		fl;

	fl.l_type = F_UNLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = index;
	fl.l_len = 1;
	LM_CHK(fcntl(file, F_SETLK, &fl) != -1);
}

int
benchmark_init()
{
	char			fname[80];

	(void) sprintf(fname, "/tmp/oneflock.%d", getpid());

	file = open(fname, O_CREAT | O_TRUNC | O_RDWR, 0600);

	LM_CHK(file != -1);
	LM_CHK(unlink(fname) == 0);
	lm_tsdsize = 0;

	return (0);
}

/*ARGSUSED*/
int
benchmark(void *tsd, result_t *res)
{
	int			i;

	for (i = 0; i < lm_optB; i ++) {
		block(0);
		unblock(0);
	}
	res->re_count = i;

	return (0);
}
