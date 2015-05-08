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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include "libmicro.h"

typedef struct {
	int	ts_semid;
} tsd_t;

int
benchmark_init()
{
	lm_tsdsize = sizeof (tsd_t);

	(void) sprintf(lm_usage, "notes: measures semop()\n");

	return (0);
}

int
benchmark_pre(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;

	LM_CHK((ts->ts_semid = semget(IPC_PRIVATE, 2, 0600)) != -1);

	return (0);
}

int
benchmark_post(void *tsd)
{
	tsd_t			*ts = (tsd_t *)tsd;

	LM_CHK(semctl(ts->ts_semid, 0, IPC_RMID) != -1);

	return (0);
}

int
benchmark(void *tsd, result_t *res)
{
	tsd_t			*ts = (tsd_t *)tsd;
	struct sembuf		s[1];

	s[0].sem_num = 0;
	s[0].sem_op  = 1;
	s[0].sem_flg = 0;
	LM_CHK(semop(ts->ts_semid, s, 1) == 0);

	s[0].sem_num = 0;
	s[0].sem_op  = -1;
	s[0].sem_flg = 0;
	LM_CHK(semop(ts->ts_semid, s, 1) == 0);

	return (0);
}
