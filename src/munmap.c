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
 * Copyright 2002 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>

#include "libmicro.h"

typedef volatile char		vchar_t;

typedef struct {
	int			ts_once;
	vchar_t *		ts_map;
	vchar_t			ts_foo;
} tsd_t;

#define	DEFF			"/dev/zero"
#define	DEFL			8192

static char			*optf = DEFF;
static long long		optl = DEFL;
static int			optr = 0;
static int			optw = 0;
static int			opts = 0;
static int			fd = -1;
static int			anon = 0;

int
benchmark_init()
{
	lm_tsdsize = sizeof (tsd_t);

	(void) sprintf(lm_optstr, "f:l:rsw");

	(void) sprintf(lm_usage,
	    "       [-f file-to-map (default %s)]\n"
	    "       [-l mapping-length (default %d)]\n"
	    "       [-r] (read a byte from each page)\n"
	    "       [-w] (write a byte on each page)\n"
	    "       [-s] (use MAP_SHARED)\n"
	    "notes: measures munmap()\n",
	    DEFF, DEFL);

	(void) sprintf(lm_header, "%8s %5s", "size", "flags");

	return (0);
}

int
benchmark_optswitch(int opt, char *optarg)
{
	switch (opt) {
	case 'f':
		optf = optarg;
		anon = strcmp(optf, "MAP_ANON") == 0;
		break;
	case 'l':
		optl = sizetoll(optarg);
		break;
	case 'r':
		optr = 1;
		break;
	case 's':
		opts = 1;
		break;
	case 'w':
		optw = 1;
		break;
	default:
		return (-1);
	}
	return (0);
}

int
benchmark_initrun()
{
	if (!anon) {
		LM_CHK((fd = open(optf, O_RDWR)) != -1);
	}
	return (0);
}

int
benchmark_pre(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			j;

	if (ts->ts_once++ == 0) {
		ts->ts_map = malloc(sizeof (void *));
		LM_CHK(ts->ts_map != NULL);
	}

	if (anon) {
		ts->ts_map = mmap(NULL, optl, PROT_READ | PROT_WRITE,
		    MAP_ANON | (opts ? MAP_SHARED : MAP_PRIVATE),
		    -1, 0L);
	} else {
		ts->ts_map = mmap(NULL, optl, PROT_READ | PROT_WRITE,
		    opts ? MAP_SHARED : MAP_PRIVATE,
		    fd, 0L);
	}

	LM_CHK(ts->ts_map != MAP_FAILED);

	if (optr) {
		for (j = 0; j < optl; j += 4096) {
			ts->ts_foo += ts->ts_map[j];
		}
	}
	if (optw) {
		for (j = 0; j < optl; j += 4096) {
			ts->ts_map[j] = 1;
		}
	}

	return (0);
}

int
benchmark(void *tsd, result_t *res)
{
	tsd_t			*ts = (tsd_t *)tsd;

	LM_CHK(munmap((void *)ts->ts_map, optl) == 0);

	return (0);
}

char *
benchmark_result()
{
	static char		result[256];
	char			flags[5];

	flags[0] = anon ? 'a' : '-';
	flags[1] = optr ? 'r' : '-';
	flags[2] = optw ? 'w' : '-';
	flags[3] = opts ? 's' : '-';
	flags[4] = 0;

	(void) sprintf(result, "%8lld %5s", optl, flags);

	return (result);
}
