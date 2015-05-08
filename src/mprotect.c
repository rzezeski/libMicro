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
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
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
	int			ts_run;
	int			ts_res;
} tsd_t;

#define	DEFF			"/dev/zero"
#define	DEFL			8192

static char			*optf = DEFF;
static long long		optl = DEFL;
static int			optr = 0;
static int			optw = 0;
static int			opts = 0;
static int			optt = 0;
static int			fd = -1;
static int			anon = 0;
static int			foo = 0;
static vchar_t			*seg;
static int			pagesize;

int
benchmark_init()
{
	lm_tsdsize = sizeof (tsd_t);

	(void) sprintf(lm_optstr, "f:l:rstw");

	(void) sprintf(lm_usage,
	    "       [-f file-to-map (default %s)]\n"
	    "       [-l mapping-length (default %d)]\n"
	    "       [-r] (read a byte from each page)\n"
	    "       [-w] (write a byte on each page)\n"
	    "       [-s] (use MAP_SHARED)\n"
	    "       [-t] (touch each page after restoring permissions)\n"
	    "notes: measures mprotect()\n",
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
	case 't':
		optt = 1;
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
	int			flags;
	int			i;

	if (!anon) {
		LM_CHK((fd = open(optf, O_RDWR)) != -1);
	}

	flags = opts ? MAP_SHARED : MAP_PRIVATE;
	flags |= anon ? MAP_ANON : 0;

	seg = (vchar_t *)mmap(NULL, optl, PROT_READ | PROT_WRITE,
	    flags, anon ? -1 : fd, 0L);
	LM_CHK(seg != MAP_FAILED);

	pagesize = getpagesize();

	if (optr) {
		for (i = 0; i < optl; i += pagesize) {
			foo += seg[i];
		}
	}

	if (optw) {
		for (i = 0; i < optl; i += pagesize) {
			seg[i] = 1;
		}
	}

	return (0);
}

int
benchmark(void *tsd, result_t *res)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			us;
	int			prot = PROT_NONE;
	int			j, k;

	us = (getpindex() * lm_optT) + gettindex();

	switch ((us + ts->ts_run) % 2) {
	case 0:
		prot = PROT_NONE;
		if (optt) {
			for (j = k = 0; j < optl; j += pagesize)
				k += seg[j];
			ts->ts_res += k;
		}
		break;
	default:
		prot = PROT_READ | PROT_WRITE;
		break;
	}

	LM_CHK(mprotect((void *)seg, optl, prot) == 0);
	ts->ts_run++;

	return (0);
}

char *
benchmark_result()
{
	static char		result[256];
	char			flags[6];

	flags[0] = anon ? 'a' : '-';
	flags[1] = optr ? 'r' : '-';
	flags[2] = optw ? 'w' : '-';
	flags[3] = opts ? 's' : '-';
	flags[4] = optt ? 't' : '-';
	flags[5] = 0;

	(void) sprintf(result, "%8lld %5s", optl, flags);

	return (result);
}
