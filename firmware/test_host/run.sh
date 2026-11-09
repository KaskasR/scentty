#!/bin/sh
# Compile and run the pump state-machine tests on your laptop (no ESP32 needed).
#   firmware/test_host/run.sh       quiet
#   firmware/test_host/run.sh -v    also print the firmware's serial log
set -e
cd "$(dirname "$0")"
c++ -std=c++17 -Wall -Wextra -O1 -Istubs -I../src test_pumps.cpp ../src/pumps.cpp -o /tmp/scentty_test_pumps
/tmp/scentty_test_pumps "$@"
