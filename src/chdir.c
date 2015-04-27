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

/*
 * change directory benchmark
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "libmicro.h"

#define	DEFAULTDIR		"/"
#define	MAXPATHLEN		1024

static int			optg = 0;

static int			dircount;
static char **			dirlist;

int
benchmark_init()
{
	(void) sprintf(lm_optstr, "g");
	lm_tsdsize = 0;

	(void) sprintf(lm_usage,
	    "       [-g] (do getcwd() also)\n"
	    "       directory ... (default = %s)\n"
	    "notes: measures chdir() and (optionally) getcwd()",
	    DEFAULTDIR);

	(void) sprintf(lm_header, "%5s %5s", "dirs", "gets");

	return (0);
}

/*ARGSUSED*/
int
benchmark_optswitch(int opt, char *optarg)
{
	switch (opt) {
	case 'g':
		optg = 1;
		break;
	default:
		return (-1);
	}
	return (0);
}

int
benchmark_initrun()
{
	extern int		optind;
	int			i;

	dircount = lm_argc - optind;
	if (dircount <= 0) {
		dirlist = (char **)malloc(sizeof (char *));
		dirlist[0] = DEFAULTDIR;
		dircount = 1;
	} else {
		dirlist = (char **)malloc(dircount * sizeof (char *));
		LM_CHK(dirlist != NULL);
		for (i = 0; i < dircount; i++) {
			dirlist[i] = lm_argv[optind++];
		}
	}

	return (0);
}

/*ARGSUSED*/
int
benchmark(void *tsd, result_t *res)
{
	int			i, j = 0;
	char			buf[MAXPATHLEN];

	for (i = 0; i < lm_optB; i++) {
		LM_CHK(chdir(dirlist[j]) == 0);
		j++;
		j %= dircount;

		if (optg) {
			LM_CHK(getcwd(buf, MAXPATHLEN) != NULL);
		}
	}

	res->re_count = i;

	return (0);
}

char *
benchmark_result()
{
	static char		result[256];

	(void) sprintf(result, "%5d %5s", dircount, optg ? "y" : "n");

	return (result);
}
