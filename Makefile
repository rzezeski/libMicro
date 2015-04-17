#
# CDDL HEADER START
#
# The contents of this file are subject to the terms
# of the Common Development and Distribution License
# (the "License").  You may not use this file except
# in compliance with the License.
#
# You can obtain a copy of the license at
# src/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing
# permissions and limitations under the License.
#
# When distributing Covered Code, include this CDDL
# HEADER in each file and include the License file at
# usr/src/OPENSOLARIS.LICENSE.  If applicable,
# add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your
# own identifying information: Portions Copyright [yyyy]
# [name of copyright owner]
#
# CDDL HEADER END
#

#
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
	@mkdir -p bin-`uname -p`; cd bin-`uname -p`; $(MAKE) -f ../Makefile.`uname -s` UNAME_RELEASE=`uname -r | sed 's/\./_/g'` $@

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

