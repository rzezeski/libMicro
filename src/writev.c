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

#include <sys/uio.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#ifndef IOV_MAX
#define	IOV_MAX			UIO_MAXIOV
#endif

#include "libmicro.h"

typedef struct {
	int			ts_once;
	struct iovec		*ts_iov;
	int 			ts_fd;
} tsd_t;

#define	DEFF			"/dev/null"
#define	DEFS			1024
#define	DEFV			10

static char			*optf = DEFF;
static int			opts = DEFS;
static int			optv = DEFV;

int
benchmark_init()
{
	lm_tsdsize = sizeof (tsd_t);

	(void) sprintf(lm_optstr, "f:s:v:");

	(void) sprintf(lm_usage,
	    "       [-f file-to-write (default %s)]\n"
	    "       [-s buffer-size (default %d)]\n"
	    "       [-v vector-size (default %d)]\n"
	    "notes: measures writev()\n"
	    "       IOV_MAX is %d\n"
	    "       SSIZE_MAX is %ld\n",
	    DEFF, DEFS, DEFV, IOV_MAX, SSIZE_MAX);

	(void) sprintf(lm_header, "%8s %4s", "size", "vec");

	lm_defB = 1;

	return (0);
}

int
benchmark_optswitch(int opt, char *optarg)
{
	switch (opt) {
	case 'f':
		optf = optarg;
		break;
	case 's':
		opts = sizetoint(optarg);
		break;
	case 'v':
		optv = atoi(optarg);
		break;
	default:
		return (-1);
	}
	return (0);
}

int
benchmark_initbatch(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			i;

	if (ts->ts_once++ == 0) {
		LM_CHK((ts->ts_fd = open(optf, O_WRONLY)) != -1);
		ts->ts_iov = (struct iovec *)malloc(
		    optv * sizeof (struct iovec));
		for (i = 0; i < optv; i++) {
			ts->ts_iov[i].iov_base = malloc(opts);
			ts->ts_iov[i].iov_len = opts;
		}
	}

        LM_CHK(lseek(ts->ts_fd, 0, SEEK_SET) == 0);

	return (0);
}

int
benchmark(void *tsd, result_t *res)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			i;

	for (i = 0; i < lm_optB; i++) {
		LM_CHK(writev(ts->ts_fd, ts->ts_iov, optv) == opts * optv);
	}
	res->re_count = i;

	return (0);
}

char *
benchmark_result()
{
	static char		result[256];

	(void) sprintf(result, "%8d %4d", opts, optv);

	return (result);
}
