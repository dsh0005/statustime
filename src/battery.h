/* SPDX-License-Identifier: 0BSD */

/* Copyright 2023, Douglas Storm Hill
 *
 * Provided under zero-clause-BSD license.
 *
 * Use it if you like it, modify it however you like.
 * I _think_ I got all the bugs out, but you should review
 * it for yourself before assuming it's safe.
 *
 * This file is for battery related functionality.
 */

#ifndef STATUSTIME_BATTERY_H
#define STATUSTIME_BATTERY_H 1

#include <stddef.h>

/* Battery subsystem context/state stuct.
 *
 * Contains stuff relating to reading battery state.
 */
struct battery_context {
	int bat_now_fd;
	int bat_full_fd;
};

/* Set up the battery subsystem.
 *
 * context: an instance of battery_context that this will fill in.
 *
 * Returns 0 on success.
 */
int battery_setup(struct battery_context *context);

/* Print out the battery percentage into a string buffer.
 *
 * context: the battery subsystem context
 *
 * Returns the nonnegative number of characters printed on success.
 */
int snprintbat(char* restrict buf, size_t len, struct battery_context context);

#endif /* STATUSTIME_BATTERY_H */
