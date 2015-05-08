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
 * memory access time check
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include "libmicro.h"

static long	opts = 1024*1024;

typedef struct {
	long			**ts_data;
	long			ts_result;
} tsd_t;

int
benchmark_init()
{
	lm_tsdsize = sizeof (tsd_t);

	(void) sprintf(lm_optstr, "s:");

	(void) sprintf(lm_usage,
	    "       [-s size] number of bytes to "
	    " access (default %ld)\n"
	    "notes: measures \"random\" memory access times\n",
	    opts);

	(void) sprintf(lm_header, "%8s", "size");

	return (0);
}

int
benchmark_optswitch(int opt, char *optarg)
{
	switch (opt) {
	case 's':
		opts = sizetoint(optarg);
		break;
	default:
		return (-1);
	}

	return (0);
}

int
benchmark_initworker(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int i, j;

	ts->ts_data = malloc(opts);

	if (ts->ts_data == NULL) {
		return (1);
	}

	/*
	 * use lmbench style backwards stride
	 */

	for (i = 0; i < (int)(opts / sizeof (long)); i++) {
		j = i - 128;
		if (j < 0)
			j = j + opts / sizeof (long);
		ts->ts_data[i] = (long *)&(ts->ts_data[j]);
	}
	return (0);
}

int
benchmark(void *tsd, result_t *res)
{
	tsd_t			*ts = (tsd_t *)tsd;

	long **ptr = ts->ts_data;

	ptr = (long **)*ptr;
	ptr = (long **)*ptr;
	ptr = (long **)*ptr;
	ptr = (long **)*ptr;
	ptr = (long **)*ptr;
	ptr = (long **)*ptr;
	ptr = (long **)*ptr;
	ptr = (long **)*ptr;
	ptr = (long **)*ptr;
	ptr = (long **)*ptr;

	ts->ts_result = (long)*ptr;

	return (0);
}

char *
benchmark_result()
{
	static char  result[256];

	(void) sprintf(result, "%8ld ", opts);


	return (result);
}
