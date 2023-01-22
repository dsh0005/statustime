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
#include <threads.h>
#include <time.h>

#include <fcntl.h>
#include <sys/stat.h>

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
	/* What we have to do depends on if /etc/localtime is a symlink or not. */
	if(ctx->localtime_link_fd != -1){
		/* Yes, it is a symlink. Check both the link and where
		 * it (used to?) go. */

		/* Check the link part first. */
		struct stat link_stat = {0};
		if(fstatat(ctx->localtime_link_fd, "", &link_stat, AT_EMPTY_PATH | AT_SYMLINK_NOFOLLOW))
			return -1;

		/* Check where it goes now. */
		struct stat tz_stat = {0};
		if(fstatat(ctx->timezone_fd, "", &tz_stat, AT_EMPTY_PATH))
			return -2;

		/* Now check if they're different, and save the new results. */
		int was_different = 0;
		if(!stateq(ctx->localtime_link_stat, link_stat)){
			was_different = 1;
			ctx->localtime_link_stat = link_stat;
		}
		if(!stateq(ctx->timezone_stat, tz_stat)){
			was_different = 1;
			ctx->timezone_stat = tz_stat;
		}

		return was_different;
	}else{
		/* No, it's not a symlink. Check just the file itself. */
		struct stat tz_stat = {0};
		if(fstatat(ctx->timezone_fd, "", &tz_stat, AT_EMPTY_PATH | AT_SYMLINK_NOFOLLOW))
			return -3;

		/* Now check if they're different, and save the new results. */
		if(!stateq(ctx->timezone_stat, tz_stat)){
			ctx->timezone_stat = tz_stat;
			return 1;
		}

		return 0;
	}
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

	/* Open the /etc/localtime 'path'. */
	const int localtime_path = open("/etc/localtime", O_RDONLY | O_CLOEXEC | O_PATH | O_NOFOLLOW);
	if(localtime_path == -1)
		return 3;

	/* Check if that's the symlink or not. */
	struct stat localtime_stat = {0};
	if(fstatat(localtime_path, "", &localtime_stat, AT_EMPTY_PATH | AT_SYMLINK_NOFOLLOW))
		return 4;

	if(S_ISLNK(localtime_stat.st_mode)){
		/* /etc/localtime is a symlink, so we also need to look
		 * where it's going.
		 *
		 * NOTE: We're assuming that there are no intermediary symlinks, or
		 * at least none that get changed. */
		context->localtime_link_fd = localtime_path;
		context->localtime_link_stat = localtime_stat;

		const int tz_path = open("/etc/localtime", O_RDONLY | O_CLOEXEC | O_PATH);
		if(tz_path == -1)
			return 5;
		/* NOTE: there _could_ be a TOCTTOU problem between the two open(2)s,
		 * but actually we'll catch that on a later check anyways. 
		 *
		 * Having said that, there could be a TOCTTOU+ABA problem. I'm
		 * not sure what to do about it, though. */
		context->timezone_fd = tz_path;

		struct stat tz_stat = {0};
		if(fstatat(tz_path, "", &tz_stat, AT_EMPTY_PATH))
			return 6;

		/* Store it and we're done with this part. */
		context->timezone_stat = tz_stat;
	}else{
		/* That's the file itself. Mark the link as not used, save the
		 * stat, and we're good to go. */
		context->localtime_link_fd = -1;
		context->localtime_link_stat = (struct stat){0};
		context->timezone_fd = localtime_path;
		context->timezone_stat = localtime_stat;
	}

	return 0;
}
