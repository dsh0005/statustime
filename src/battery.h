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

#include <stddef.h>

/* Open an FD for the current battery charge.
 * Returns -1 on error or if unused!
 */
int open_bat_now(void);

/* Open an FD for the full battery charge.
 * Returns -1 on error or if unused!
 */
int open_bat_full(void);

/* Print out the battery percentage into a string buffer.
 *
 * charge_fd: the FD for the current battery charge
 * charge_full_fd: the FD for the full battery charge
 *
 * Returns the nonnegative number of characters printed on success.
 */
int snprintbat(char* restrict buf, size_t len, int charge_fd, int charge_full_fd);
