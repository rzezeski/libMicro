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

/*
 * time program to recursively test exec time
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(int argc, char *argv[])
{
	int left;

	if (argc == 1) {
		exit(1);
	}

	left = atoi(argv[1]);

	left--;

	if (left <= 0) {
		exit(0);
	} else {
		char buffer[80];
		(void) sprintf(buffer, "%d", left);
		argv[1] = buffer;
		if (execv(argv[0], argv)) {
			exit(2);
		}
	}

	return (0);
}
