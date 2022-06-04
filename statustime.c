/* SPDX-License-Identifier: 0BSD */

/* Copyright 2022, Douglas Storm Hill
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

#ifdef linux
#include <sys/prctl.h>
#endif

#ifndef DISPLAY_BAT
#define DISPLAY_BAT 0
#endif

/* We need this, otherwise localtime/strftime return
 * the previous minute. Determined by trial and error. */
static const long SLEEP_EXTRA_NS = 1000000L;

#ifdef linux
/* Filenames for battery printing. */
#define BAT_PREFIX "/sys/class/power_supply/BAT0"
#define BAT_FULL BAT_PREFIX "/charge_full"
#define BAT_NOW BAT_PREFIX "/charge_now"
#define BAT_STATUS BAT_PREFIX "/status"
#endif

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

static inline int sleep_until_minute(void){
	int ret;
	struct timespec slp = time_until_minute();

	do{
		ret = thrd_sleep(&slp, &slp);
	}while(ret == -1);

	return ret;
}

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

static inline int battery_charge(const int charge_fd, const int charge_full_fd){
#if DISPLAY_BAT

	const int charge = read_charge_file(charge_fd);
	if(charge < 0)
		return -1;

	const int fcharge = read_charge_file(charge_full_fd);
	if(charge < 0)
		return -2;

	return 100*charge/fcharge;

#else /* !DISPLAY_BAT */

	(void)charge_fd;
	(void)charge_full_fd;

	return -1;

#endif
}

static inline int print_time(const int charge_fd, const int charge_full_fd){
	const time_t t = time(NULL);
	struct tm * const tbuf = localtime(&t);
	if(!tbuf)
		return 1;

	char sbuf[32];

#if DISPLAY_BAT
	const int charge = battery_charge(charge_fd, charge_full_fd);
	const int preflen = snprintf(sbuf, sizeof(sbuf), "%d%% | ", charge);
	if(preflen < 0){
		return 2;
	}else{
		const unsigned int upreflen = preflen;
		if(upreflen >= sizeof(sbuf)){
			return 3;
		}
	}
#else /* !DISPLAY_BAT */
	(void)charge_fd;
	(void)charge_full_fd;

	const int preflen = 0;
#endif

	size_t len;
	if(!(len = strftime(sbuf+preflen, sizeof(sbuf)-preflen, "%F %R\n", tbuf)))
		return 4;
	sbuf[sizeof(sbuf)-1] = '\0';

	if(write(STDOUT_FILENO, sbuf, preflen+len) != preflen+len)
		return 5;

	return 0;
}

static inline int timerslack_setup(void){
#ifdef linux
	if(prctl(PR_SET_TIMERSLACK, 1UL*10000000UL, 0UL, 0UL, 0UL))
		return -1;
#endif
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
