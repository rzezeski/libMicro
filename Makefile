#
# This file and its contents are supplied under the terms of the
# Common Development and Distribution License ("CDDL"), version 1.0.
# You may only use this file in accordance with the terms of version
# 1.0 of the CDDL.
#
# A full copy of the text of the CDDL should have accompanied this
# source.  A copy of the CDDL is also available via the Internet at
# http://www.illumos.org/license/CDDL.
#

#
# Copyright 2015 Ryan Zezeski <ryan@zinascii.com>
# Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#

include Makefile.benchmarks

BINS=		$(ALL:%=bin/%) bin/tattle

TARBALL_CONTENTS = 		\
	Makefile.benchmarks 	\
	Makefile.SunOS 		\
	Makefile.Linux 		\
	Makefile.Aix 		\
	Makefile.com 		\
	Makefile		\
	bench			\
	bench.sh		\
	mk_tarball		\
	multiview		\
	multiview.sh		\
	src/			\
	OPENSOLARIS.LICENSE	\
	wrapper			\
	wrapper.sh		\
	README

default $(ALL) run cstyle lint tattle: $(BINS)
	@cp bench.sh bench
	@cp multiview.sh multiview
	@cp wrapper.sh wrapper
	@chmod +x bench multiview wrapper
	@mkdir -p bin-`uname -p`; cd bin-`uname -p`; $(MAKE) -f ../Makefile.`uname -s` $@

clean:
	rm -rf bin bin-* wrapper multiview bench

bin:
	@mkdir -p bin

$(BINS): bin
	@cp wrapper.sh wrapper
	@chmod +x wrapper
	@ln -sf ../wrapper $@


libMicro.tar:	FORCE
	@chmod +x ./mk_tarball wrapper
	@./mk_tarball $(TARBALL_CONTENTS) 

FORCE:

