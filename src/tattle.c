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


#ifdef USE_RDTSC
#ifdef __GNUC__
#define	ENABLE_RDTSC 1
#endif
#endif

/*
 * dummy so we can link w/ libmicro
 */

/*ARGSUSED*/
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

	if (strlen(compiler_version) > 30)
		compiler_version[30] = 0;

	cleanup(compiler_version);
	cleanup(extra_compiler_flags);

	while ((c = getopt(argc, argv, "vcfrsVTR")) != -1) {
		switch (c) {
		case 'V':
			(void) printf("%s\n", LIBMICRO_VERSION);
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

		case 'R':
#ifdef ENABLE_RDTSC
			{
				struct timeval 	s;
				struct timeval	f;
				long long 	start_nsecs;
				long long 	end_nsecs;
				long 		elapsed_usecs;

				gettimeofday(&s, NULL);
				start_nsecs = rdtsc();
				for (;;) {
					gettimeofday(&f, NULL);
					elapsed_usecs = (f.tv_sec - s.tv_sec) *
					    1000000 + (f.tv_usec - s.tv_usec);
					if (elapsed_usecs > 1000000)
						break;
				}
				end_nsecs = rdtsc();
				(void) printf("LIBMICRO_HZ=%lld\n",
				    (long long)elapsed_usecs *
				    (end_nsecs - start_nsecs) / 1000000LL);
			}
#else
			(void) printf("\n");
#endif
			break;
		}
	}

	exit(0);
}
