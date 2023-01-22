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

	struct timing_context time_ctx = {0};
	if(timing_setup(&time_ctx))
		return EXIT_FAILURE;

	struct battery_context bat_ctx = {0};
	if(battery_setup(&bat_ctx))
		return EXIT_FAILURE;

	while(1){
		if(print_time(bat_ctx))
			break;
		if(sleep_until_minute())
			break;
	}
	return EXIT_FAILURE;
}
