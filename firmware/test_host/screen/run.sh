#!/bin/sh
# Render every screen scene to PNGs on your laptop (no ESP32 needed).
# Needs the libraries PlatformIO downloads on the first firmware build (pio run -d firmware).
set -e
cd "$(dirname "$0")"
LIB=../../.pio/libdeps/scentty
mkdir -p out
cc -O1 -w -c "$LIB/QRCode/src/qrcode.c" -o /tmp/scentty_qrcode.o
c++ -std=c++17 -O1 -w -DARDUINO=100 -DSCREEN_HOST_TEST -Istubs -I../../src -I"$LIB/Adafruit GFX Library" -I"$LIB/QRCode/src" \
  preview.cpp ../../src/display.cpp "$LIB/Adafruit GFX Library/Adafruit_GFX.cpp" /tmp/scentty_qrcode.o \
  -lz -o /tmp/scentty_screen_preview
/tmp/scentty_screen_preview
