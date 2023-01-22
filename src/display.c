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
#include "display.h"

#include <time.h>

#include <unistd.h>
#include <limits.h>

#include "battery.h"

#ifndef DISPLAY_BAT
#define DISPLAY_BAT 0
#endif

/* Print out the time.
 *
 * charge_fd: the FD for the current battery charge
 * charge_full_fd: the FD for the full battery charge
 *
 * Returns 0 on success.
 */
int print_time(const int charge_fd, const int charge_full_fd){
	const time_t t = time(NULL);
	struct tm * const tbuf = localtime(&t);
	if(!tbuf)
		return 1;

	char sbuf[32];

	const int preflen = snprintbat(sbuf, sizeof(sbuf), charge_fd, charge_full_fd);

	if(preflen < 0){
		return 2;
	}

	const unsigned int upreflen = preflen;

	if(upreflen >= sizeof(sbuf))
		return 3;

	size_t len;
	if(!(len = strftime(sbuf+upreflen, sizeof(sbuf)-preflen, "%F %R\n", tbuf)))
		return 4;

	/* Forcefully terminate it out of habit. */
	sbuf[sizeof(sbuf)-1] = '\0';

	if(len >= sizeof(sbuf))
		return 5;

	/* FIXME: how to check that upreflen+len doesn't overflow? */
	const size_t write_expect = upreflen+len;

	/* Be paranoid. Make sure we can use the result from write(2). */
	if(write_expect > SSIZE_MAX)
		return 6;

	const ssize_t swrite_expect = write_expect;

	if(write(STDOUT_FILENO, sbuf, upreflen+len) != swrite_expect)
		return 7;

	return 0;
}
