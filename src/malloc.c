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
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*
 * malloc benchmark (crude)
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include "libmicro.h"

static int		optg = 100;
static int		opts[32] = {32};
static int		optscnt = 0;

typedef struct {
	void 			**ts_glob;
} tsd_t;

int
benchmark_init()
{
	lm_tsdsize = sizeof (tsd_t);

	(void) sprintf(lm_optstr, "s:g:");

	(void) sprintf(lm_usage,
	    "       [-g number of mallocs before free (default %d)]\n"
	    "       [-s size to malloc (default %d)."
	    "  Up to 32 sizes accepted\n"
	    "notes: measures malloc()/free()",
	    optg, opts[0]);

	(void) sprintf(lm_header, "%6s %6s", "glob", "sizes");

	return (0);
}

int
benchmark_optswitch(int opt, char *optarg)
{
	switch (opt) {
	case 'g':
		optg = sizetoint(optarg);
		break;
	case 's':
		opts[optscnt++] = sizetoint(optarg);
		optscnt = optscnt & (31);
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

	if (optscnt == 0)
		optscnt = 1;

	LM_CHK((ts->ts_glob = malloc(sizeof (void *)* optg)) != NULL);
	return (0);
}

int
benchmark(void *tsd, result_t *res)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			i, j = 0, k;

	for (i = 0; i < lm_optB; i++) {
		for (k = j = 0; j < optg; j++) {
			LM_CHK((ts->ts_glob[j] = malloc(opts[k++])) != NULL);
			if (k >= optscnt)
				k = 0;
		}
		for (j = 0; j < optg; j++) {
			free(ts->ts_glob[j]);
		}
	}

	res->re_count = i * j;

	return (0);
}

char *
benchmark_result()
{
	static char  result[256];
	int i;

	(void) sprintf(result, "%6d ", optg);

	for (i = 0; i < optscnt; i++)
		(void) sprintf(result + strlen(result), "%d ", opts[i]);
	return (result);
}
