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

/* Sleep until the top of the minute.
 * Returns 0 on success. */
int sleep_until_minute(void);

/* Try to set up timers to have a bit less than 1 frame of slack.
 * Returns 0 on success. */
int timerslack_setup(void);

#endif /* STATUSTIME_TIMING_H */
