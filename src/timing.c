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

	if(!timespec_get(&slp, TIME_UTC)){
		slp.tv_sec = -1;
		slp.tv_nsec = -1;
		return slp;
	}

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
	if(slp.tv_nsec >= 1000000000){
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
int timerslack_setup(void){
#ifdef linux
	/* NOTE: this value should change based on target framerate. */
	if(prctl(PR_SET_TIMERSLACK, 1UL*10000000UL, 0UL, 0UL, 0UL))
		return -1;
#endif
	/* Since it's just an optimization for power, not knowing
	 * how to do it is still successful-ish. */
	return 0;
}
