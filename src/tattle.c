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
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <tattle.h>
#include "libmicro.h"
#include <math.h>

/*
 * dummy so we can link w/ libmicro
 */

int
benchmark(void *tsd, result_t *res)
{
	return (0);
}

static void
cleanup(char *s)
{
	char *o = s;
	char *e;

	while (*s == ' ')
		s++;

	if (o != s)
		(void) strcpy(o, s);

	e = o;

	while (*e != 0)
		e++;

	e--;

	while (*e == ' ' && e > o)
		*e-- = 0;

}


int
main(int argc, char *argv[])
{
	int c;

	if (strlen(compiler_version) > 50)
		compiler_version[50] = 0;

	cleanup(compiler_version);
	cleanup(extra_compiler_flags);

	while ((c = getopt(argc, argv, "vcfrsVTR")) != -1) {
		switch (c) {
		case 'V':
			(void) printf("%s %s\n", LIBMICRO_VERSION, LM_VSN_HASH);
			break;
		case 'v':
			(void) printf("%s\n", compiler_version);
			break;
		case 'c':
			(void) printf("%s\n", CC);
			break;
		case 'f':
			if (strlen(extra_compiler_flags) == 0)
				(void) printf("[none]\n");
			else
				(void) printf("%s\n", extra_compiler_flags);
			break;

		case 's':
			(void) printf("%lu\n", sizeof (long));
			break;

		case 'r':

			(void) printf("%d nsecs\n", 1);
			break;
		}
	}

	exit(0);
}
