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
 * benchmark to measure time to close a local tcp connection
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
	int			*ts_lsns;
	int			*ts_accs;
	int			*ts_cons;
	struct sockaddr_in	*ts_adds;
} tsd_t;

int
benchmark_init()
{
	lm_tsdsize = sizeof (tsd_t);

	(void) sprintf(lm_usage,
	    "notes: measures close() on local TCP connections");

	return (0);
}

int
benchmark_initrun()
{
	(void) setfdlimit(3 * lm_optT + 10);

	return (0);
}

int
benchmark_initworker(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			j = FIRSTPORT;
	int			opt = 1;
	struct hostent		*host;

	ts->ts_lsns = (int *)malloc(sizeof (int));
	LM_CHK(ts->ts_lsns != NULL);

	ts->ts_accs = (int *)malloc(sizeof (int));
	LM_CHK(ts->ts_accs != NULL);

	ts->ts_cons = (int *)malloc(sizeof (int));
	LM_CHK(ts->ts_cons != NULL);

	ts->ts_adds = malloc(sizeof (struct sockaddr_in));
	LM_CHK(ts->ts_adds != NULL);

	ts->ts_lsns[0] = socket(AF_INET, SOCK_STREAM, 0);
	LM_CHK(ts->ts_lsns[0] != -1);

	LM_CHK(setsockopt(ts->ts_lsns[0], SOL_SOCKET, SO_REUSEADDR,
		&opt, sizeof (int)) != -1);

	LM_CHK((host = gethostbyname("localhost")) != NULL);

	for (;;) {
		(void) memset(&ts->ts_adds[0], 0,
		    sizeof (struct sockaddr_in));
		ts->ts_adds[0].sin_family = AF_INET;
		ts->ts_adds[0].sin_port = htons(j++);
		(void) memcpy(&ts->ts_adds[0].sin_addr.s_addr,
		    host->h_addr_list[0], sizeof (struct in_addr));

		if (bind(ts->ts_lsns[0],
			(struct sockaddr *)&ts->ts_adds[0],
			sizeof (struct sockaddr_in)) == 0) {
			break;
		}

		LM_CHK(errno == EADDRINUSE);
	}

	LM_CHK(listen(ts->ts_lsns[0], 5) == 0);

	return (0);
}

int
benchmark_pre(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			result;
	struct sockaddr_in	addr;
	socklen_t		size;

	ts->ts_cons[0] = socket(AF_INET, SOCK_STREAM, 0);
	LM_CHK(ts->ts_cons[0] != -1);
	LM_CHK(fcntl(ts->ts_cons[0], F_SETFL, O_NDELAY) == 0);

	result = connect(ts->ts_cons[0],
	    (struct sockaddr *)&ts->ts_adds[0],
	    sizeof (struct sockaddr_in));

	LM_CHK((result == -1) && (errno == EINPROGRESS));

	size = sizeof (struct sockaddr);
	result = accept(ts->ts_lsns[0], (struct sockaddr *)&addr,
	    &size);
	LM_CHK(result != -1);
	ts->ts_accs[0] = result;

	return (0);
}

int
benchmark(void *tsd, result_t *res)
{
	tsd_t			*ts = (tsd_t *)tsd;

	LM_CHK(close(ts->ts_accs[0]) == 0);

	return (0);
}

int
benchmark_post(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;

	LM_CHK(close(ts->ts_cons[0]) == 0);

	return (0);
}

int
benchmark_finiworker(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;

	LM_CHK(close(ts->ts_lsns[0]) == 0);

	return (0);
}
