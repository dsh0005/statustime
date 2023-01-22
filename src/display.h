/* SPDX-License-Identifier: 0BSD */

/* Copyright 2023, Douglas Storm Hill
 *
 * Provided under zero-clause-BSD license.
 *
 * Use it if you like it, modify it however you like.
 * I _think_ I got all the bugs out, but you should review
 * it for yourself before assuming it's safe.
 *
 * This file is for display/printing the status widget.
 */

#ifndef STATUSTIME_DISPLAY_H
#define STATUSTIME_DISPLAY_H 1

/* Print out the time.
 *
 * charge_fd: the FD for the current battery charge
 * charge_full_fd: the FD for the full battery charge
 *
 * Returns 0 on success.
 */
int print_time(int charge_fd, int charge_full_fd);

#endif /* STATUSTIME_DISPLAY_H */
