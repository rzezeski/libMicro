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
 * benchmark for bind... keep in mind tcp hash chain effects
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include "libmicro.h"

#define	FIRSTPORT		12345

typedef struct {
	int 			*ts_lsns;
	struct sockaddr_in 	*ts_adds;
} tsd_t;

static int optz = -0;

int
benchmark_init()
{
	lm_tsdsize = sizeof (tsd_t);

	lm_defB = 256;
	(void) sprintf(lm_optstr, "z");

	(void) sprintf(lm_usage,
	    "		[-z bind to port 0 rather than seq. number\n"
	    "notes: measures bind() on TCP");

	return (0);
}

int
benchmark_initrun()
{
	(void) setfdlimit(lm_optB * lm_optT + 10);

	return (0);
}

/*ARGSUSED*/
int
benchmark_optswitch(int opt, char *optarg)
{
	switch (opt) {
	case 'z':
		optz = 1;
		break;
	default:
		return (-1);
	}
	return (0);
}

int
benchmark_initbatch(void *tsd)
{
	tsd_t 			*ts = (tsd_t *)tsd;
	int			i;
	int			j = FIRSTPORT;
	int			opt = 1;
	struct hostent		*host;

	ts->ts_lsns = (int *)malloc(lm_optB * sizeof (int));
	LM_CHK(ts->ts_lsns != NULL);

	ts->ts_adds = (struct sockaddr_in *)malloc(lm_optB *
	    sizeof (struct sockaddr_in));
	LM_CHK(ts->ts_adds != NULL);

	for (i = 0; i < lm_optB; i++) {
		LM_CHK((ts->ts_lsns[i] =
			socket(PF_INET, SOCK_STREAM, 0)) != -1);
		LM_CHK(setsockopt(ts->ts_lsns[i], SOL_SOCKET, SO_REUSEADDR,
			&opt, sizeof (int)) != -1);
		LM_CHK((host = gethostbyname("localhost")) != NULL);

		(void) memset(&ts->ts_adds[i], 0,
		    sizeof (struct sockaddr_in));
		ts->ts_adds[i].sin_family = AF_INET;
		ts->ts_adds[i].sin_port = (optz ? 0 : htons(j++));
		(void) memcpy(&ts->ts_adds[i].sin_addr.s_addr,
		    host->h_addr_list[0], sizeof (struct in_addr));
	}
	return (0);
}

int
benchmark(void *tsd, result_t *res)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			i;

	for (i = 0; i < lm_optB; i++) {
		int r;
		r = bind(ts->ts_lsns[i], (struct sockaddr *)&ts->ts_adds[i],
		    sizeof (struct sockaddr_in));
		LM_CHK(r == 0 || errno == EADDRINUSE);
	}
	res->re_count = i;

	return (0);
}

int
benchmark_finibatch(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			i;

	for (i = 0; i < lm_optB; i++)
		LM_CHK(close(ts->ts_lsns[i]) == 0);
	return (0);
}
