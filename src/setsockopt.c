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
#include <errno.h>

#include "libmicro.h"

typedef struct {
	int	ts_fd;
} tsd_t;

int
benchmark_init()
{
	lm_tsdsize = sizeof (tsd_t);

	(void) sprintf(lm_usage, "setsockopt(TCP_NODELAY)\n");

	return (0);
}

int
benchmark_pre(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;

	LM_CHK((ts->ts_fd = socket(AF_INET, SOCK_STREAM, 0)) != -1);

	return (0);
}

int
benchmark_post(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;

	LM_CHK(close(ts->ts_fd) == 0);

	return (0);
}

int
benchmark(void *tsd, result_t *res)
{
	tsd_t			*ts = (tsd_t *)tsd;
	int			opt;

	opt = 1;
	LM_CHK(setsockopt(ts->ts_fd, IPPROTO_TCP, TCP_NODELAY,
		&opt, sizeof (int)) == 0);

	return (0);
}
