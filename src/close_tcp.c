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
	int			ts_lsn;
	int			ts_acc;
	int			ts_conn;
	struct sockaddr_in	ts_add;
	int			ts_runs;
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

	ts->ts_runs = 0;

	ts->ts_lsn = socket(AF_INET, SOCK_STREAM, 0);
	LM_CHK(ts->ts_lsn != -1);

	LM_CHK(setsockopt(ts->ts_lsn, SOL_SOCKET, SO_REUSEADDR,
		&opt, sizeof (int)) != -1);

	LM_CHK((host = gethostbyname("localhost")) != NULL);

	for (;;) {
		(void) memset(&ts->ts_add, 0, sizeof (struct sockaddr_in));
		ts->ts_add.sin_family = AF_INET;
		ts->ts_add.sin_port = htons(j++);
		(void) memcpy(&ts->ts_add.sin_addr.s_addr,
		    host->h_addr_list[0], sizeof (struct in_addr));

		if (bind(ts->ts_lsn,
			(struct sockaddr *)&ts->ts_add,
			sizeof (struct sockaddr_in)) == 0) {
			break;
		}

		LM_CHK(errno == EADDRINUSE);
	}

	LM_CHK(listen(ts->ts_lsn, 5) == 0);

	return (0);
}

int
benchmark_pre(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			result;
	struct sockaddr_in	addr;
	socklen_t		size;

	ts->ts_conn = socket(AF_INET, SOCK_STREAM, 0);
	LM_CHK(ts->ts_conn != -1);
	LM_CHK(fcntl(ts->ts_conn, F_SETFL, O_NDELAY) == 0);

	result = connect(ts->ts_conn,
	    (struct sockaddr *)&ts->ts_add,
	    sizeof (struct sockaddr_in));

	LM_CHK((result == -1) && (errno == EINPROGRESS));

	size = sizeof (struct sockaddr);
	result = accept(ts->ts_lsn, (struct sockaddr *)&addr,
	    &size);
	LM_CHK(result != -1);
	ts->ts_acc = result;

	return (0);
}

int
benchmark(void *tsd, result_t *res)
{
	tsd_t			*ts = (tsd_t *)tsd;

	LM_CHK(close(ts->ts_acc) == 0);

	return (0);
}

int
benchmark_post(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;

	LM_CHK(close(ts->ts_conn) == 0);

	return (0);
}

int
benchmark_finiworker(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;

	LM_CHK(close(ts->ts_lsn) == 0);

	return (0);
}
