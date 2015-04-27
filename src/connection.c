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
	int			*ts_lsns;
	int			*ts_accs;
	int			*ts_cons;
	struct sockaddr_in	*ts_adds;
} tsd_t;

static int			opta = 0;
static int			optc = 0;
static struct hostent		*host;

int
benchmark_init()
{
	lm_defB = 256;
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
	(void) setfdlimit(3 * lm_optB * lm_optT + 10);

	return (0);
}

int
benchmark_initbatch_once(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			i;
	int			j = FIRSTPORT;

	LM_CHK((ts->ts_lsns = (int *)malloc(lm_optB * sizeof (int))) != NULL);
	LM_CHK((ts->ts_accs = (int *)malloc(lm_optB * sizeof (int))) != NULL);
	LM_CHK((ts->ts_cons = (int *)malloc(lm_optB * sizeof (int))) != NULL);
	ts->ts_adds =
	    (struct sockaddr_in *)malloc(lm_optB *
	    sizeof (struct sockaddr_in));
	LM_CHK(ts->ts_accs != NULL);

	for (i = 0; i < lm_optB; i++) {
		ts->ts_lsns[i] = socket(AF_INET, SOCK_STREAM, 0);
		LM_CHK(ts->ts_lsns[i] != -1);

		/*
		 * make accept socket non-blocking so in case of errors
		 * we don't hang
		 */

	        LM_CHK(fcntl(ts->ts_lsns[i], F_SETFL, O_NDELAY) != -1);
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

	if (ts->ts_once++ == 0) {
		LM_CHK(benchmark_initbatch_once(tsd) == 0);
	}

	for (i = 0; i < lm_optB; i++) {
		ts->ts_cons[i] = socket(AF_INET, SOCK_STREAM, 0);
		LM_CHK(ts->ts_cons[i] != -1);
		LM_CHK(fcntl(ts->ts_cons[i], F_SETFL, O_NDELAY) != -1);
		if (opta) {
			result = connect(ts->ts_cons[i],
			    (struct sockaddr *)&ts->ts_adds[i],
			    sizeof (struct sockaddr_in));
			LM_CHK((result == 0) || (errno == EINPROGRESS));
		}
	}

	return (0);
}

int
benchmark(void *tsd, result_t *res)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			i;
	int			result;
	struct sockaddr_in	addr;
	socklen_t		size;

	for (i = 0; i < lm_optB; i++) {
		if (!opta) {
		again:
			result = connect(ts->ts_cons[i],
			    (struct sockaddr *)&ts->ts_adds[i],
			    sizeof (struct sockaddr_in));
			if (result != 0 && errno != EISCONN) {
				LM_CHK(errno == EINPROGRESS);
				struct pollfd pollfd;
				if (optc)
					continue;
				pollfd.fd = ts->ts_cons[i];
				pollfd.events = POLLOUT;
				if (poll(&pollfd, 1, -1) == 1)
					goto again;
			}
		}

		if (!optc) {
			size = sizeof (struct sockaddr);
			for (;;) {
				struct pollfd pollfd;
				result = accept(ts->ts_lsns[i],
				    (struct sockaddr *)&addr, &size);
				if (result > 0 || (result == -1 &&
				    errno != EAGAIN))
					break;
				pollfd.fd = ts->ts_lsns[i];
				pollfd.events = POLLIN;
				if (poll(&pollfd, 1, -1) != 1)
					break;
			}

			ts->ts_accs[i] = result;
			LM_CHK(result != -1);
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
		if (!optc) {
			LM_CHK(close(ts->ts_accs[i]) == 0);
		}
		LM_CHK(close(ts->ts_cons[i]) == 0);
	}

	return (0);
}
