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

	lm_defB = 256;

	(void) sprintf(lm_usage,
	    "notes: measures close() on local TCP connections");

	return (0);
}

int
benchmark_initrun()
{
	(void) setfdlimit(3 * lm_optB * lm_optT + 10);

	return (0);
}

int
benchmark_initworker(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			i;
	int			j = FIRSTPORT;
	int			opt = 1;
	struct hostent		*host;

	ts->ts_lsns = (int *)malloc(lm_optB * sizeof (int));
	LM_CHK(ts->ts_lsns != NULL);

	ts->ts_accs = (int *)malloc(lm_optB * sizeof (int));
	LM_CHK(ts->ts_accs != NULL);

	ts->ts_cons = (int *)malloc(lm_optB * sizeof (int));
	LM_CHK(ts->ts_cons != NULL);

	ts->ts_adds = (struct sockaddr_in *)malloc(lm_optB *
	    sizeof (struct sockaddr_in));
	LM_CHK(ts->ts_adds != NULL);

	for (i = 0; i < lm_optB; i++) {
		ts->ts_lsns[i] = socket(AF_INET, SOCK_STREAM, 0);
		LM_CHK(ts->ts_lsns[i] != -1);

		LM_CHK(setsockopt(ts->ts_lsns[i], SOL_SOCKET, SO_REUSEADDR,
			&opt, sizeof (int)) != -1);

		LM_CHK((host = gethostbyname("localhost")) != NULL);

		for (;;) {
			(void) memset(&ts->ts_adds[i], 0,
			    sizeof (struct sockaddr_in));
			ts->ts_adds[i].sin_family = AF_INET;
			ts->ts_adds[i].sin_port = htons(j++);
			(void) memcpy(&ts->ts_adds[i].sin_addr.s_addr,
			    host->h_addr_list[0], sizeof (struct in_addr));

			if (bind(ts->ts_lsns[i],
			    (struct sockaddr *)&ts->ts_adds[i],
			    sizeof (struct sockaddr_in)) == 0) {
				break;
			}

			LM_CHK(errno == EADDRINUSE);
		}

		LM_CHK(listen(ts->ts_lsns[i], 5) == 0);
	}
	return (0);
}

int
benchmark_initbatch(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			i;
	int			result;
	struct sockaddr_in	addr;
	socklen_t		size;

	for (i = 0; i < lm_optB; i++) {
		ts->ts_cons[i] = socket(AF_INET, SOCK_STREAM, 0);
		LM_CHK(ts->ts_cons[i] != -1);
		LM_CHK(fcntl(ts->ts_cons[i], F_SETFL, O_NDELAY) == 0);

		result = connect(ts->ts_cons[i],
		    (struct sockaddr *)&ts->ts_adds[i],
		    sizeof (struct sockaddr_in));

		LM_CHK((result == -1) && (errno == EINPROGRESS));

		size = sizeof (struct sockaddr);
		result = accept(ts->ts_lsns[i], (struct sockaddr *)&addr,
		    &size);
		LM_CHK(result != -1);
		ts->ts_accs[i] = result;
	}

	return (0);
}

int
benchmark(void *tsd, result_t *res)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			i;

	for (i = 0; i < lm_optB; i++) {
		LM_CHK(close(ts->ts_accs[i]) == 0);
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
		LM_CHK(close(ts->ts_cons[i]) == 0);
	}

	return (0);
}

int
benchmark_finiworker(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			i;

	for (i = 0; i < lm_optB; i++) {
		LM_CHK(close(ts->ts_lsns[i]) == 0);
	}

	return (0);
}
