/*
 * Copyright 2002 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifdef	__sun
#pragma ident	"@(#)mmap.c	1.6	05/08/04 SMI"
#endif

#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <strings.h>

#include "libmicro.h"

typedef volatile char		vchar_t;

typedef struct {
	int			ts_once;
	vchar_t **		ts_map;
	vchar_t 		ts_foo;
} tsd_t;

#define	DEFF			"/dev/zero"
#define	DEFL			8192

static char			*optf = DEFF;
static long long		optl = DEFL;
static int			optr = 0;
static int			opts = 0;
static int			optw = 0;
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
	    "notes: measures mmap()\n",
	    DEFF, DEFL);

	(void) sprintf(lm_header, "%8s %5s", "length", "flags");

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
	if (!anon)
		fd = open(optf, O_RDWR);

	return (0);
}

int
benchmark_initbatch(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			errors = 0;

	if (ts->ts_once++ == 0) {
		ts->ts_map = (vchar_t **)malloc(lm_optB * sizeof (void *));
		if (ts->ts_map == NULL) {
			errors++;
		}
	}

	return (errors);
}

int
benchmark(void *tsd, result_t *res)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			i, j;

	for (i = 0; i < lm_optB; i++) {
		if (anon) {
			ts->ts_map[i] = (vchar_t *)mmap(NULL, optl,
			    PROT_READ | PROT_WRITE,
			    MAP_ANON | (opts ? MAP_SHARED : MAP_PRIVATE),
			    -1, 0L);
		} else {
			ts->ts_map[i] = (vchar_t *)mmap(NULL, optl,
			    PROT_READ | PROT_WRITE,
			    opts ? MAP_SHARED : MAP_PRIVATE,
			    fd, 0L);
		}

		if (ts->ts_map[i] == MAP_FAILED) {
			res->re_errors++;
			continue;
		}

		if (optr) {
			for (j = 0; j < optl; j += 4096) {
				ts->ts_foo += ts->ts_map[i][j];
			}
		}
		if (optw) {
			for (j = 0; j < optl; j += 4096) {
				ts->ts_map[i][j] = 1;
			}
		}
	}
	res->re_count = i;

	return (0);
}

int
benchmark_finibatch(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			i;

	for (i = 0; i < lm_optB; i++) {
		(void) munmap((void *)ts->ts_map[i], optl);
	}
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
