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

include ../Makefile.benchmarks

EXTRA_CFILES=	exec_bin.c 	\
		elided.c	\
		tattle.c

CFLAGS+=-std=c99 -m64 -Wall -Wextra -Wno-unused-parameter -Werror

#
# Assume GCC or LLVM.
#
COMPILER_VERSION_CMD=$(CC) --version | head -1

default: $(ALL) tattle

cstyle:
	for file in $(ALL:%=../src/%.c) $(EXTRA_CFILES:%=../%) ; \
	do cstyle -p $$file ;\
	done


lint:	libmicro.ln $(ALL:%=%.lint) $(EXTRA_CFILES:%.c=%.lint)


$(EXTRA_CFILES:%.c=%.lint):
	$(LINT) ../src/$(@:%.lint=%.c) -I. -mu -lc libmicro.ln -lm

%.lint:	../src/%.c libmicro.ln
	$(LINT) -mu $(CPPFLAGS) $< libmicro.ln -lpthread -lsocket -lnsl -lm

%.o:	../src/%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

libmicro.ln: ../libmicro.c ../libmicro_main.c ../libmicro.h ../benchmark_*.c
	$(LINT) -muc $(CPPFLAGS) ../libmicro.c ../libmicro_main.c ../benchmark_*.c

CPPFLAGS+= -D_REENTRANT

bind_EXTRA_LIBS=$(NSLLIB) $(SOCKLIB)
cascade_flock_EXTRA_LIBS=$(UCBLIB)
close_tcp_EXTRA_LIBS=$(NSLLIB) $(SOCKLIB)
connection_EXTRA_LIBS=$(NSLLIB) $(SOCKLIB)
fcntl_ndelay_EXTRA_LIBS=$(SOCKLIB)
getpeername_EXTRA_LIBS=$(NSLLIB) $(SOCKLIB)
getsockname_EXTRA_LIBS=$(NSLLIB) $(SOCKLIB)
listen_EXTRA_LIBS=$(NSLLIB) $(SOCKLIB)
log_EXTRA_LIBS=$(MATHLIB)
pipe_EXTRA_LIBS=$(NSLLIB) $(SOCKLIB)
poll_EXTRA_LIBS=$(SOCKLIB)
select_EXTRA_LIBS=$(SOCKLIB)
setsockopt_EXTRA_LIBS=$(NSLLIB) $(SOCKLIB)
socket_EXTRA_LIBS=$(SOCKLIB)
socketpair_EXTRA_LIBS=$(SOCKLIB)

BENCHMARK_FUNCS=		\
	benchmark_init.o	\
	benchmark_fini.o	\
	benchmark_initrun.o	\
	benchmark_finirun.o	\
	benchmark_initbatch.o	\
	benchmark_finibatch.o	\
	benchmark_initworker.o	\
	benchmark_finiworker.o	\
	benchmark_optswitch.o	\
	benchmark_result.o

recurse_EXTRA_DEPS=recurse2.o


recurse:	$(recurse_EXTRA_DEPS)

libmicro.a:	libmicro.o libmicro_main.o $(BENCHMARK_FUNCS)
		$(AR) -cr libmicro.a libmicro.o libmicro_main.o $(BENCHMARK_FUNCS)

tattle:		../src/tattle.c	libmicro.a
	echo "char compiler_version[] = \""`$(COMPILER_VERSION_CMD)`"\";" > \
		tattle.h
	echo "char CC[] = \""$(CC)"\";" >> tattle.h
	echo "char extra_compiler_flags[] = \""$(extra_CFLAGS)"\";" >> tattle.h
	$(CC) -o $(@) $(CFLAGS) -I. $< $(CPPFLAGS) \
		libmicro.a -lm -pthread $(EXTRA_LIBS)

$(ELIDED_BENCHMARKS):	../src/elided.c
	$(CC) -o $(@) ../src/elided.c

%: libmicro.a %.o
	$(CC) -o $(@) $(@).o $($(@)_EXTRA_DEPS) $(CFLAGS) libmicro.a $($(@)_EXTRA_LIBS) $(EXTRA_LIBS) -lpthread -lm

exec:	exec_bin

exec_bin:	exec_bin.o
	$(CC) -o exec_bin $(CFLAGS) exec_bin.o

FORCE:


._KEEP_STATE:

