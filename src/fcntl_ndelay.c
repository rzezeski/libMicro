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
 * measures  O_NDELAY on socket
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
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

static int			fd = -1;

int
benchmark_init()
{
	(void) sprintf(lm_usage,
	    "notes: measures F_GETFL/F_SETFL O_NDELAY on socket\n");

	lm_tsdsize = 0;

	return (0);
}

int
benchmark_initrun()
{
	LM_CHK((fd = socket(AF_INET, SOCK_STREAM, 0)) != -1);
	return (0);
}

/*ARGSUSED*/
int
benchmark(void *tsd, result_t *res)
{
	int			flags;

	LM_CHK(fcntl(fd, F_GETFL, &flags) != -1);
	flags |= O_NDELAY;
	LM_CHK(fcntl(fd, F_SETFL, O_NDELAY) != -1);
	LM_CHK(fcntl(fd, F_GETFL, &flags) != -1);
	flags &= ~O_NDELAY;
	LM_CHK(fcntl(fd, F_SETFL, &flags) != -1);

	return (0);
}
