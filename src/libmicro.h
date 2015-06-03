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
 * Copyright 2015 Ryan Zezeski <ryan@zinascii.com>.
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef LIBMICRO_H
#define	LIBMICRO_H

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>

#define	LIBMICRO_VERSION	"1505"

#define	STRSIZE			1024

typedef struct {
	long long		re_t0;
	long long		re_t1;
} result_t;

typedef struct {
	double			sum;
	long long		count;
} histo_t;

#define	HISTOSIZE		32

/*
 * Stats computed per benchmark.
 */
typedef struct stats {
	double	st_min;
	double	st_max;
	double	st_mean;
	double	st_median;
	double	st_stddev;
} stats_t;

/*
 * Barrier stuff.
 */
typedef struct {
	int			ba_hwm;		/* barrier setpoint	*/
	int			ba_flag;	/* benchmark while true	*/
	long long		ba_deadline;	/* when to stop		*/
	int			ba_phase;	/* number of time used	*/
	int 			ba_waiters;	/* how many are waiting	*/

	pthread_mutex_t		ba_lock;
	pthread_cond_t		ba_cv;

	unsigned int		ba_tgt_samples; /* # samples to take */
	unsigned int		ba_samples;	/* # samples taken */

	double			ba_starttime;	/* test time start */
	double			ba_endtime;	/* test time end */

	stats_t			ba_raw;		/* raw stats */
	double			ba_data[1];	/* start of data ararry	*/
} barrier_t;


/*
 * Barrier interfaces.
 */
barrier_t *barrier_create(int hwm, size_t target_samples);
int barrier_destroy(barrier_t *bar);
int barrier_queue(barrier_t *bar, result_t *res);


/*
 * Callbacks implemented by the benchmark module. Any callback not
 * defined by a module will fallback to a default nop behavior.
 */
int	benchmark(void *tsd, result_t *res);
int	benchmark_init();
int	benchmark_fini();
int	benchmark_initrun();
int	benchmark_finirun();
int	benchmark_initworker();
int	benchmark_finiworker();
int	benchmark_pre(void *tsd);
int	benchmark_post(void *tsd);
int	benchmark_optswitch(int opt, char *optarg);
char	*benchmark_result();


/*
 * Variables visible to the benchmark modules.
 */
extern int			lm_argc;
extern char			**lm_argv;

extern int			lm_optD;
extern int			lm_optH;
extern char			*lm_optN;
extern int			lm_optP;
extern int			lm_optS;
extern int			lm_optT;

extern int			lm_defD;
extern int			lm_defH;
extern char			*lm_defN;
extern int			lm_defP;
extern int			lm_defS;
extern int			lm_defT;

extern char			*lm_procpath;
extern char			lm_procname[STRSIZE];
extern char 			lm_usage[STRSIZE];
extern char 			lm_optstr[STRSIZE];
extern char 			lm_header[STRSIZE];
extern size_t			lm_tsdsize;


/*
 * Utility functions.
 */
int 		getpindex();
int 		gettindex();
void 		*gettsd(int p, int t);
uint64_t 	getusecs();
uint64_t 	getnsecs();
int 		setfdlimit(unsigned int limit);
long long 	sizetoll();
int 		sizetoint();
int		fit_line(double *, double *, int, double *, double *);
void		lm_err();

/*
 * Like an assert() but for benchmark errors (instead of checking for
 * impossible behavior). Use anywhere an error may occur, especially
 * functions that dole out system resources (open(2)), or can fail for
 * a varity of reasons (write(2)).
 *
 * This cannot be used in the benchmark_init() callback as the name
 * (lm_optN) is not yet set.
 *
 * Example 1:
 *
 *     fd = open("/path/to/file", O_RDWR);
 *     LM_CHK(fd != -1);
 *
 * Example 2:
 *
 *     int p[2];
 *     LM_CHK(pipe(p) != -1);
 */
#define	LM_CHK(exp)							\
	if (exp) {} else lm_err(lm_optN, errno, __FILE__, __LINE__)

#endif /* LIBMICRO_H */
