#!/bin/sh
gcc -Os -D_POSIX_C_SOURCE=200809L -Wall -Wextra -pedantic -pie -fno-plt statustime.c -o statustime -s
