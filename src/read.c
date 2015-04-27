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

#ifdef linux
#define	_XOPEN_SOURCE 500
#endif

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include "libmicro.h"

typedef struct {
	char			*ts_buf;
	int			ts_fd;
} tsd_t;

#define	DEFF			"/dev/zero"
#define	DEFS			1024

static char			*optf = DEFF;
static long long		opts = DEFS;
int				optw = 0;

int
benchmark_init()
{
	lm_tsdsize = sizeof (tsd_t);

	(void) sprintf(lm_optstr, "f:s:w");

	(void) sprintf(lm_usage,
	    "       [-f file-to-read (default %s)]\n"
	    "       [-s buffer-size (default %d)]\n"
	    "       [-w (store a byte to each page after read)]\n"
	    "notes: measures read()\n",
	    DEFF, DEFS);

	(void) sprintf(lm_header, "%8s", "size");

	lm_defB = 1;

	return (0);
}

int
benchmark_optswitch(int opt, char *optarg)
{
	switch (opt) {
	case 'w':
		optw = getpagesize();
		break;
	case 'f':
		optf = optarg;
		break;
	case 's':
		opts = sizetoll(optarg);
		break;
	default:
		return (-1);
	}
	return (0);
}

int
benchmark_initrun()
{
	return (0);
}

int
benchmark_initbatch(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;

	if (ts->ts_buf == NULL) {
		LM_CHK((ts->ts_buf = malloc(opts)) != NULL);
		LM_CHK((ts->ts_fd = open(optf, O_RDONLY)) != -1);
	}

	LM_CHK(lseek(ts->ts_fd, 0, SEEK_SET) == 0);

	return (0);
}

int
benchmark(void *tsd, result_t *res)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			i;
	int			j;

	for (i = 0; i < lm_optB; i++) {
		LM_CHK(read(ts->ts_fd, ts->ts_buf, opts) == opts);
		if (optw)
			for (j = 0; j < opts; j += optw)
				ts->ts_buf[j] = 0;
	}
	res->re_count = i;

	return (0);
}

char *
benchmark_result()
{
	static char		result[256];

	(void) sprintf(result, "%8lld", opts);

	return (result);
}
