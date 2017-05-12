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
 *
 * Copyright (c) 2002, 2012, Oracle and/or its affiliates. All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/mman.h>
#ifndef __HAIKU__
#include <sys/sysmacros.h>
#endif
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
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
#define	GIGABYTE		(1024 * 1024 * 1024)

static char			*optf = DEFF;
static size_t			optl = 0;
static size_t			optp = 0;
static int			optr = 0;
static int			opts = 0;
static int			optw = 0;
static int			fd = -1;
static int			anon = 0;

static size_t			def_optl;
static size_t			def_optp;
static int			npagesizes = 1;
static size_t			*pagesizes;

int
benchmark_init()
{
	lm_tsdsize = sizeof (tsd_t);

	(void) sprintf(lm_optstr, "f:l:p:rsw");

	/*
	 * XXX disable for now because
	 * C99/_XOPEN_SOURCE=600/__EXTENSIONS__ doesn't expose this
	 * interface
	 */

/* #ifdef __sun */
/* 	/\* determine available page sizes before parsing options *\/ */
/* 	npagesizes = getpagesizes2(NULL, 0); */
/* 	if (npagesizes <= 0) { */
/* 		return (-1); */
/* 	} */
/* #endif */

	LM_CHK((pagesizes = malloc(npagesizes * sizeof (size_t))) != NULL);
	pagesizes[0] = sysconf(_SC_PAGESIZE);

	/*
	 * XXX disable for now because
	 * C99/_XOPEN_SOURCE=600/__EXTENSIONS__ doesn't expose this
	 * interface
	 */

/* #ifdef __sun */
/* 	if (getpagesizes2(pagesizes, npagesizes) != npagesizes) { */
/* 		return (-1); */
/* 	} */
/* #endif */

	/* pick a default for mapping length and page size */
	def_optp = pagesizes[0];

	/* set mapping length so we map at least 2 pages */
	def_optl = def_optp * 2;
	optp = def_optp;
	optl = def_optl;

	(void) sprintf(lm_usage,
	    "       [-f file-to-map (default %s)]\n"
	    "       [-l mapping-length (default %zu)]\n"
	    "       [-p page-size (default %zu)]\n"
	    "       [-r] (read a byte from each page)\n"
	    "       [-w] (write a byte on each page)\n"
	    "       [-s] (use MAP_SHARED)\n"
	    "notes: measures mmap()\n",
	    DEFF, def_optl, def_optp);

	(void) sprintf(lm_header, "%8s %5s %8s", "length", "flags", "pgsz");

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
	case 'p':
		optp = sizetoll(optarg);
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
	int i;

	/*
	 * If optp or optl were set by caller, make sure that they point to a
	 * valid pagesize.  If not, return an error.
	 */
	if (optp != def_optp) {
		for (i = 0; i < npagesizes; i++) {
			if (pagesizes[i] == optp) {
				break;
			}
		}
		if (i == npagesizes) {
			printf("Invalid page size: %zu\n", optp);
			exit(1);
		}
	}

	/*
	 * If the benchmark is running 32-bit, return an error if a page
	 * size >= 1G is selected.
	 */
	if ((optp >= GIGABYTE) && (sizeof (void *) == 4)) {
		printf("Page sizes >= 1G not supported on 32-bit\n");
		exit(1);
	}

	if (optl == def_optl) {
		optl = optp * 2;
	} else if (optl % optp != 0) {
		printf("map length %zu must be aligned to page size %zu\n",
		    optl, optp);
		exit(1);
	}

	if (!anon) {
		LM_CHK((fd = open(optf, O_RDWR)) != -1);
	}

	return (0);
}

int
benchmark_pre(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;

	if (ts->ts_once++ == 0) {
		ts->ts_map = malloc(sizeof (void *));
		LM_CHK(ts->ts_map != NULL);
	}

	return (0);
}

int
benchmark(void *tsd, result_t *res)
{
	tsd_t			*ts = (tsd_t *)tsd;
	size_t			j;

	/*
	 * XXX disable for now because
	 * C99/_XOPEN_SOURCE=600/__EXTENSIONS__ doesn't expose this
	 * interface
	 */

/* #ifdef __sun */
/* 	struct memcntl_mha	marg; */

/* 	marg.mha_cmd = MHA_MAPSIZE_VA; */
/* 	marg.mha_flags = 0; */
/* 	marg.mha_pagesize = optp; */
/* #endif */

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

	/*
	 * XXX disable for now because
	 * C99/_XOPEN_SOURCE=600/__EXTENSIONS__ doesn't expose this
	 * interface
	 */

/* #ifdef __sun */
/* 		if ((optp != def_optp) && */
/* 		    memcntl((caddr_t)ts->ts_map[i], optl, MC_HAT_ADVISE, */
/* 		    (caddr_t)&marg, 0, 0) == -1) { */
/* 			res->re_errors++; */
/* 			continue; */
/* 		} */
/* #endif */

	if (optr) {
		for (j = 0; j < optl; j += optp) {
			ts->ts_foo += ts->ts_map[j];
		}
	}
	if (optw) {
		for (j = 0; j < optl; j += optp) {
			ts->ts_map[j] = 1;
		}
	}

	return (0);
}

int
benchmark_post(void *tsd)
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

	(void) sprintf(result, "%8zu %5s %8zu", optl, flags, optp);

	return (result);
}
