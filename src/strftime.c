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
#include <time.h>

#include "libmicro.h"

#define	DEFF		"%c"
#define	MAXSIZE			80

static char *optf = DEFF;

typedef struct {
	int		ts_once;
	struct tm 	ts_tm1;
	struct tm 	ts_tm2;
} tsd_t;

int
benchmark_init()
{

	lm_tsdsize = sizeof (tsd_t);

	(void) sprintf(lm_optstr, "f:");

	(void) sprintf(lm_usage,
	    "       [-f format (default = \"%s\")]\n"
	    "notes: measures strftime()\n",
	    DEFF);

	(void) sprintf(lm_header, "%8s", "format");

	return (0);
}
int
benchmark_optswitch(int opt, char *optarg)
{
	switch (opt) {

	case 'f':
		optf = optarg;
		break;
	default:
		return (-1);
	}
	return (0);
}


char *
benchmark_result()
{
	static char	result[256];

	(void) sprintf(result, "%8s", optf);

	return (result);
}

int
benchmark_pre(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;

	static time_t		clock1 = 0L;
	static time_t		clock2 = 1L;

	(void) localtime_r(&clock1, &ts->ts_tm1);
	(void) localtime_r(&clock2, &ts->ts_tm2);

	return (0);
}


int
benchmark(void *tsd, result_t *res)
{
	tsd_t			*ts = (tsd_t *)tsd;
	char			s[MAXSIZE];

	(void) strftime(s, MAXSIZE, optf, &ts->ts_tm1);

	return (0);
}
