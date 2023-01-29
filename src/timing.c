/* SPDX-License-Identifier: 0BSD */

/* Copyright 2023, Douglas Storm Hill
 *
 * Provided under zero-clause-BSD license.
 *
 * Use it if you like it, modify it however you like.
 * I _think_ I got all the bugs out, but you should review
 * it for yourself before assuming it's safe.
 *
 * This file is for timing related functionality.
 */
#include "timing.h"

#include <stdbool.h>
#include <time.h>
#include <errno.h>

#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef linux
/* For setting timer slack. */
#include <sys/prctl.h>
#endif

/* We need this, otherwise localtime/strftime return
 * the previous minute. Determined by trial and error. */
static const long SLEEP_EXTRA_NS = 1000000L;

/* Get the amount of time since epoch at the top of the next minute. */
static inline struct timespec time_next_minute(void){
	struct timespec slp;

	/* Get the current UTC time.
	 *
	 * NOTE: We're fundamentally assuming an integer number of seconds
	 * offset from UTC. With a few exceptions (e.g. UT1), this has been
	 * the case for more or less as long as people have been able to
	 * keep time that accurately, AFAIK. */
	if(clock_gettime(CLOCK_REALTIME, &slp)){
		slp.tv_sec = -1;
		slp.tv_nsec = -1;
		return slp;
	}

	/* Calculate how long until the top of the next minute.
	 *
	 * NOTE: In the case of a (positive) leap second, we'll update
	 * a second early then... maybe be wrong for... well, we should fire
	 * a second later. It depends on how the system does it.
	 *
	 * NOTE: In the case of a negative leap second, we'll update a second
	 * late. Oh well.
	 *
	 * TODO: Query the system for pending leap seconds?
	 *
	 * NOTE: We're assuming a multiple of 60 seconds offset from UTC,
	 * which has been the case for most systems for a very long time.
	 * Exceptions include UT1 (and other non-TAI synced systems) and
	 * Dublin Mean Time prior to 1916-10-01T0200Z. Use e.g.:
	 * grep -e '[[:digit:]]\{1,2\}:[[:digit:]]\{1,2\}:[[:digit:]]\{1,2\}' -n
	 * over your tzdata sources to find some. In my copy of 2022g, it says
	 * that Africa/Monrovia was the last, at 0:44:30 until 1972-01-07!
	 */
	if(slp.tv_nsec){
		slp.tv_sec++;
		slp.tv_nsec = 0;
	}
	slp.tv_sec += 60 - (slp.tv_sec % 60);

	slp.tv_nsec += SLEEP_EXTRA_NS;
	while(slp.tv_nsec >= 1000000000){
		slp.tv_nsec -= 1000000000;
		slp.tv_sec++;
	}

	return slp;
}

/* Sleep until the top of the minute. */
int sleep_until_minute(struct timing_context * const ctx){
	int ret;
	struct timespec next_min = time_next_minute();

	/* Set a timer to check for the clock being adjusted back. */
	struct itimerspec oversleep_time = {
		{0}, /* no repeats */
		{61, 0}, /* 61 seconds from now */
	};
	if(timer_settime(ctx->adjcheck_timer, 0, &oversleep_time, NULL))
		return 1;

	ret = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME,
		&next_min, NULL);

	/* Being interrupted is sort of successful. */
	if(ret == EINTR)
		ret = 0;

	/* Clear the oversleep timer */
	oversleep_time = (struct itimerspec){{0}, {0}};
	if(timer_settime(ctx->adjcheck_timer, 0, &oversleep_time, NULL))
		return 2;

	return ret;
}

/* Return true when struct stats are equal. */
static inline bool stateq(const struct stat a, const struct stat b){
	/* TODO: we probably want to skip some fields. */
	return ((a.st_dev          == b.st_dev) &&
	        (a.st_ino          == b.st_ino) &&
	        (a.st_mode         == b.st_mode) &&
	        (a.st_nlink        == b.st_nlink) &&
	        (a.st_uid          == b.st_uid) &&
	        (a.st_gid          == b.st_gid) &&
	        (a.st_rdev         == b.st_rdev) &&
	        (a.st_size         == b.st_size) &&
	        (a.st_blksize      == b.st_blksize) &&
	        (a.st_blocks       == b.st_blocks) &&
	        (a.st_atim.tv_sec  == b.st_atim.tv_sec) &&
	        (a.st_atim.tv_nsec == b.st_atim.tv_nsec) &&
	        (a.st_mtim.tv_sec  == b.st_mtim.tv_sec) &&
	        (a.st_mtim.tv_nsec == b.st_mtim.tv_nsec) &&
	        (a.st_ctim.tv_sec  == b.st_ctim.tv_sec) &&
	        (a.st_ctim.tv_nsec == b.st_ctim.tv_nsec));
}

/* Try to detect timezone changes. */
int has_timezone_changed(struct timing_context * const ctx){
	struct stat tz_stat = {0};
	if(stat("/etc/localtime", &tz_stat))
		return -1;

	/* Now check if they're different, and save the new results. */
	if(!stateq(ctx->timezone_stat, tz_stat)){
		ctx->timezone_stat = tz_stat;
		return 1;
	}

	return 0;
}

/* Try to set up timers to have a bit less than 1 frame of slack. */
static inline int timerslack_setup(void){
#ifdef linux
	/* NOTE: this value should change based on target framerate. */
	if(prctl(PR_SET_TIMERSLACK, 1UL*10000000UL, 0UL, 0UL, 0UL))
		return -1;
#endif
	/* Since it's just an optimization for power, not knowing
	 * how to do it is still successful-ish. */
	return 0;
}

/* Our alarm signal handler.
 * We actually take advantage of not restarting the sleep syscall
 * to get a tz change or clock adjustment check earlier.
 */
static void alarm_handler(
		const int signum,
		siginfo_t * const info,
		void * const ctx){
	/* don't care */
	(void)signum;
	(void)ctx;

	/* We could check these if we wanted. */
	/* info.si_code == SI_TIMER; */
	/* info.si_value.sival_int; */
	/* But we don't care. */
	(void)info;
}

#ifdef linux
#define PREFERRED_ADJCHECK_CLOCK CLOCK_BOOTTIME
#else
#define PREFERRED_ADJCHECK_CLOCK CLOCK_MONOTONIC
#endif

/* Set up the subsystem. */
int timing_setup(struct timing_context * const context){
	if(!context)
		return 1;

	if(timerslack_setup())
		return 2;

	struct stat tz_stat = {0};
	if(stat("/etc/localtime", &tz_stat))
		return 3;

	/* Store it and we're done with this part. */
	context->timezone_stat = tz_stat;

	/* Set up our alarm handler. */
	struct sigaction alrm_handler_info = {0};
	alrm_handler_info.sa_sigaction = alarm_handler;
	alrm_handler_info.sa_flags = SA_SIGINFO;

	/* Set the alarm signal handler to our handler. */
	if(sigaction(SIGALRM, &alrm_handler_info, NULL))
		return 4;

	/* Create the timer that will call our alarm handler. */
	if(timer_create(PREFERRED_ADJCHECK_CLOCK,
	                NULL, &context->adjcheck_timer))
	  return 5;

	return 0;
}
