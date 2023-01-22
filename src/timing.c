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

#include <threads.h>
#include <time.h>

#ifdef linux
/* For setting timer slack. */
#include <sys/prctl.h>
#endif

/* We need this, otherwise localtime/strftime return
 * the previous minute. Determined by trial and error. */
static const long SLEEP_EXTRA_NS = 1000000L;

/* Get the amount of time remaining before the top of the next minute. */
static inline struct timespec time_until_minute(void){
	struct timespec slp;

	/* Get the current UTC time.
	 *
	 * NOTE: We're fundamentally assuming an integer number of seconds
	 * offset from UTC. With a few exceptions (e.g. UT1), this has been
	 * the case for more or less as long as people have been able to
	 * keep time that accurately, AFAIK. */
	if(!timespec_get(&slp, TIME_UTC)){
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
	slp.tv_sec = 60 - (slp.tv_sec % 60);
	if(slp.tv_nsec){
		slp.tv_nsec = 1000000000 - slp.tv_nsec;
		if(slp.tv_sec){
			slp.tv_sec--;
		}else{
			slp.tv_sec = 59;
		}
	}

	slp.tv_nsec += SLEEP_EXTRA_NS;
	while(slp.tv_nsec >= 1000000000){
		slp.tv_nsec -= 1000000000;
		slp.tv_sec++;
	}

	return slp;
}

/* Sleep until the top of the minute. */
int sleep_until_minute(void){
	int ret;
	struct timespec slp = time_until_minute();

	do{
		ret = thrd_sleep(&slp, &slp);
	}while(ret == -1);

	return ret;
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

/* Set up the subsystem. */
int timing_setup(struct timing_context * const context){
	if(!context)
		return 1;

	if(timerslack_setup())
		return 2;

	/* TODO: fill in the context */

	return 0;
}
