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
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include <pthread.h>
#include <dlfcn.h>
#include <errno.h>
#include <sys/resource.h>
#include <math.h>
#include <limits.h>

#include "libmicro.h"

/*
 * Globals visible by benchmark modules.
 */
int				lm_argc = 0;
char **				lm_argv = NULL;

int				lm_opt1;
int				lm_optA;
unsigned int			lm_optC = 100;
int				lm_optD;
int				lm_optE;
int				lm_optH;
int				lm_optL = 0;
int				lm_optM = 0;
char				*lm_optN;
int				lm_optP;
int				lm_optS;
int				lm_optT;

int				lm_def1 = 0;
int				lm_defD = 5;
int				lm_defH = 0;
char				*lm_defN = NULL;
int				lm_defP = 1;

int				lm_defS = 0;
int				lm_defT = 1;

char				*lm_procpath;
char				lm_procname[STRSIZE];
char				lm_usage[STRSIZE];
char				lm_optstr[STRSIZE];
char				lm_header[STRSIZE];
size_t				lm_tsdsize = 0;

/*
 * Globals not visible to the benchmark modules.
 */
static barrier_t		*lm_barrier;
static pid_t			*pids = NULL;
static pthread_t		*tids = NULL;
static int			pindex = -1;
static void			*tsdseg = NULL;
static size_t			tsdsize = 0;

static void 		worker_process();
static void 		usage();
static void 		print_stats(barrier_t *);
static void 		print_histo(barrier_t *);
static long long	nsecs_overhead;
static long long	nsecs_resolution;
static long long	get_nsecs_overhead();
static int		crunch_stats(double *, int, stats_t *);
static void 		compute_stats(barrier_t *);

/*
 * Main routine renamed to allow linking with other files
 */
int
actual_main(int argc, char *argv[])
{
	int			i;
	int			opt;
	extern char		*optarg;
	char			*tmp;
	char			optstr[2048];
	barrier_t		*b;
	long long		startnsecs;

	startnsecs = getnsecs();

	lm_argc = argc;
	lm_argv = argv;

	(void) benchmark_init();

	nsecs_overhead = get_nsecs_overhead();
	nsecs_resolution = 1;

	/* Set defaults */
	lm_opt1	= lm_def1;
	lm_optD	= lm_defD;
	lm_optH	= lm_defH;
	lm_optN	= lm_defN;
	lm_optP	= lm_defP;

	lm_optS	= lm_defS;
	lm_optT	= lm_defT;

	/*
	 * Squirrel away the path to the current binary in a way that
	 * works on both Linux and Solaris.
	 */
	if (*argv[0] == '/') {
		lm_procpath = strdup(argv[0]);
		*strrchr(lm_procpath, '/') = 0;
	} else {
		char path[1024];
		(void) getcwd(path, 1024);
		(void) strcat(path, "/");
		(void) strcat(path, argv[0]);
		*strrchr(path, '/') = 0;
		lm_procpath = strdup(path);
	}

	/* Name of binary. */
	if ((tmp = strrchr(argv[0], '/')) == NULL)
		(void) strcpy(lm_procname, argv[0]);
	else
		(void) strcpy(lm_procname, tmp + 1);

	if (lm_optN == NULL) {
		lm_optN = lm_procname;
	}

	/* Parse command line arguments. */
	(void) sprintf(optstr, "1AB:C:D:EHI:LMN:P:RST:VW?%s", lm_optstr);
	while ((opt = getopt(argc, argv, optstr)) != -1) {
		switch (opt) {
		case '1':
			lm_opt1 = 1;
			break;
		case 'A':
			lm_optA = 1;
			break;
		case 'C':
			lm_optC = sizetoint(optarg);
			break;
		case 'D':
			lm_optD = sizetoint(optarg);
			break;
		case 'E':
			lm_optE = 1;
			break;
		case 'H':
			lm_optH = 1;
			break;
		case 'L':
			lm_optL = 1;
			break;
		case 'M':
			lm_optM = 1;
			break;
		case 'N':
			lm_optN = optarg;
			break;
		case 'P':
			lm_optP = sizetoint(optarg);
			break;
		case 'S':
			lm_optS = 1;
			break;
		case 'T':
			lm_optT = sizetoint(optarg);
			break;
		case 'V':
			(void) printf("%s %s\n", LIBMICRO_VERSION, LM_VSN_HASH);
			exit(0);
			break;
		case '?':
			usage();
			exit(0);
			break;
		default:
			if (benchmark_optswitch(opt, optarg) == -1) {
				usage();
				exit(0);
			}
		}
	}

	/* Deal with implicit and overriding options. */
	if (lm_opt1 && lm_optP > 1) {
		lm_optP = 1;
		(void) printf("warning: -1 overrides -P\n");
	}

	if (lm_optE) {
		(void) fprintf(stderr, "Running:%20s", lm_optN);
		(void) fflush(stderr);
	}


	LM_CHK(benchmark_initrun() != -1);

	/* Allocate dynamic data. */
	pids = (pid_t *)malloc(lm_optP * sizeof (pid_t));
	LM_CHK(pids != NULL);
	tids = (pthread_t *)malloc(lm_optT * sizeof (pthread_t));
	LM_CHK(tids != NULL);

	/*
	 * Check that the case defines lm_tsdsize before proceeding.
	 */
	if (lm_tsdsize == (size_t)-1) {
		(void) fprintf(stderr, "error in benchmark_init: "
		    "lm_tsdsize not set\n");
		exit(1);
	}

	/*
	 * Round up tsdsize to nearest 128 to eliminate false
	 * sharing.
	 */
	tsdsize = ((lm_tsdsize + 127) / 128) * 128;

	/* Allocate sufficient TSD for each thread in each process. */
	tsdseg = (void *)mmap(NULL, lm_optT * lm_optP * tsdsize + 8192,
	    PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0L);
	if (tsdseg == NULL) {
		perror("mmap(tsd)");
		exit(1);
	}

	/* Initialise worker synchronisation. */

	/* Option -C specifies number of samples per thread. */
	b = barrier_create(lm_optT * lm_optP, lm_optC * lm_optP * lm_optT);
	if (b == NULL) {
		perror("barrier_create()");
		exit(1);
	}
	lm_barrier = b;
	b->ba_flag = 1;

	/* Flush so that parent and children can call exit(). */
	(void) fflush(stdout);
	(void) fflush(stderr);

	/* When we started and when to stop. */
	b->ba_starttime = getnsecs();
	b->ba_deadline = (b->ba_starttime + SECS_TO_NANOS(lm_optD));

	/* Do the work. */
	if (lm_opt1) {
		/* Single process, non-fork mode. */
		pindex = 0;
		worker_process();
	} else {
		/* Create worker processes. */
		for (i = 0; i < lm_optP; i++) {
			pids[i] = fork();

			switch (pids[i]) {
			case 0:
				pindex = i;
				worker_process();
				exit(0);
				break;
			case -1:
				perror("fork");
				exit(1);
				break;
			default:
				continue;
			}
		}

		/* Wait for worker processes. */
		for (i = 0; i < lm_optP; i++) {
			if (pids[i] > 0) {
				int chldstat = 0;

				(void) waitpid(pids[i], &chldstat, 0);

				/*
				 * If the benchmark failed there is no
				 * more work to be done.
				 */
				if (chldstat != 0) {
					exit(1);
				}
			}
		}
	}

	b->ba_endtime = getnsecs();
	compute_stats(b);

	/* Cleanup. */
	(void) benchmark_finirun();
	(void) benchmark_fini();

	if (lm_optE) {
		(void) fprintf(stderr, " for %12.5f seconds\n",
		    (double)(getnsecs() - startnsecs) /
		    1.e9);
		(void) fflush(stderr);
	}

	/* Print benchmark arguments. */
	if (lm_optL) {
		int l;
		(void) printf("# %s ", argv[0]);
		for (l = 1; l < argc; l++) {
			(void) printf("%s ", argv[l]);
		}
		(void) printf("\n");
	}

	/* Print result header. */
	if (!lm_optH) {
		(void) printf("%12s %3s %3s %12s %12s %s\n",
		    "", "prc", "thr",
		    "usecs/call",
		    "samples", lm_header);
	}

	/* Print result. */
	(void) printf("%-12s %3d %3d %12.5f %12d %s\n",
	    lm_optN, lm_optP, lm_optT,
	    (lm_optM?b->ba_raw.st_mean:b->ba_raw.st_median),
	    b->ba_samples,
	    benchmark_result());

	if (lm_optS) {
		print_stats(b);
	}

	/* Just incase something goes awry. */
	(void) fflush(stdout);
	(void) fflush(stderr);

	(void) barrier_destroy(b);

	return (0);
}

void *
worker_thread(void *arg)
{
	result_t	r;
	long long	last_sleep = 0;
	long long	t;

	LM_CHK(benchmark_initworker(arg) == 0);

	while (lm_barrier->ba_flag) {
		LM_CHK(benchmark_pre(arg) == 0);

		/* Sync to clock. */
		if (lm_optA && ((t = getnsecs()) - last_sleep) > 75000000LL) {
			(void) poll(0, 0, 10);
			last_sleep = t;
		}

		/* Wait for it. */
		(void) barrier_queue(lm_barrier, NULL);

		/* Run benchmark. */
		r.re_t0 = getnsecs();
		LM_CHK(benchmark(arg, &r) == 0);
		r.re_t1 = getnsecs();

		/*
		 * Stop if either:
		 *
		 * a) The maximum amount of time has been spent.
		 *
		 * b) The target number of samples has been met.
		 */
		if (r.re_t1 > lm_barrier->ba_deadline ||
		    (lm_barrier->ba_tgt_samples == lm_barrier->ba_samples)) {
			lm_barrier->ba_flag = 0;
		}

		/* Record results and sync. */
		(void) barrier_queue(lm_barrier, &r);
		LM_CHK(benchmark_post(arg) == 0);
	}

	LM_CHK(benchmark_finiworker(arg) == 0);

	return (0);
}

void
worker_process()
{
	int			i;
	void			*tsd;

	for (i = 1; i < lm_optT; i++) {
		tsd = gettsd(pindex, i);
		if (pthread_create(&tids[i], NULL, worker_thread, tsd) != 0) {
			perror("pthread_create");
			exit(1);
		}
	}

	tsd = gettsd(pindex, 0);
	(void) worker_thread(tsd);

	for (i = 1; i < lm_optT; i++) {
		(void) pthread_join(tids[i], NULL);
	}
}

void
usage()
{
	(void) printf(
	    "usage: %s\n"
	    "       [-1] (single process; overrides -P > 1)\n"
	    "       [-A] (align with clock)\n"
	    "       [-C target number of samples per thread (default 100)]\n"
	    "       [-D maximum duration in secs (default %d secs)]\n"
	    "       [-E (echo name to stderr)]\n"
	    "       [-H] (suppress headers)\n"
	    "       [-L] (print argument line)\n"
	    "       [-M] (reports mean rather than median)\n"
	    "       [-N test-name (default '%s')]\n"
	    "       [-P processes (default %d)]\n"
	    "       [-S] (print detailed stats)\n"
	    "       [-T threads (default %d)]\n"
	    "       [-V] (print the libMicro version and exit)\n"
	    "%s\n",
	    lm_procname,
	    lm_defD, lm_procname, lm_defP, lm_defT,
	    lm_usage);
}

void
print_stats(barrier_t *b)
{
	(void) printf("#\n");
	(void) printf("# STATISTICS         %12s\n", "usecs/call");

	if (b->ba_samples == 0) {
		(void) printf("zero samples\n");
		return;
	}

	(void) printf("#                    min %12.5f\n",
	    b->ba_raw.st_min);
	(void) printf("#                    max %12.5f\n",
	    b->ba_raw.st_max);
	(void) printf("#                  range %12.5f\n",
	    b->ba_raw.st_range);
	(void) printf("#                    95%% %12.5f\n",
	    b->ba_raw.st_p95);
	(void) printf("#                    99%% %12.5f\n",
	    b->ba_raw.st_p99);
	(void) printf("#                   mean %12.5f\n",
	    b->ba_raw.st_mean);
	(void) printf("#                 median %12.5f\n",
	    b->ba_raw.st_median);
	(void) printf("#                 stddev %12.5f\n",
	    b->ba_raw.st_stddev);
	(void) printf("#\n");

	(void) printf("#           elasped time %12.5f\n", (b->ba_endtime -
	    b->ba_starttime) / 1.0e9);
	(void) printf("#      number of samples %12d\n",   b->ba_samples);
	(void) printf("#      getnsecs overhead %12d\n", (int)nsecs_overhead);

	(void) printf("#\n");
	(void) printf("# DISTRIBUTION\n");

	print_histo(b);
}

void
add_sample(barrier_t *b, result_t *r)
{
	double			time;

	time = (double)r->re_t1 - (double)r->re_t0 - (double)nsecs_overhead;
	b->ba_data[b->ba_samples] = time;
	b->ba_samples++;
}

barrier_t *
barrier_create(int hwm, size_t target_samples)
{
	pthread_mutexattr_t	attr;
	pthread_condattr_t	cattr;
	barrier_t		*b;

	b = (barrier_t *)mmap(NULL,
	    sizeof (barrier_t) + (target_samples - 1) * sizeof (double),
	    PROT_READ | PROT_WRITE,
	    MAP_SHARED | MAP_ANON, -1, 0L);
	if (b == (barrier_t *)MAP_FAILED) {
		return (NULL);
	}

	b->ba_hwm = hwm;
	b->ba_flag = 0;
	b->ba_tgt_samples = target_samples;

	(void) pthread_mutexattr_init(&attr);
	(void) pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

	(void) pthread_condattr_init(&cattr);
	(void) pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);

	(void) pthread_mutex_init(&b->ba_lock, &attr);
	(void) pthread_cond_init(&b->ba_cv, &cattr);

	b->ba_waiters = 0;
	b->ba_phase = 0;

	return (b);
}

int
barrier_destroy(barrier_t *b)
{
	(void) munmap((void *)b, sizeof (barrier_t));

	return (0);
}

int
barrier_queue(barrier_t *b, result_t *r)
{
	int			phase;

	(void) pthread_mutex_lock(&b->ba_lock);

	if (r != NULL && lm_barrier->ba_flag) {
		add_sample(b, r);
	}

	phase = b->ba_phase;

	b->ba_waiters++;
	if (b->ba_hwm == b->ba_waiters) {
		b->ba_waiters = 0;
		b->ba_phase++;
		(void) pthread_cond_broadcast(&b->ba_cv);
	}

	while (b->ba_phase == phase) {
		(void) pthread_cond_wait(&b->ba_cv, &b->ba_lock);
	}

	(void) pthread_mutex_unlock(&b->ba_lock);
	return (0);
}

int
gettindex()
{
	int			i;

	if (tids == NULL) {
		return (-1);
	}

	for (i = 1; i < lm_optT; i++) {
		if (pthread_self() == tids[i]) {
			return (i);
		}
	}

	return (0);
}

int
getpindex()
{
	return (pindex);
}

void *
gettsd(int p, int t)
{
	if ((p < 0) || (p >= lm_optP) || (t < 0) || (t >= lm_optT))
		return (NULL);

	return ((void *)((unsigned long)tsdseg +
	    (((p * lm_optT) + t) * tsdsize)));
}

uint64_t
getusecs()
{
	struct timeval tv;
	(void) gettimeofday(&tv, NULL);
	return (((uint64_t)tv.tv_sec * 1000000L) + tv.tv_usec);
}

#if defined(__sun) || defined(__linux)

uint64_t
getnsecs()
{
	struct timespec ts;
	(void) clock_gettime(CLOCK_MONOTONIC, &ts);
	return (((uint64_t)ts.tv_sec * 1000000000L) + ts.tv_nsec);
}

#elif defined(__APPLE__)

#include <mach/mach_time.h>

/*
 * There are plenty of articles out there explaining the use of
 * mach_absolute_time() for high resolution timing on OSX. Many of
 * them are dated -- don't hurt yourself.
 *
 * https://developer.apple.com/library/mac/qa/qa1398/_index.html
 * http://stackoverflow.com/questions/23378063/\
 * how-can-i-use-mach-absolute-time-without-overflowing
 *
 */
uint64_t
getnsecs()
{
	static mach_timebase_info_data_t	tbi;

	if (tbi.denom == 0) {
		(void) mach_timebase_info(&tbi);
		/*
		 * See clock_timebase_info() in xnu/osfmk/i386/rtclock.c
		 *
		 * Numer & denom are always 1 because
		 * mach_absolute_time() is already scaled to nanos on
		 * modern hardware. If assert fails then either things
		 * have changed or running on older hardware.
		 *
		 */
		assert(tbi.numer == 1);
		assert(tbi.denom == 1);
	}

	return (mach_absolute_time());
}

#else

#error "Unsupported platform."

#endif /* defined(__sun) || defined(__linux) */

int
setfdlimit(unsigned int limit)
{
	struct rlimit rlimit;

	if (getrlimit(RLIMIT_NOFILE, &rlimit) < 0) {
		perror("getrlimit");
		exit(1);
	}

	if (rlimit.rlim_cur > limit)
		return (0); /* no worries */

	rlimit.rlim_cur = limit;

	if (rlimit.rlim_max < limit)
		rlimit.rlim_max = limit;

	if (setrlimit(RLIMIT_NOFILE, &rlimit) < 0) {
		perror("setrlimit");
		exit(3);
	}

	return (0);
}


#define	KILOBYTE		1024
#define	MEGABYTE		(KILOBYTE * KILOBYTE)
#define	GIGABYTE		(KILOBYTE * MEGABYTE)

long long
sizetoll(const char *arg)
{
	int			len = strlen(arg);
	int			i;
	long long		mult = 1;

	if (len && isalpha(arg[len - 1])) {
		switch (arg[len - 1]) {

		case 'k':
		case 'K':
			mult = KILOBYTE;
			break;
		case 'm':
		case 'M':
			mult = MEGABYTE;
			break;
		case 'g':
		case 'G':
			mult = GIGABYTE;
			break;
		default:
			return (-1);
		}

		for (i = 0; i < len - 1; i++)
			if (!isdigit(arg[i]))
				return (-1);
	}

	return (mult * strtoll(arg, NULL, 10));
}

int
sizetoint(const char *arg)
{
	int			len = strlen(arg);
	int			i;
	long long		mult = 1;

	if (len && isalpha(arg[len - 1])) {
		switch (arg[len - 1]) {

		case 'k':
		case 'K':
			mult = KILOBYTE;
			break;
		case 'm':
		case 'M':
			mult = MEGABYTE;
			break;
		case 'g':
		case 'G':
			mult = GIGABYTE;
			break;
		default:
			return (-1);
		}

		for (i = 0; i < len - 1; i++)
			if (!isdigit(arg[i]))
				return (-1);
	}

	return (mult * atoi(arg));
}

static void
print_bar(long count, long total)
{
	int			i;

	(void) putchar_unlocked(count ? '*' : ' ');
	for (i = 1; i < (32 * count) / total; i++)
		(void) putchar_unlocked('*');
	for (; i < 32; i++)
		(void) putchar_unlocked(' ');
}

static int
doublecmp(const void *p1, const void *p2)
{
	double a = *((double *)p1);
	double b = *((double *)p2);

	if (a > b)
		return (1);
	if (a < b)
		return (-1);
	return (0);
}

static void
print_histo(barrier_t *b)
{
	int			n;
	int			i;
	int			j;
	int			last;
	long long		maxcount;
	double			sum;
	long long		min;
	long long		scale;
	double			x;
	long long		y;
	long long		count;
	int			i95;
	double			p95;
	double			r95;
	double			m95;
	histo_t			*histo;

	(void) printf("#	%12s %12s %32s %12s\n", "counts", "usecs/call",
	    "", "means");

	n = b->ba_samples;

	/* Find the 95th percentile - index, value and range. */
	qsort((void *)b->ba_data, n, sizeof (double), doublecmp);
	min = b->ba_data[0] + 0.000001;
	i95 = n * 95 / 100;
	p95 = b->ba_data[i95];
	r95 = p95 - min + 1;

	/* Find a suitable min and scale. */
	i = 0;
	x = r95 / (HISTOSIZE - 1);
	while (x >= 10.0) {
		x /= 10.0;
		i++;
	}
	y = x + 0.9999999999;
	while (i > 0) {
		y *= 10;
		i--;
	}
	min /= y;
	min *= y;
	scale = y * (HISTOSIZE - 1);
	if (scale < (HISTOSIZE - 1)) {
		scale = (HISTOSIZE - 1);
	}

	/* Create and initialise the histogram. */
	histo = malloc(HISTOSIZE * sizeof (histo_t));
	for (i = 0; i < HISTOSIZE; i++) {
		histo[i].sum = 0.0;
		histo[i].count = 0;
	}

	/* Populate the histogram. */
	last = 0;
	sum = 0.0;
	count = 0;
	for (i = 0; i < i95; i++) {
		j = (HISTOSIZE - 1) * (b->ba_data[i] - min) / scale;

		if (j >= HISTOSIZE) {
			(void) printf("panic!\n");
			j = HISTOSIZE - 1;
		}

		histo[j].sum += b->ba_data[i];
		histo[j].count++;

		sum += b->ba_data[i];
		count++;
	}
	m95 = sum / count;

	/* Find the largest bucket. */
	maxcount = 0;
	for (i = 0; i < HISTOSIZE; i++)
		if (histo[i].count > 0) {
			last = i;
			if (histo[i].count > maxcount)
				maxcount = histo[i].count;
		}

	/* Print the buckets. */
	for (i = 0; i <= last; i++) {
		(void) printf("#       %12lld %12.5f |", histo[i].count,
		    (min + scale * (double)i / (HISTOSIZE - 1)));

		print_bar(histo[i].count, maxcount);

		if (histo[i].count > 0)
			(void) printf("%12.5f\n",
			    histo[i].sum / histo[i].count);
		else
			(void) printf("%12s\n", "-");
	}

	/* Find the mean of values beyond the 95th percentile. */
	sum = 0.0;
	count = 0;
	for (i = i95; i < n; i++) {
		sum += b->ba_data[i];
		count++;
	}

	/* Print the >95% bucket summary. */
	(void) printf("#\n");
	(void) printf("#       %12lld %12s |", count, "> 95%");
	print_bar(count, maxcount);
	if (count > 0)
		(void) printf("%12.5f\n", sum / count);
	else
		(void) printf("%12s\n", "-");
	(void) printf("#\n");
	(void) printf("#       %12s %12.5f\n", "mean of 95%", m95);
	(void) printf("#       %12s %12.5f\n", "95th %ile", p95);
}

static void
compute_stats(barrier_t *b)
{
	/* convert to usecs/call. */
	for (unsigned int i = 0; i < b->ba_samples; i++)
		b->ba_data[i] /= 1000.0;

	/* Calculate raw stats. */
	if (b->ba_samples > 0)
		(void) crunch_stats(b->ba_data, b->ba_samples, &b->ba_raw);
}

/*
 * Compute various statistics on array of doubles.
 */
static int
crunch_stats(double *data, int count, stats_t *stats)
{
	double std;
	double diff;
	double mean;
	int i;
	int bytes;
	double *dupdata;

	/* Calculate the mean. */
	mean = 0.0;

	for (i = 0; i < count; i++) {
		mean += data[i];
	}

	mean /= count;

	stats->st_mean = mean;

	/* Calculate the median. */
	dupdata = malloc(bytes = sizeof (double) * count);
	(void) memcpy(dupdata, data, bytes);
	qsort((void *)dupdata, count, sizeof (double), doublecmp);
	stats->st_median = dupdata[count/2];

	stats->st_p95	= dupdata[(count * 95) / 100];
	stats->st_p99	= dupdata[(count * 99) / 100];

	free(dupdata);

	std = 0.0;

	stats->st_max = -1;
	stats->st_min = 1.0e99; /* Hard to find portable values. */

	for (i = 0; i < count; i++) {
		if (data[i] > stats->st_max)
			stats->st_max = data[i];
		if (data[i] < stats->st_min)
			stats->st_min = data[i];

		diff = data[i] - mean;
		std += diff * diff;
	}

	stats->st_range = stats->st_max - stats->st_min;
	stats->st_stddev = sqrt(std/(double)(count - 1));

	assert(stats->st_min <= stats->st_p95);
	assert(stats->st_p95 <= stats->st_p99);
	assert(stats->st_p99 <= stats->st_max);

	return (0);
}

/*
 * Does a least squares fit to the set of points x, y and
 * fits a line y = a + bx. Returns a, b.
 */
int
fit_line(double *x, double *y, int count, double *a, double *b)
{
	double sumx, sumy, sumxy, sumx2;
	double denom;
	int i;

	sumx = sumy = sumxy = sumx2 = 0.0;

	for (i = 0; i < count; i++) {
		sumx	+= x[i];
		sumx2	+= x[i] * x[i];
		sumy	+= y[i];
		sumxy	+= x[i] * y[i];
	}

	denom = count * sumx2 - sumx * sumx;

	if (denom == 0.0)
		return (-1);

	*a = (sumy * sumx2 - sumx * sumxy) / denom;

	*b = (count * sumxy - sumx * sumy) / denom;

	return (0);
}

/*
 * Empty function for measurement purposes.
 */
int
nop()
{
	return (1);
}

#define	NSECITER 1000

static long long
get_nsecs_overhead()
{
	long long s;

	double data[NSECITER];
	stats_t stats;

	int i;
	int count;

	/* Warmup. */
	(void) getnsecs();
	(void) getnsecs();
	(void) getnsecs();

	i = 0;

	count = NSECITER;

	for (i = 0; i < count; i++) {
		s = getnsecs();
		data[i] = getnsecs() - s;
	}

	(void) crunch_stats(data, count, &stats);

	return ((long long)stats.st_mean);

}

/*
 * Indicate a benchmark failure has occurred. When an error occurs the
 * benchmark is most likely in a bad state and all bets are off:
 * report the where and why of the error and exit the process.
 *
 * You almost always want to use the LM_CHK macro instead of calling
 * this function directly.
 *
 */
void
lm_err(char *name, int errnum, char *file, int line)
{
	/* Log where and why benchmark failed. */
	printf("%s ERROR %d %s %s:%d\n",
	    name, errnum, strerror(errnum), file, line);
	fflush(stdout);

	/* If reporting status to stderr (-E) then indicate
	 * failure. */
	if (lm_optE) {
		fprintf(stderr, " FAIL\n");
		fflush(stderr);
	}

	exit(1);
}
