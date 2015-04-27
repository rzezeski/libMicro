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
 * getpeername test
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

static int			sock = -1;

int
benchmark_init()
{
	(void) sprintf(lm_usage, "notes: measures getpeername()\n");
	lm_tsdsize = 0;

	return (0);
}

int
benchmark_initrun()
{
	int			j = FIRSTPORT;
	int			opt = 1;
	int			result;
	socklen_t		size;
	struct hostent		*host;
	struct sockaddr_in	adds;
	int			sock2, sock3;

	LM_CHK((sock2 = socket(AF_INET, SOCK_STREAM, 0)) != -1);
	LM_CHK(setsockopt(sock2, SOL_SOCKET, SO_REUSEADDR,
		&opt, sizeof (int)) == 0);
	LM_CHK((host = gethostbyname("localhost")) != NULL);

	for (;;) {
		(void) memset(&adds, 0, sizeof (struct sockaddr_in));
		adds.sin_family = AF_INET;
		adds.sin_port = htons(j++);
		(void) memcpy(&adds.sin_addr.s_addr, host->h_addr_list[0],
		    sizeof (struct in_addr));

		if (bind(sock2, (struct sockaddr *)&adds,
		    sizeof (struct sockaddr_in)) == 0) {
			break;
		}

		LM_CHK(errno == EADDRINUSE);
	}

	LM_CHK(listen(sock2, 5) == 0);
	LM_CHK((sock3 = socket(AF_INET, SOCK_STREAM, 0)) != -1);
	LM_CHK(fcntl(sock3, F_SETFL, O_NDELAY) != -1);

	result = connect(sock3, (struct sockaddr *)&adds,
	    sizeof (struct sockaddr_in));
	LM_CHK((result == -1) && (errno == EINPROGRESS));

	size = sizeof (struct sockaddr);
	LM_CHK((sock = accept(sock2, (struct sockaddr *)&adds, &size)) != -1);

	return (0);
}

/*ARGSUSED*/
int
benchmark(void *tsd, result_t *res)
{
	int			i;
	struct sockaddr_in	adds;
	socklen_t		size;

	for (i = 0; i < lm_optB; i++) {
		size = sizeof (struct sockaddr_in);
		LM_CHK(getpeername(sock, (struct sockaddr *)&adds, &size) == 0);
	}
	res->re_count = i;

	return (0);
}
