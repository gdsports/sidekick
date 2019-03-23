#!/bin/bash
IDEVER="1.8.8"
TEENSYVER="1.45"
WORKDIR="/tmp/autobuild_$$"
mkdir -p ${WORKDIR}
# Install Ardino IDE with teensyduino in work directory
TARFILE="${HOME}/Downloads/arduino-${IDEVER}-teensyduino-${TEENSYVER}.tbz"
if [ -f ${TARFILE} ]
then
    tar xf ${TARFILE} -C ${WORKDIR}
else
    exit -1
fi
# Create portable sketchbook and library directories
IDEDIR="${WORKDIR}/arduino-${IDEVER}"
LIBDIR="${IDEDIR}/portable/sketchbook/libraries"
mkdir -p "${LIBDIR}"
export PATH="${IDEDIR}:${PATH}"
cd ${IDEDIR}
which arduino
# Install board package
#arduino --pref "compiler.warning_level=default" --save-prefs
arduino --install-boards "arduino:samd"
arduino --pref "boardsmanager.additional.urls=https://adafruit.github.io/arduino-board-index/package_adafruit_index.json" --save-prefs
arduino --install-boards "adafruit:samd"
arduino --install-library "Adafruit DotStar"
BOARD="adafruit:samd:adafruit_trinket_m0"
arduino --board "${BOARD}" --save-prefs
CC="arduino --verify --board ${BOARD}"
cd ${IDEDIR}/portable/sketchbook
ln -s ~/Sync/sidekick .
find . -name '*.ino' -print0 | xargs -0 -n 1 $CC >/tmp/tm0_$$.txt 2>&1
BOARD="teensy:avr:teensyLC"
arduino --pref "compiler.warning_level=default" \
		--pref "custom_usb=teensyLC_hid" \
        --pref "custom_keys=teensyLC_en-us" \
		--pref "custom_opt=teensyLC_o2std" \
        --pref "custom_speed=teensyLC_48" --save-prefs
CC="arduino --verify --board ${BOARD}"
find . -name '*.ino' -print0 | xargs -0 -n 1 $CC >/tmp/teensy_$$.txt 2>&1
