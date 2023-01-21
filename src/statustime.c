/* SPDX-License-Identifier: 0BSD */

/* Copyright 2022, 2023, Douglas Storm Hill
 *
 * Provided under zero-clause-BSD license.
 *
 * Use it if you like it, modify it however you like.
 * I _think_ I got all the bugs out, but you should review
 * it for yourself before assuming it's safe.
 */

#include <threads.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <fcntl.h>
#include <limits.h>

#include "timing.h"

#ifndef DISPLAY_BAT
#define DISPLAY_BAT 0
#endif

#ifdef linux
/* Filenames for battery printing. */
#define BAT_PREFIX "/sys/class/power_supply/BAT0"
#define BAT_FULL BAT_PREFIX "/charge_full"
#define BAT_NOW BAT_PREFIX "/charge_now"
#define BAT_STATUS BAT_PREFIX "/status"
#endif

/* Open an FD for the current battery charge.
 * Returns -1 on error or if unused!
 */
static inline int open_bat_now(void){
#if DISPLAY_BAT

#ifdef linux

	const int bnfd = open(BAT_NOW, O_RDONLY | O_CLOEXEC);
	if(bnfd < 0)
		return -1;
	return bnfd;

#else

#error "don't know how to read current battery charge"

#endif

#else /* !DISPLAY_BAT */

	return -1;

#endif
}

/* Open an FD for the full battery charge.
 * Returns -1 on error or if unused!
 */
static inline int open_bat_full(void){
#if DISPLAY_BAT

#ifdef linux

	const int bffd = open(BAT_FULL, O_RDONLY | O_CLOEXEC);
	if(bffd < 0)
		return -1;
	return bffd;

#else

#error "don't know how to read full battery charge"

#endif

#else /* !DISPLAY_BAT */

	return -1;

#endif
}

/* Read out the charge from e.g. a sysfs file.
 *
 * Expects a text-formatted, base-10,
 * nonnegative integer of 10 digits or less.
 *
 * Returns negative on failures.
 */
static inline int read_charge_file(const int fd){
#if DISPLAY_BAT

#ifdef linux

	char cbuf[11];

	ssize_t read = pread(fd, cbuf, sizeof(cbuf)-1, 0);
	if(read < 0)
		return -1;
	cbuf[read] = '\0';

	char *ebuf;
	const long parsel = strtol(cbuf, &ebuf, 10);
	if(!*ebuf)
		return -2;

	return (int)parsel;

#else

#error "don't know how to read battery on linux"

#endif

#else /* !DISPLAY_BAT */

	(void)fd;

	return -1;

#endif
}

/* Read out the battery charge.
 *
 * charge_fd: the FD for the (current) charge file
 * charge_full_fd: the FD for the (max/full) charge file
 *
 * The FDs must be negative if they shouldn't be used.
 *
 * On success, return the charge level as a percentage.
 * Returns negative on failure.
 */
static inline int battery_charge(const int charge_fd, const int charge_full_fd){
	if(!DISPLAY_BAT)
		return -1;

	if(charge_fd < 0 || charge_full_fd < 0)
		return -2;

	const int charge = read_charge_file(charge_fd);
	if(charge < 0)
		return -3;

	const int fcharge = read_charge_file(charge_full_fd);
	if(charge < 0)
		return -4;

	return 100*charge/fcharge;
}

#if DISPLAY_BAT
#define DISPLAY_BAT_FORMAT "%d%% | "
#else
#define DISPLAY_BAT_FORMAT ""
#endif

#define WITH_DIAGNOSTIC_SURPRESSIONS(surpressions, code) \
	_Pragma("GCC diagnostic push") \
	surpressions \
	code \
	_Pragma("GCC diagnostic pop")

#define PRAGMA_STRINGY(prag) _Pragma(#prag)

#define PRAGMA_NOWARN(warning) PRAGMA_STRINGY(GCC diagnostic ignored #warning)

/* Yes, we _intentionally_ pass "" as the format sometimes.
 * And yes, in that case we pass an extra argument.
 * Although I haven't seen this one yet, be proactive in
 * surpressing it.
 */
#define WITH_POSSIBLY_ZERO_OR_EXTRA_ARGS(code) \
	WITH_DIAGNOSTIC_SURPRESSIONS( \
		PRAGMA_NOWARN(-Wformat-zero-length) \
		PRAGMA_NOWARN(-Wformat-extra-args), \
		code)

/* Print out the time.
 *
 * charge_fd: the FD for the current battery charge
 * charge_full_fd: the FD for the full battery charge
 *
 * Returns 0 on success.
 */
static inline int print_time(const int charge_fd, const int charge_full_fd){
	const time_t t = time(NULL);
	struct tm * const tbuf = localtime(&t);
	if(!tbuf)
		return 1;

	char sbuf[32];

	const int charge = battery_charge(charge_fd, charge_full_fd);

	WITH_POSSIBLY_ZERO_OR_EXTRA_ARGS(const int preflen = snprintf(sbuf, sizeof(sbuf), DISPLAY_BAT_FORMAT, charge);)

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

int main(int argc, char ** argv){
	(void)argc;
	(void)argv;

	if(timerslack_setup())
		return EXIT_FAILURE;

	const int bat_now_fd = open_bat_now();
	const int bat_full_fd = open_bat_full();

#if DISPLAY_BAT
	if(bat_now_fd < 0 || bat_full_fd < 0)
		return EXIT_FAILURE;
#endif

	while(1){
		if(print_time(bat_now_fd, bat_full_fd))
			break;
		if(sleep_until_minute())
			break;
	}
	return EXIT_FAILURE;
}
