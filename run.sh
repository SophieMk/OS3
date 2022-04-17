#!/bin/bash

set -e  # exit on error

gcc quicksort.c -o quicksort -pthread

./quicksort 4 <20m.txt >/dev/null &

ps $!
sleep 1
ps -o thcount $!

wait
