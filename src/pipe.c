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
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include "libmicro.h"

typedef struct {
	int			ts_once;
	pid_t			ts_child;
	pthread_t		ts_thread;
	int			ts_in;
	int			ts_out;
	int			ts_in2;
	int			ts_out2;
	int			ts_lsn;
	struct sockaddr_in	ts_add;
} tsd_t;

#define	FIRSTPORT		12345

static char			*modes[] = {"st", "mt", "mp", NULL};
#define	MD_SINGLE		0
#define	MD_MULTITHREAD		1
#define	MD_MULTIPROCESS		2

static char			*xports[] = {"pipe", "fifo", "sock", "tcp",
				    NULL};
#define	XP_PIPES		0
#define	XP_FIFOS		1
#define	XP_SOCKETPAIR		2
#define	XP_LOCALTCP		3

#define	DEFM			MD_SINGLE
#define	DEFS			1024
#define	DEFX			XP_PIPES

static int			optm = DEFM;
static ssize_t			opts = DEFS;
static int			optx = DEFX;
static void			*rbuf = NULL;
static void			*wbuf = NULL;

int readall(int s, void *buf, size_t len);
void *loopback(void *arg);
int prepare_pipes(tsd_t *tsd);
int prepare_fifos(tsd_t *tsd);
int cleanup_fifos(tsd_t *tsd);
int prepare_socketpair(tsd_t *tsd);
int prepare_localtcp(tsd_t *tsd);
int prepare_localtcp_once(tsd_t *tsd);
char *lookupa(int x, char *names[]);
int lookup(char *x, char *names[]);

int
benchmark_init()
{
	lm_tsdsize = sizeof (tsd_t);

	(void) sprintf(lm_optstr, "m:s:x:");

	(void) sprintf(lm_usage,
	    "       [-m mode (st|mt|mp, default %s)]\n"
	    "       [-s buffer-size (default %d)]\n"
	    "       [-x transport (pipe|fifo|sock|tcp, default %s)]\n"
	    "notes: measures write()/read() across various transports\n",
	    lookupa(DEFM, modes), DEFS, lookupa(DEFX, xports));

	(void) sprintf(lm_header, "%2s %4s", "md", "xprt");

	return (0);
}

int
benchmark_optswitch(int opt, char *optarg)
{
	int			x;

	switch (opt) {
	case 'm':
		x = lookup(optarg, modes);
		if (x == -1)
			return (-1);
		optm = x;
		break;
	case 's':
		opts = sizetoll(optarg);
		break;
	case 'x':
		x = lookup(optarg, xports);
		if (x == -1)
			return (-1);
		optx = x;
		break;
	default:
		return (-1);
	}
	return (0);
}

int
benchmark_initrun()
{
	if (optx == XP_FIFOS) {
		if (geteuid() != 0) {
			(void) printf("sorry, must be root to create fifos\n");
			exit(1);
		}
	}

	(void) setfdlimit(4 * lm_optT + 10);

	rbuf = malloc(opts);
	wbuf = malloc(opts);

	return (0);
}

int
benchmark_initbatch(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			result;
	pid_t			pid;

	switch (optx) {
	case XP_SOCKETPAIR:
		result = prepare_socketpair(ts);
		break;
	case XP_LOCALTCP:
		result = prepare_localtcp(ts);
		break;
	case XP_FIFOS:
		result = prepare_fifos(ts);
		break;
	case XP_PIPES:
	default:
		result = prepare_pipes(ts);
		break;
	}
	if (result == -1) {
		return (1);
	}

	switch (optm) {
	case MD_MULTITHREAD:
		LM_CHK(pthread_create(&ts->ts_thread,
			NULL, loopback, tsd) != -1);
		break;
	case MD_MULTIPROCESS:
		pid = fork();
		switch (pid) {
		case 0:
			(void) loopback(tsd);
			exit(0);
			break;
		case -1:
			return (-1);
		default:
			ts->ts_child = pid;
			break;
		}
		break;
	case MD_SINGLE:
	default:
		break;
	}

	/* Prime the loopback */
	LM_CHK(write(ts->ts_out, wbuf, opts) == opts);
	LM_CHK(readall(ts->ts_in, rbuf, opts) == opts);

	return (0);
}

int
benchmark(void *tsd, result_t *res)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			i;

	for (i = 0; i < lm_optB; i++) {
		LM_CHK(write(ts->ts_out, wbuf, opts) == opts);
		LM_CHK(readall(ts->ts_in, rbuf, opts) != -1);
	}
	res->re_count = i;

	return (0);
}

int
benchmark_finibatch(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;

	/* Terminate the loopback */
	LM_CHK(write(ts->ts_out, wbuf, opts) == opts);
	LM_CHK(readall(ts->ts_in, rbuf, opts) != -1);

	switch (optm) {
	case MD_MULTITHREAD:
		LM_CHK(close(ts->ts_in2) == 0);
		LM_CHK(close(ts->ts_out2) == 0);
		LM_CHK(pthread_join(ts->ts_thread, NULL) == 0);
		break;
	case MD_MULTIPROCESS:
		LM_CHK(close(ts->ts_in2) == 0);
		LM_CHK(close(ts->ts_out2) == 0);
		LM_CHK(waitpid(ts->ts_child, NULL, 0) != -1);
		break;
	case MD_SINGLE:
	default:
		break;
	}

	LM_CHK(close(ts->ts_in) == 0);
        LM_CHK(close(ts->ts_out) == 0);

	if (optx == XP_FIFOS) {
		(void) cleanup_fifos(ts);
	}

	return (0);
}

char *
benchmark_result()
{
	static char		result[256];

	(void) sprintf(result, "%2s %4s",
	    lookupa(optm, modes), lookupa(optx, xports));

	return (result);
}

int
readall(int s, void *buf, size_t len)
{
	size_t			n;
	size_t			total = 0;

	for (;;) {
		n = read(s, (void *)((long)buf + total), len - total);
		LM_CHK(n >= 1);
		total += n;
		if (total == len) {
			return (total);
		}
		LM_CHK(total < len);
	}
}

void *
loopback(void *arg)
{
	tsd_t			*ts = (tsd_t *)arg;
	int			i, m;

	/* Include priming and termination */
	m = lm_optB + 2;

	for (i = 0; i < m; i++) {
		LM_CHK(readall(ts->ts_in2, rbuf, opts) == opts);
		LM_CHK(write(ts->ts_out2, wbuf, opts) == opts);
	}

	return (NULL);
}

int
prepare_localtcp_once(tsd_t *ts)
{
	int			j = FIRSTPORT;
	int			opt = 1;
	struct hostent	*host;

	ts->ts_lsn = socket(AF_INET, SOCK_STREAM, 0);
	LM_CHK(ts->ts_lsn != -1);

	LM_CHK((setsockopt(ts->ts_lsn, SOL_SOCKET, SO_REUSEADDR,
		    &opt, sizeof (int))) != -1);

	LM_CHK((host = gethostbyname("localhost")) != NULL);

	for (;;) {
		(void) memset(&ts->ts_add, 0,
		    sizeof (struct sockaddr_in));
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

	LM_CHK(listen(ts->ts_lsn, 5) != -1);
	return (0);
}

int
prepare_localtcp(tsd_t *ts)
{
	int			result;
	struct sockaddr_in	addr;
	int			opt = 1;
	socklen_t		size;

	if (ts->ts_once++ == 0) {
		LM_CHK(prepare_localtcp_once(ts) != -1);
	}

	ts->ts_out = socket(AF_INET, SOCK_STREAM, 0);
	LM_CHK(ts->ts_out != -1);
	LM_CHK(fcntl(ts->ts_out, F_SETFL, O_NDELAY) != -1);

	result = connect(ts->ts_out, (struct sockaddr *)&ts->ts_add,
	    sizeof (struct sockaddr_in));

	LM_CHK((result == -1) && (errno == EINPROGRESS));
	LM_CHK(fcntl(ts->ts_out, F_SETFL, 0) != -1);

	size = sizeof (struct sockaddr);
	result = accept(ts->ts_lsn, (struct sockaddr *)&addr, &size);
	LM_CHK(result != -1);
	ts->ts_out2 = result;

	LM_CHK(setsockopt(ts->ts_out, IPPROTO_TCP, TCP_NODELAY,
		&opt, sizeof (int)) != -1);
	LM_CHK(setsockopt(ts->ts_out2, IPPROTO_TCP, TCP_NODELAY,
		&opt, sizeof (int)) != -1);

	if (optm == MD_SINGLE) {
		ts->ts_in = ts->ts_out2;
	} else {
		ts->ts_in = ts->ts_out;
		ts->ts_in2 = ts->ts_out2;
	}

	return (0);
}

int
prepare_socketpair(tsd_t *ts)
{
	int			s[2];

	LM_CHK(socketpair(PF_UNIX, SOCK_STREAM, 0, s) != -1);

	if (optm == MD_SINGLE) {
		ts->ts_in = s[0];
		ts->ts_out = s[1];
	} else {
		ts->ts_in = s[0];
		ts->ts_out = s[0];
		ts->ts_in2 = s[1];
		ts->ts_out2 = s[1];
	}

	return (0);
}

int
prepare_fifos(tsd_t *ts)
{
	char			path[64];

	(void) sprintf(path, "/tmp/pipe_%d.%dA",
	    getpid(), (int)pthread_self());

	LM_CHK(mknod(path, 0600, S_IFIFO) != -1);

	if (optm == MD_SINGLE) {
		ts->ts_in = open(path, O_RDONLY);
		ts->ts_out = open(path, O_WRONLY);
	} else {
		ts->ts_in = open(path, O_RDONLY);
		ts->ts_out2 = open(path, O_WRONLY);

		(void) sprintf(path, "/tmp/pipe_%d.%dB",
		    getpid(), (int)pthread_self());

		LM_CHK(mknod(path, 0600, S_IFIFO) != -1);

		ts->ts_in2 = open(path, O_RDONLY);
		ts->ts_out = open(path, O_WRONLY);
	}

	return (0);
}

/*ARGSUSED*/
int
cleanup_fifos(tsd_t *ts)
{
	char			path[64];

	(void) sprintf(path, "/tmp/pipe_%d.%dA", getpid(), (int)pthread_self());
	LM_CHK(unlink(path) == 0);
	(void) sprintf(path, "/tmp/pipe_%d.%dB", getpid(), (int)pthread_self());
	LM_CHK(unlink(path) == 0);

	return (0);
}

int
prepare_pipes(tsd_t *ts)
{
	int			p[2];

	if (optm == MD_SINGLE) {
		LM_CHK(pipe(p) != -1);
		ts->ts_in = p[0];
		ts->ts_out = p[1];

	} else {
		LM_CHK(pipe(p) != -1);
		ts->ts_in = p[0];
		ts->ts_out2 = p[1];

		LM_CHK(pipe(p) != -1);
		ts->ts_in2 = p[0];
		ts->ts_out = p[1];
	}

	return (0);
}

char *
lookupa(int x, char *names[])
{
	int			i = 0;

	while (names[i] != NULL) {
		if (x == i) {
			return (names[i]);
		}
		i++;
	}
	return (NULL);
}

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
