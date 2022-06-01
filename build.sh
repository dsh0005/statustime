#!/bin/sh
gcc -Os -D_POSIX_C_SOURCE=200809L -D_FORTIFY_SOURCE=2 -Wall -Wextra -pedantic -pie -fno-plt -Wl,-z,relro,-z,now statustime.c -o statustime -s
