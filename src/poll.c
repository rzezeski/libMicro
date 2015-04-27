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

#define	MAX(x, y)		((x) > (y) ? (x) : (y))
#define	MIN(x, y)		((x) > (y) ? (y) : (x))

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <sys/socket.h>

#include "libmicro.h"

#define	DEFN			256

static int			optn = DEFN;
static int			optr = 0;
static int			optw = 0;
static int			optx = 0;
static int			*fds;
static int			target = 0;

typedef struct pollfd		pfd_t;

typedef struct {
	int			ts_once;
	pfd_t			*ts_pfds;
} tsd_t;

int
benchmark_init()
{
	lm_tsdsize = sizeof (tsd_t);

	(void) sprintf(lm_optstr, "n:r:w:x");

	(void) sprintf(lm_usage,
	    "       [-n fds-per-thread (default %d)]\n"
	    "       [-r readable-fds (default 0)]\n"
	    "       [-w writeable-fds (default 0)]\n"
	    "       [-x] (start -r option with highest fd first; "
	    "default is lowest first)\n"
	    "notes: measures poll()\n",
	    DEFN);

	(void) sprintf(lm_header, "%8s %5s", "nfds", "flags");

	return (0);
}

int
benchmark_optswitch(int opt, char *optarg)
{
	switch (opt) {
	case 'n':
		optn = atoi(optarg);
		break;
	case 'r':
		optr = atoi(optarg);
		break;
	case 'w':
		optw = atoi(optarg);
		break;
	case 'x':
		optx = 1;
		break;
	default:
		return (-1);
	}
	return (0);
}

int
benchmark_initrun()
{
	int			i;
	int			j;
	int			pair[2];

	if (optn % 2 != 0) {
		(void) printf("ERROR: -n value must be even\n");
		optn = optr = optw = 0;
		return (-1);
	}

	if (optn < 0 || optr < 0 || optw < 0) {
		(void) printf("ERROR: -n, -r and -w values must be > 0\n");
		optn = optr = optw = 0;
		return (-1);
	}

	if (optr > optn || optw > optn) {
		(void) printf("ERROR: -r and -w values must be <= maxfd\n");
		optn = optr = optw = 0;
		return (-1);
	}

	LM_CHK((fds = (int *)malloc(optn * sizeof (int))) != NULL);
	(void) setfdlimit(optn + 10);

	for (i = 0; i < optn; i += 2) {
		LM_CHK(socketpair(PF_UNIX, SOCK_STREAM, 0, pair) != -1);
		fds[i] = MIN(pair[0], pair[1]);
		fds[i+1] = MAX(pair[0], pair[1]);
	}

	if (optx) {
		target = MIN(optr + optw, optn);
		for (i = 0, j = optn - 1; i < optr; i++, j--) {
			LM_CHK(write(fds[j+1 - (2*(j%2))], "", 1) == 1);
		}
	} else {
		target = MAX(optr, optw);
		for (i = 0; i < optr; i++) {
			LM_CHK(write(fds[i+1 - (2*(i%2))], "", 1) == 1);
		}
	}

	return (0);
}

int
benchmark_initbatch(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			i;

	if (ts->ts_once++ == 0) {
		ts->ts_pfds = (pfd_t *)malloc(optn * sizeof (pfd_t));
		LM_CHK(ts->ts_pfds != NULL);

		for (i = 0; i < optn; i++) {
			ts->ts_pfds[i].fd = fds[i];
			ts->ts_pfds[i].events = POLLIN;
		}

		for (i = 0; i < optw; i++) {
			ts->ts_pfds[i].events |= POLLOUT;
		}
	}

	return (0);
}

int
benchmark(void *tsd, result_t *res)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			i;

	for (i = 0; i < lm_optB; i++) {
		LM_CHK(poll(ts->ts_pfds, optn, 0) == target);
	}
	res->re_count = i;

	return (0);
}

char *
benchmark_result()
{
	static char		result[256];
	char			flags[4];

	flags[0] = optr ? 'r' : '-';
	flags[1] = optw ? 'w' : '-';
	flags[2] = optx ? 'x' : '-';
	flags[3] = 0;

	(void) sprintf(result, "%8d %5s", optn, flags);

	return (result);
}
