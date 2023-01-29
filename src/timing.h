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

#ifndef STATUSTIME_TIMING_H
#define STATUSTIME_TIMING_H 1

#include <time.h>

#include <sys/stat.h>

/* Timing subsystem context/state struct.
 *
 * Contains stuff relating to watching for timezone changes, currently.
 */
struct timing_context {
	struct stat timezone_stat;
	timer_t adjcheck_timer;
};

/* Sleep until the top of the minute.
 *
 * context: pointer to the timing subsystem context.
 *
 * Returns 0 on success. */
int sleep_until_minute(struct timing_context * context);

/* Attempt to detect if the timezone has changed.
 * TODO: changed since when?
 *
 * context: pointer to the timing subsystem context.
 *
 * Returns 0 if the timezone (probably) hasn't changed.
 * Returns 1 if the timezone (probably) changed.
 * Returns negative on errors.
 */
int has_timezone_changed(struct timing_context * context);

/* Set up the timing subsystem.
 *
 * context: an instance of timing_context that this will fill in.
 *
 * Returns 0 on success.
 */
int timing_setup(struct timing_context *context);

#endif /* STATUSTIME_TIMING_H */
