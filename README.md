# USB Sidekick

The USB Sidekick (USk) provides USB HID keyboard and mouse services to another
device that lacks USB hardware. The USk accepts AT commands and translates
them into USB keyboard and mouse events.

## USB Sidekick hardware

The hardware is an Adafruit Trinket M0 which is convenient for use with
ESP8266 and ESP32 boards because they use 3.3V logic levels.

Trinket M0 USB Sidekick|ESP8266
-----------------------|---------
GND         |GND
0/Serial2-TX|n/c
2/Serial2-RX|GPIO#2/Serial1-TX

The USk accepts AT commands at 115,200 bits/sec on its Serial2-RX pin. It
sends responses ("OK" or "ERROR") on Serial2-TX. The ESP8266 second UART has
TX but no RX so it cannot read the response. Sending with a short delay
between AT commands should work.

Bare mininum code for an ESP8266 running MicroPython.

```
from machine import UART
# GPIO#2 UART TX
uart = UART(1, 115200)
uart.write('at+usbkeyboard=https://micropython.org\\n\r')
```

The two back slashes is not a typo. More below.

USk is also a USB serial pass through although it is not complete. If using an
ESP8266 without a USB Serial adapter, connect the Trinket M0 running USk like
this. The pass through is good enough for the REPL but esptool does not work.
Maybe some day.

Trinket M0 USB Sidekick|ESP8266
-----------------------|---------
GND         |GND
0/Serial2-TX|n/c
2/Serial2-RX|GPIO#2/Serial1-TX
4/Serial1-TX|RX/Serial-RX
3/Serial1-RX|TX/Serial-TX

On the Trinket M0, Serial is the USB native port connected to a computer.
Serial1 is UART connected to the primary ESP8266 UART (REPL). Serial2 is
connected to the ESP826 Serial1-TX to receive AT+USB commands from the ESP.

## USB Sidekick firmware

The USk firmware is written using the Arduino framework so it leverages
the Arduino USB keyboard and mouse libraries.

The AT+USB commands are based on the Adafruit Bluefruit BLE services.

https://learn.adafruit.com/introducing-adafruit-ble-bluetooth-low-energy-friend/ble-services

Note: The source code include some support for the PJRC Teensy LC board which
has an expanded USB keyboard library (for example, media keys and non-USB
keyboards). However, it is not complete and has not been tested.

### AT+USBKEYBOARD

Sends text data over the USB keyboard interface. Use the +USBKEYBOARDCODE
command to send non-text data such as F1 or CTRL-ALT-DEL.

Any valid alpha-numeric character can be sent, and the following escape
sequences are also supported:


Esc| Description
---|------------------
\r | Carriage Return
\n | Line Feed
\b | Backspace
\t | Tab
\\\\ | Backslash

The following command sends "https://micropython.org" followed by USB keyboard
ENTER. The '\n' at the end is part of the AT command.

```
AT+USBKEYBOARD=https://micropython.org\n
```

In Micropython/CircuitPython, the newline '\n' must be escaped to send it as 
a USB key press.

```
uart.write('AT+USBKEYBOARD=https://micropython.org\\n\r')
```

The two back slashes in row is not a typo. This is done so the '\n' is passed
through as a USB keyboard press (same as pressing the ENTER key). The '\r'
terminates the AT command but is not sent as a USB keyboard press.

### AT+USBKEYBOARDCODE

Send USB HID report in HEX.

The following sends key 'a' press.

```
AT+USBKEYBOARDCODE=00-00-04-00-00-00-00-00
```

In Micropython/CircuitPython, send the command like this. Note trailing 00 may
be omitted.

```
uart.write('AT+USBKEYBOARDCODE=00-00-04\r')
```

See the following Wiki page for details on the HID report format and
USB key codes.

http://wiki.micropython.org/USB-HID-Keyboard-mode-example-a-password-dongle

See the following for similar information but from the point of view of AT
commands.

https://learn.adafruit.com/introducing-adafruit-ble-bluetooth-low-energy-friend/ble-services#at-plus-blekeyboardcode-14-25

### AT+USBKEYBOARDMACRO

Send USB keyboard and mouse macro.

TBD, maybe

### AT+USBHIDCONTROLKEY

Send USB HID consumer keys.

TBD, maybe

### AT+USBHIDMOUSEMOVE

Moves the HID mouse or scroll wheel position the specified number of ticks.

All parameters are signed 8-bit values (-128 to +127).  Positive values move
to the right or down, and origin is the top left corner.

Parameters: X Ticks (+/-), Y Ticks (+/-), Scroll Wheel (+/-), Pan Wheel (+/-)
Pan Wheel is ignored for now.

```
uart.write('at+usbhidmousemove=-100,100\r')
```

### AT+USBHIDMOUSEBUTTON

Manipulates the HID mouse buttons via the specific string(s).

Parameters: Button Mask String [L][R][M][B][F]

Code | Description
-----|-------------
L | Left Button
R | Right Button
M | Middle Button
B | Back Button
F | Forward Button

The first command presses the left mouse button. The next command releases
all mouse buttons.

```
uart.write('at+usbhidmousebutton=L\r')
uart.write('at+usbhidmousebutton=\r')
```

## Applications

* WiFi Pliable Python
* WiFi Malleable Mallard

## Compiled release for Trinket M0

See the [firmware](./firmware) directory for a compiled binary release.

Compiled programs can be burned into the Trinket M0 just by dragging and
dropping a UF2 file on to the Trinket M0 USB drive. There is no need to install
the Arduino IDE, source code, or USB serial device driver.

* Download the UF2 file of your choice.
* Plug in the Trinket M0 to the computer.
* Double tap the Trinket M0 reset button.
* When the TRINKETBOOT USB drive appears, drop the UF2 file on to the drive.
* Wait until the Trinket M0 reboots.

