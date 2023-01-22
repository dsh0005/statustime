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
#include <stdio.h>

#include <unistd.h>

#include "timing.h"
#include "battery.h"
#include "display.h"

#ifndef DISPLAY_BAT
#define DISPLAY_BAT 0
#endif

int main(int argc, char ** argv){
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

		/* Check for timezone change.
		 *
		 * Yeah, glibc does this for us, but musl doesn't, and I don't want
		 * to assume other libcs do, either. Getting localtime(3) to use
		 * the new timezone might be difficult, but we don't have much state,
		 * so we can just re-exec ourselves. */
		const int tz_change = has_timezone_changed(&time_ctx);
		if(tz_change < 0){
			return EXIT_FAILURE;
		}else if(tz_change){
			fputs("detected timezone change, re-execing\n", stderr);
			/* TODO: exec ourselves */
			if(argc){
				execvp(argv[0], argv);
				perror("re-exec failed");
				return EXIT_FAILURE;
			}else{
				fputs("argc == 0, cannot re-exec", stderr);
				return EXIT_FAILURE;
			}
		}
	}
	return EXIT_FAILURE;
}
