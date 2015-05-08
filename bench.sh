#!/bin/sh
#
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
# Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#

bench_version=1505
libmicro_version=`bin/tattle -V`

case $libmicro_version in
$bench_version)
	;;
*)
	echo "ERROR: libMicro version doesn't match 'bench' script version"
	exit 1
esac

TMPROOT=/tmp/libmicro.$$
VARROOT=/var/tmp/libmicro.$$
mkdir -p $TMPROOT
mkdir -p $VARROOT
trap "rm -rf $TMPROOT $VARROOT; exit 1" 0 2

TFILE=$TMPROOT/data
IFILE=$TMPROOT/ifile
TDIR1=$TMPROOT/0/1/2/3/4/5/6/7/8/9
TDIR2=$TMPROOT/1/2/3/4/5/6/7/8/9/0
VFILE=$VARROOT/data
VDIR1=$VARROOT/0/1/2/3/4/5/6/7/8/9
VDIR2=$VARROOT/1/2/3/4/5/6/7/8/9/0


OPTS="-E -C 200 -L -S"

dd if=/dev/zero of=$TFILE bs=1024k count=10 2>/dev/null
dd if=/dev/zero of=$VFILE bs=1024k count=10 2>/dev/null
mkdir -p $TDIR1 $TDIR2
mkdir -p $VDIR1 $VDIR2

touch $IFILE

ARCH=`uname -p`

# produce benchmark header for easier comparisons

hostname=`uname -n`

if [ -f /usr/sbin/psrinfo ]; then
	p_count=`psrinfo|wc -l`
	p_mhz=`psrinfo -v | awk '/operates/{print $6 "MHz"; exit }'`
	p_type=`psrinfo -vp 2>/dev/null | awk '{if (NR == 3) {print $0; exit}}'` 
fi

if [ -f /proc/cpuinfo ]; then
	p_count=`egrep processor /proc/cpuinfo | wc -l`
	p_mhz=`awk -F: '/cpu MHz/{printf("%5.0f00Mhz\n",$2/100); exit}' /proc/cpuinfo`
	p_type=`awk -F: '/model name/{print $2; exit}' /proc/cpuinfo`
fi

printf "!Libmicro_#:   %30s\n" $libmicro_version
printf "!Options:      %30s\n" "$OPTS"
printf "!Machine_name: %30s\n" $hostname
printf "!OS_name:      %30s\n" `uname -s`
printf "!OS_release:   %30s\n" `uname -r`
printf "!OS_build:     %30.18s\n" "`uname -v`"
printf "!Processor:    %30s\n" `uname -p`
printf "!#CPUs:        %30s\n" $p_count
printf "!CPU_MHz:      %30s\n" $p_mhz
printf "!CPU_NAME:     %30s\n" "$p_type"
printf "!IP_address:   %30s\n" `getent hosts $hostname | awk '{print $1}'`
printf "!Run_by:       %30s\n" $LOGNAME
printf "!Date:	       %30s\n" "`date '+%D %R'`"
printf "!Compiler:     %30s\n" `bin/tattle -c`
printf "!Compiler Ver.:%30s\n" "`bin/tattle -v`"
printf "!sizeof(long): %30s\n" `bin/tattle -s`
printf "!extra_CFLAGS: %30s\n" "`bin/tattle -f`"
printf "!TimerRes:     %30s\n" "`bin/tattle -r`"
 
mkdir -p $TMPROOT/bin
cp bin-$ARCH/exec_bin $TMPROOT/bin/$A

while read A B
do
	# $A contains the command, $B contains the arguments
	# we echo blank lines and comments
	# we ship anything which fails to match *$1* (useful
	# if we only want to test one case, but a nasty hack)

	case $A in
	""|"#"*)
		echo "$A $B"
		continue
		;;
	*$1*)
		;;
	*)
		continue
	esac

	if [ ! -f $TMPROOT/bin/$A ]
	then
		cp bin-$ARCH/$A $TMPROOT/bin/$A
	fi
	(cd $TMPROOT && eval "bin/$A $B")
done <<.
#
# Obligatory null system call: use very short time
# for default since SuSe implements this "syscall" in userland
#
getpid		$OPTS -N "getpid"

getenv		$OPTS -N "getenv"	-s 100
getenv		$OPTS -N "getenvT2"	-s 100 	-T 2

gettimeofday	$OPTS -N "gettimeofday"

log		$OPTS -N "log"
exp		$OPTS -N "exp"
lrand48		$OPTS -N "lrand48"

memset		$OPTS -N "memset_10"	-s 10
memset		$OPTS -N "memset_256"	-s 256
memset		$OPTS -N "memset_256_u"	-s 256	 -a 1
memset		$OPTS -N "memset_1k"	-s 1k
memset		$OPTS -N "memset_4k"    -s 4k
memset		$OPTS -N "memset_4k_uc" -s 4k    -u

memset		$OPTS -N "memset_10k"	-s 10k
memset		$OPTS -N "memset_1m"	-s 1m
memset		$OPTS -N "memset_10m"	-s 10m
memset		$OPTS -N "memsetP2_10m"	-s 10m -P 2

memrand		$OPTS -N "memrand"	-s 128m
cachetocache	$OPTS -N "cachetocache" -s 100k -T 2

isatty		$OPTS -N "isatty_yes"
isatty		$OPTS -N "isatty_no"  -f $IFILE

malloc		$OPTS -N "malloc_10"    -s 10    -g 10
malloc		$OPTS -N "malloc_100"   -s 100   -g 10
malloc		$OPTS -N "malloc_1k"    -s 1k    -g 10
malloc		$OPTS -N "malloc_10k"   -s 10k   -g 10
malloc		$OPTS -N "malloc_100k"  -s 100k  -g 10

malloc		$OPTS -N "mallocT2_10"    -s 10   -g 10 -T 2
malloc		$OPTS -N "mallocT2_100"   -s 100  -g 10 -T 2
malloc		$OPTS -N "mallocT2_1k"    -s 1k   -g 10 -T 2
malloc		$OPTS -N "mallocT2_10k"   -s 10k  -g 10 -T 2
malloc		$OPTS -N "mallocT2_100k"  -s 100k -g 10 -T 2

close		$OPTS -N "close_bad"		-b
close		$OPTS -N "close_tmp"		-f $TFILE
close		$OPTS -N "close_usr"		-f $VFILE
close		$OPTS -N "close_zero"		-f /dev/zero

memcpy		$OPTS -N "memcpy_10"	-s 10
memcpy		$OPTS -N "memcpy_1k"	-s 1k
memcpy		$OPTS -N "memcpy_10k"	-s 10k
memcpy		$OPTS -N "memcpy_1m"	-s 1m
memcpy		$OPTS -N "memcpy_10m"	-s 10m

strcpy		$OPTS -N "strcpy_10"	-s 10
strcpy		$OPTS -N "strcpy_1k"	-s 1k

strlen		$OPTS -N "strlen_10"	-s 10
strlen		$OPTS -N "strlen_1k"	-s 1k

strchr		$OPTS -N "strchr_10"	-s 10
strchr		$OPTS -N "strchr_1k"	-s 1k
strcmp		$OPTS -N "strcmp_10"	-s 10
strcmp		$OPTS -N "strcmp_1k"	-s 1k

strcasecmp	$OPTS -N "scasecmp_10"	-s 10
strcasecmp	$OPTS -N "scasecmp_1k"	-s 1k

strtol		$OPTS -N "strtol"

getcontext	$OPTS -N "getcontext"
setcontext	$OPTS -N "setcontext"

mutex		$OPTS -N "mutex_st"
mutex		$OPTS -N "mutex_mt"	-t
mutex		$OPTS -N "mutex_T2"     -T 2

longjmp		$OPTS -N "longjmp"
siglongjmp	$OPTS -N "siglongjmp"

getrusage	$OPTS -N "getrusage"

times		$OPTS -N "times"
time		$OPTS -N "time"
localtime_r	$OPTS -N "localtime_r"
strftime	$OPTS -N "strftime"

mktime		$OPTS -N "mktime"
mktime		$OPTS -N "mktimeT2" -T 2

cascade_mutex	$OPTS -N "c_mutex_1"
cascade_mutex	$OPTS -N "c_mutex_10"	-T 10
cascade_mutex	$OPTS -N "c_mutex_200"	-T 200

cascade_cond	$OPTS -N "c_cond_1"	-I 100
cascade_cond	$OPTS -N "c_cond_10"	-T 10
cascade_cond	$OPTS -N "c_cond_200"	-T 200

cascade_lockf	$OPTS -N "c_lockf_1"
cascade_lockf	$OPTS -N "c_lockf_10"	-P 10
cascade_lockf	$OPTS -N "c_lockf_200"	-P 200 -I 5000000

cascade_flock	$OPTS -N "c_flock"
cascade_flock	$OPTS -N "c_flock_10"	-P 10
cascade_flock	$OPTS -N "c_flock_200"	-P 200

cascade_fcntl	$OPTS -N "c_fcntl_1"
cascade_fcntl	$OPTS -N "c_fcntl_10"	-P 10
cascade_fcntl	$OPTS -N "c_fcntl_200"	-P 200

file_lock	$OPTS -N "file_lock"

getsockname	$OPTS -N "getsockname"
getpeername	$OPTS -N "getpeername"

chdir		$OPTS -N "chdir_tmp"	$TDIR1 $TDIR2
chdir		$OPTS -N "chdir_usr"	$VDIR1 $VDIR2

chdir		$OPTS -N "chgetwd_tmp"	-g $TDIR1 $TDIR2
chdir		$OPTS -N "chgetwd_usr"	-g $VDIR1 $VDIR2

realpath	$OPTS -N "realpath_tmp" -f $TDIR1
realpath	$OPTS -N "realpath_usr"	-f $VDIR1

stat		$OPTS -N "stat_tmp"	-f $TFILE
stat		$OPTS -N "stat_usr"	-f $VFILE

fcntl		$OPTS -N "fcntl_tmp"	-f $TFILE
fcntl		$OPTS -N "fcntl_usr"	-f $VFILE
fcntl_ndelay	$OPTS -N "fcntl_ndelay"

lseek		$OPTS -N "lseek_t8k"	-s 8k	-f $TFILE
lseek		$OPTS -N "lseek_u8k"	-s 8k	-f $VFILE

open		$OPTS -N "open_tmp"		-f $TFILE
open		$OPTS -N "open_usr"		-f $VFILE
open		$OPTS -N "open_zero"		-f /dev/zero

dup		$OPTS -N "dup"

socket		$OPTS -N "socket_u"
socket		$OPTS -N "socket_i"		-f PF_INET

socketpair	$OPTS -N "socketpair"

setsockopt	$OPTS -N "setsockopt"

bind		$OPTS -N "bind"

listen		$OPTS -N "listen"

connection	$OPTS -N "connection"

poll		$OPTS -N "poll_10"	-n 10
poll		$OPTS -N "poll_100"	-n 100
poll		$OPTS -N "poll_1000"	-n 1000

poll		$OPTS -N "poll_w10"	-n 10	-w 1
poll		$OPTS -N "poll_w100"	-n 100	-w 10
poll		$OPTS -N "poll_w1000"	-n 1000	-w 100

select		$OPTS -N "select_10"	-n 10
select		$OPTS -N "select_100"	-n 100
select		$OPTS -N "select_1000"	-n 1000

select		$OPTS -N "select_w10"	-n 10	-w 1
select		$OPTS -N "select_w100"	-n 100	-w 10
select		$OPTS -N "select_w1000"	-n 1000	-w 100

semop		$OPTS -N "semop"

sigaction	$OPTS -N "sigaction"
signal		$OPTS -N "signal"
sigprocmask	$OPTS -N "sigprocmask"

pthread_create  $OPTS -N "pthread"

fork		$OPTS -N "fork_10"
fork		$OPTS -N "fork_100"		-C 100
fork		$OPTS -N "fork_1000"		-C 50

exit		$OPTS -N "exit_10"
exit		$OPTS -N "exit_100"
exit		$OPTS -N "exit_1000"		-C 50

exit		$OPTS -N "exit_10_nolibc"	-e

exec		$OPTS -N "exec"

system		$OPTS -N "system"

recurse		$OPTS -N "recurse"

read		$OPTS -N "read_t1k"	-s 1k			-f $TFILE
read		$OPTS -N "read_t10k"	-s 10k			-f $TFILE
read		$OPTS -N "read_t100k"	-s 100k			-f $TFILE

read		$OPTS -N "read_u1k"	-s 1k			-f $VFILE
read		$OPTS -N "read_u10k"	-s 10k			-f $VFILE
read		$OPTS -N "read_u100k"	-s 100k			-f $VFILE

read		$OPTS -N "read_z1k"	-s 1k			-f /dev/zero
read		$OPTS -N "read_z10k"	-s 10k			-f /dev/zero
read		$OPTS -N "read_z100k"	-s 100k			-f /dev/zero
read		$OPTS -N "read_zw100k"	-s 100k	         -w	-f /dev/zero

write		$OPTS -N "write_t1k"	-s 1k			-f $TFILE
write		$OPTS -N "write_t10k"	-s 10k			-f $TFILE
write		$OPTS -N "write_t100k"	-s 100k			-f $TFILE

write		$OPTS -N "write_u1k"	-s 1k			-f $VFILE
write		$OPTS -N "write_u10k"	-s 10k			-f $VFILE
write		$OPTS -N "write_u100k"	-s 100k			-f $VFILE

write		$OPTS -N "write_n1k"	-s 1k	-f /dev/null
write		$OPTS -N "write_n10k"	-s 10k	-f /dev/null
write		$OPTS -N "write_n100k"	-s 100k	-f /dev/null

writev		$OPTS -N "writev_t1k"	-s 1k			-f $TFILE
writev		$OPTS -N "writev_t10k"	-s 10k		        -f $TFILE
writev		$OPTS -N "writev_t100k"	-s 100k			-f $TFILE

writev		$OPTS -N "writev_u1k"	-s 1k			-f $VFILE
writev		$OPTS -N "writev_u10k"	-s 10k			-f $VFILE
writev		$OPTS -N "writev_u100k"	-s 100k			-f $VFILE

writev		$OPTS -N "writev_n1k"	-s 1k	-f /dev/null
writev		$OPTS -N "writev_n10k"	-s 10k	-f /dev/null
writev		$OPTS -N "writev_n100k"	-s 100k	-f /dev/null

pread		$OPTS -N "pread_t1k"	-s 1k	-f $TFILE
pread		$OPTS -N "pread_t10k"	-s 10k	-f $TFILE
pread		$OPTS -N "pread_t100k"	-s 100k	-f $TFILE

pread		$OPTS -N "pread_u1k"	-s 1k	-f $VFILE
pread		$OPTS -N "pread_u10k"	-s 10k	-f $VFILE
pread		$OPTS -N "pread_u100k"	-s 100k	-f $VFILE

pread		$OPTS -N "pread_z1k"	-s 1k	-f /dev/zero
pread		$OPTS -N "pread_z10k"	-s 10k	-f /dev/zero
pread		$OPTS -N "pread_z100k"	-s 100k	-f /dev/zero
pread		$OPTS -N "pread_zw100k"	-s 100k	-w -f /dev/zero

pwrite		$OPTS -N "pwrite_t1k"	-s 1k	-f $TFILE
pwrite		$OPTS -N "pwrite_t10k"	-s 10k	-f $TFILE
pwrite		$OPTS -N "pwrite_t100k"	-s 100k	-f $TFILE

pwrite		$OPTS -N "pwrite_u1k"	-s 1k	-f $VFILE
pwrite		$OPTS -N "pwrite_u10k"	-s 10k	-f $VFILE
pwrite		$OPTS -N "pwrite_u100k"	-s 100k	-f $VFILE

pwrite		$OPTS -N "pwrite_n1k"	-s 1k	-f /dev/null
pwrite		$OPTS -N "pwrite_n10k"	-s 10k	-f /dev/null
pwrite		$OPTS -N "pwrite_n100k"	-s 100k	-f /dev/null

mmap		$OPTS -N "mmap_z8k"	-l 8k   -f /dev/zero
mmap		$OPTS -N "mmap_z128k"	-l 128k	-f /dev/zero
mmap		$OPTS -N "mmap_t8k"	-l 8k	-f $TFILE
mmap		$OPTS -N "mmap_t128k"	-l 128k	-f $TFILE
mmap		$OPTS -N "mmap_u8k"	-l 8k	-f $VFILE
mmap		$OPTS -N "mmap_u128k"	-l 128k	-f $VFILE
mmap		$OPTS -N "mmap_a8k"	-l 8k	-f MAP_ANON
mmap		$OPTS -N "mmap_a128k"	-l 128k	-f MAP_ANON


mmap		$OPTS -N "mmap_rz8k"	-l 8k	-r	-f /dev/zero
mmap		$OPTS -N "mmap_rz128k"	-l 128k	-r	-f /dev/zero
mmap		$OPTS -N "mmap_rt8k"	-l 8k	-r	-f $TFILE
mmap		$OPTS -N "mmap_rt128k"	-l 128k	-r	-f $TFILE
mmap		$OPTS -N "mmap_ru8k"	-l 8k	-r	-f $VFILE
mmap		$OPTS -N "mmap_ru128k"	-l 128k	-r	-f $VFILE
mmap		$OPTS -N "mmap_ra8k"	-l 8k	-r	-f MAP_ANON
mmap		$OPTS -N "mmap_ra128k"	-l 128k	-r	-f MAP_ANON

mmap		$OPTS -N "mmap_wz8k"	-l 8k	-w	-f /dev/zero
mmap		$OPTS -N "mmap_wz128k"	-l 128k	-w	-f /dev/zero
mmap		$OPTS -N "mmap_wt8k"	-l 8k	-w	-f $TFILE
mmap		$OPTS -N "mmap_wt128k"	-l 128k	-w	-f $TFILE
mmap		$OPTS -N "mmap_wu8k"	-l 8k	-w	-f $VFILE
mmap		$OPTS -N "mmap_wu128k"	-l 128k	-w	-f $VFILE
mmap		$OPTS -N "mmap_wa8k"	-l 8k	-w	-f MAP_ANON
mmap		$OPTS -N "mmap_wa128k"	-l 128k	-w	-f MAP_ANON

munmap		$OPTS -N "unmap_z8k"	-l 8k		-f /dev/zero
munmap		$OPTS -N "unmap_z128k"	-l 128k		-f /dev/zero
munmap		$OPTS -N "unmap_t8k"	-l 8k		-f $TFILE
munmap		$OPTS -N "unmap_t128k"	-l 128k		-f $TFILE
munmap		$OPTS -N "unmap_u8k"	-l 8k		-f $VFILE
munmap		$OPTS -N "unmap_u128k"	-l 128k		-f $VFILE
munmap		$OPTS -N "unmap_a8k"	-l 8k		-f MAP_ANON
munmap		$OPTS -N "unmap_a128k"	-l 128k		-f MAP_ANON

munmap		$OPTS -N "unmap_rz8k"	-l 8k	-r	-f /dev/zero
munmap		$OPTS -N "unmap_rz128k"	-l 128k	-r	-f /dev/zero
munmap		$OPTS -N "unmap_rt8k"	-l 8k	-r	-f $TFILE
munmap		$OPTS -N "unmap_rt128k"	-l 128k	-r	-f $TFILE
munmap		$OPTS -N "unmap_ru8k"	-l 8k	-r	-f $VFILE
munmap		$OPTS -N "unmap_ru128k"	-l 128k	-r	-f $VFILE
munmap		$OPTS -N "unmap_ra8k"	-l 8k	-r	-f MAP_ANON
munmap		$OPTS -N "unmap_ra128k"	-l 128k	-r	-f MAP_ANON

connection	$OPTS -N "conn_connect"	-c

munmap		$OPTS -N "unmap_wz8k"	-l 8k	-w	-f /dev/zero
munmap		$OPTS -N "unmap_wz128k"	-l 128k	-w	-f /dev/zero
munmap		$OPTS -N "unmap_wt8k"	-l 8k	-w	-f $TFILE
munmap		$OPTS -N "unmap_wt128k"	-l 128k	-w	-f $TFILE
munmap		$OPTS -N "unmap_wu8k"	-l 8k	-w	-f $VFILE
munmap		$OPTS -N "unmap_wu128k"	-l 128k	-w	-f $VFILE
munmap		$OPTS -N "unmap_wa8k"	-l 8k	-w	-f MAP_ANON
munmap		$OPTS -N "unmap_wa128k"	-l 128k	-w	-f MAP_ANON


mprotect	$OPTS -N "mprot_z8k"	-l 8k  		-f /dev/zero
mprotect	$OPTS -N "mprot_z128k"	-l 128k		-f /dev/zero
mprotect	$OPTS -N "mprot_wz8k"	-l 8k	-w	-f /dev/zero
mprotect	$OPTS -N "mprot_wz128k"	-l 128k	-w	-f /dev/zero
mprotect	$OPTS -N "mprot_twz8k"  -l 8k   -w -t   -f /dev/zero
mprotect	$OPTS -N "mprot_tw128k" -l 128k -w -t   -f /dev/zero
mprotect	$OPTS -N "mprot_tw4m"   -l 4m   -w -t   -f /dev/zero

pipe		$OPTS -N "pipe_pst1"	-s 1	-x pipe -m st
pipe		$OPTS -N "pipe_pmt1"	-s 1	-x pipe -m mt
pipe		$OPTS -N "pipe_pmp1"	-s 1	-x pipe -m mp
pipe		$OPTS -N "pipe_pst4k"	-s 4k	-x pipe -m st
pipe		$OPTS -N "pipe_pmt4k"	-s 4k	-x pipe -m mt
pipe		$OPTS -N "pipe_pmp4k"	-s 4k	-x pipe -m mp

pipe		$OPTS -N "pipe_sst1"	-s 1	-x sock -m st
pipe		$OPTS -N "pipe_smt1"	-s 1	-x sock -m mt
pipe		$OPTS -N "pipe_smp1"	-s 1	-x sock -m mp
pipe		$OPTS -N "pipe_sst4k"	-s 4k	-x sock -m st
pipe		$OPTS -N "pipe_smt4k"	-s 4k	-x sock -m mt
pipe		$OPTS -N "pipe_smp4k"	-s 4k	-x sock -m mp

pipe		$OPTS -N "pipe_tst1"	-s 1	-x tcp  -m st
pipe		$OPTS -N "pipe_tmt1"	-s 1	-x tcp  -m mt
pipe		$OPTS -N "pipe_tmp1"	-s 1	-x tcp  -m mp
pipe		$OPTS -N "pipe_tst4k"	-s 4k	-x tcp  -m st
pipe		$OPTS -N "pipe_tmt4k"	-s 4k	-x tcp  -m mt
pipe		$OPTS -N "pipe_tmp4k"	-s 4k	-x tcp  -m mp

connection	$OPTS -N "conn_accept"	-a

close_tcp	$OPTS -N "close_tcp"
.
