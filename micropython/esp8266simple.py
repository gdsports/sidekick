from machine import UART
uart = UART(1, 115200)
uart.write('at+usbkeyboard=https://micropython.org\\n\r')
uart.write('at+usbhidmousemove=-100,100\r');
