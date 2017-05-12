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
#include <sys/poll.h>

#include "libmicro.h"

#define	FIRSTPORT		12345

typedef struct {
	int			ts_once;
	int			ts_lsn;
	int			ts_acc;
	int			ts_conn;
	struct sockaddr_in	ts_addr;
} tsd_t;

static int			opta = 0;
static int			optc = 0;
static struct hostent		*host;

int
benchmark_init()
{
	lm_tsdsize = sizeof (tsd_t);

	(void) sprintf(lm_optstr, "ac");

	(void) sprintf(lm_usage,
	    "       [-a] (measure accept() only)\n"
	    "       [-c] (measure connect() only)\n"
	    "notes: measures connect()/accept()\n");

	return (0);
}

/*ARGSUSED*/
int
benchmark_optswitch(int opt, char *optarg)
{
	switch (opt) {
	case 'a':
		opta = 1;
		break;
	case 'c':
		optc = 1;
		break;
	default:
		return (-1);
	}

	if (opta && optc) {
		(void) printf("warning: -a overrides -c\n");
		optc = 0;
	}

	return (0);
}

int
benchmark_initrun()
{
	(void) setfdlimit(3 * lm_optT + 10);

	return (0);
}

int
benchmark_pre_once(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			j = FIRSTPORT;

	ts->ts_lsn = socket(AF_INET, SOCK_STREAM, 0);
	LM_CHK(ts->ts_lsn != -1);

	/*
	 * make accept socket non-blocking so in case of errors
	 * we don't hang
	 */

	LM_CHK(fcntl(ts->ts_lsn, F_SETFL, O_NDELAY) != -1);
	LM_CHK((host = gethostbyname("localhost")) != NULL);

	for (;;) {
		(void) memset(&ts->ts_addr, 0, sizeof (struct sockaddr_in));
		ts->ts_addr.sin_family = AF_INET;
		ts->ts_addr.sin_port = htons(j++);
		(void) memcpy(&ts->ts_addr.sin_addr.s_addr,
		    host->h_addr_list[0], sizeof (struct in_addr));

		if (bind(ts->ts_lsn,
			(struct sockaddr *)&ts->ts_addr,
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

	if (ts->ts_once++ == 0) {
		LM_CHK(benchmark_pre_once(tsd) == 0);
	}

	ts->ts_conn = socket(AF_INET, SOCK_STREAM, 0);
	LM_CHK(ts->ts_conn != -1);
	LM_CHK(fcntl(ts->ts_conn, F_SETFL, O_NDELAY) != -1);
	if (opta) {
		result = connect(ts->ts_conn,
		    (struct sockaddr *)&ts->ts_addr,
		    sizeof (struct sockaddr_in));
		LM_CHK((result == 0) || (errno == EINPROGRESS));
	}

	return (0);
}

int
benchmark(void *tsd, result_t *res)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			result;
	struct sockaddr_in	addr;
	socklen_t		size;

	if (!opta) {
	  again:
		result = connect(ts->ts_conn,
		    (struct sockaddr *)&ts->ts_addr,
		    sizeof (struct sockaddr_in));
		if (result != 0 && errno != EISCONN) {
			struct pollfd pollfd;
			LM_CHK(errno == EINPROGRESS);
			if (optc)
				goto done;
			pollfd.fd = ts->ts_conn;
			pollfd.events = POLLOUT;
			if (poll(&pollfd, 1, -1) == 1)
				goto again;
		}
	}

	if (!optc) {
		size = sizeof (struct sockaddr);
		for (;;) {
			struct pollfd pollfd;
			result = accept(ts->ts_lsn,
			    (struct sockaddr *)&addr, &size);
			if (result > 0 || (result == -1 &&
				errno != EAGAIN))
				break;
			pollfd.fd = ts->ts_lsn;
			pollfd.events = POLLIN;
			if (poll(&pollfd, 1, -1) != 1)
				break;
		}

		ts->ts_acc = result;
		LM_CHK(result != -1);
	}

  done:

	return (0);
}

int
benchmark_post(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;

	if (!optc) {
		LM_CHK(close(ts->ts_acc) == 0);
	}
	LM_CHK(close(ts->ts_conn) == 0);

	return (0);
}
