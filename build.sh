#!/bin/sh
gcc -Os -D_POSIX_C_SOURCE=200809L -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -DDISPLAY_BAT=1 -Wall -Wextra -pedantic -pie -fno-plt -Wl,-z,relro,-z,now src/statustime.c -o build/statustime -s
