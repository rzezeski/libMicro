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
 */

/*
 * Sample clock_gettime().
 */
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "libmicro.h"

static char			*clocks[] = {"MONOTONIC",
					     "PROCESS",
					     "REALTIME",
					     "THREAD",
					     NULL};
enum clocks {
	MONOTONIC=0,
	PROCESS,
	REALTIME,
	THREAD
};

#define	DEFC			MONOTONIC
static int			optc=DEFC;
static clockid_t		clk;

int lookup(char *x, char *names[]);

int
benchmark_init()
{
	sprintf(lm_optstr, "c:");
	sprintf(lm_usage,
	    "       [-c clock (MONOTONIC* | PROCESS | REALTIME | THREAD)\n"
	    "note: measures cock_gettime()");
	lm_tsdsize = 0;
	return (0);
}

int
benchmark_optswitch(int opt, char *optarg)
{
	int i;

	switch (opt) {
	case 'c':
		i = lookup(optarg, clocks);
		if (i == -1)
			return (-1);
		optc = i;
		break;
	default:
		return (-1);
	}

	switch (optc) {
	case MONOTONIC:
		/*
		 * System-wide clock measuring real time from some
		 * arbitrary point in the past. Cannot be adjusted by
		 * by clock_settime() and cannot jump backwards.
		 */
		clk = CLOCK_MONOTONIC;
		break;

	case PROCESS:
		/*
		 * CPU-time clock associated with the calling process.
		 */
		clk = CLOCK_PROCESS_CPUTIME_ID;
		break;

	case REALTIME:
		/*
		 * System-wide clock measuring real time since Unix
		 * epoch.
		 */
		clk = CLOCK_REALTIME;
		break;

	case THREAD:
		/*
		 * CPU-time clock associated with the calling thread.
		 */
		clk = CLOCK_THREAD_CPUTIME_ID;
		break;
	}

	return (0);
}

int
benchmark(void *tsd, result_t *res)
{
	struct timespec t;
	clock_gettime(clk, &t);
	return (0);
}

/*
 * TODO factor this out to common helper code.
 */
int
lookup(char *x, char *names[])
{
	int			i = 0;

	while (names[i] != NULL) {
		if (strcmp(names[i], x) == 0) {
			return (i);
		}
		i++;
	}
	return (-1);
}
