/* SPDX-License-Identifier: 0BSD */

/* Copyright 2022, 2023, Douglas Storm Hill
 *
 * Provided under zero-clause-BSD license.
 *
 * Use it if you like it, modify it however you like.
 * I _think_ I got all the bugs out, but you should review
 * it for yourself before assuming it's safe.
 */

#include <stdlib.h>

#include "timing.h"
#include "battery.h"
#include "display.h"

#ifndef DISPLAY_BAT
#define DISPLAY_BAT 0
#endif

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
