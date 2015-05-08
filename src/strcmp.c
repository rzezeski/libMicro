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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "libmicro.h"

static int unaligned = 0;
static int opts = 100;

typedef struct {
	int	ts_once;
	char 	*ts_a;
	char 	*ts_b;
	int	ts_fakegcc;
} tsd_t;

int
benchmark_init()
{

	lm_tsdsize = sizeof (tsd_t);

	(void) sprintf(lm_optstr, "s:n");

	(void) sprintf(lm_usage,
	    "       [-s string size (default %d)]\n"
	    "       [-n causes unaligned cmp]\n"
	    "notes: measures strcmp()\n",
	    opts);

	(void) sprintf(lm_header, "%8s", "size");

	return (0);
}

int
benchmark_optswitch(int opt, char *optarg)
{
	switch (opt) {
	case 'n':
		unaligned = 1;
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
benchmark_pre(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;
	static char		*demo =
	    "The quick brown fox jumps over the lazy dog.";

	if (ts->ts_once++ == 0) {
		int l = strlen(demo);
		int i;

		LM_CHK((ts->ts_a = malloc(opts + 1)) != NULL);
		LM_CHK((ts->ts_b = malloc(opts + 1 + unaligned)) != NULL);
		ts->ts_b += unaligned;

		for (i = 0; i < opts; i++) {
			ts->ts_a[i] = ts->ts_b[i] = demo[i%l];
		}
		ts->ts_a[opts] = 0;
		ts->ts_b[opts] = 0;
	}
	return (0);
}

int
benchmark(void *tsd, result_t *res)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			*sum = &ts->ts_fakegcc;
	char			*src = ts->ts_a;
	char			*src2 = ts->ts_b;

	*sum += strcmp(src, src2);

	return (0);
}

char *
benchmark_result()
{
	static char	result[256];

	if (unaligned == 0)
		(void) sprintf(result, "%8d", opts);
	else
		(void) sprintf(result, "%8d <unaligned>", opts);

	return (result);
}
