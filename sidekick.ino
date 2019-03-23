/*
MIT License

Copyright (c) 2019 gdsports625@gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*
 * Receive keyboard and mouse commands on Serial1 then send
 * the USB HID reports out the USB device interface. This can be used with
 * a device that lacks USB hardware such as the ESP32.
 *
 * Tested on PJRC Teensy LC and Adafruit Trinket M0
 */

#ifdef ADAFRUIT_TRINKET_M0
// Serial2 on Trinket M0, D0=TX, D2=RX
#define RXpin 2
#define TXpin 0
#define RXpad SERCOM_RX_PAD_1
#define TXpad UART_TX_PAD_0

/* Serial2
 * Pin    Arduino SERCOM    SERCOM alt
 * PA08   D0      SERCOM0.0 SERCOM2.0
 * PA09   D2      SERCOM0.1 SERCOM2.0
 */

Uart Serial2( &sercom2, RXpin, TXpin, RXpad, TXpad);

void SERCOM2_Handler()
{
  Serial2.IrqHandler();
}
#endif

#define ATCMDPORT Serial2

#include <Keyboard.h>
#include <Mouse.h>

#ifdef ADAFRUIT_TRINKET_M0
// setup Dotstar LED on Trinket M0
#include <Adafruit_DotStar.h>
#define DATAPIN    7
#define CLOCKPIN   8
Adafruit_DotStar strip = Adafruit_DotStar(1, DATAPIN, CLOCKPIN, DOTSTAR_BRG);
#endif

void setup()
{
  // Turn off built-in RED LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
#ifdef ADAFRUIT_TRINKET_M0
  // Turn off built-in Dotstar RGB LED
  strip.begin();
  strip.clear();
  strip.show();
#endif

  Serial.begin(115200);
  Serial1.begin(115200);
  ATCMDPORT.begin(115200);
  Keyboard.begin();
  Mouse.begin();
}

typedef void (*command_t)(char *aLine);

typedef struct {
  const char *command;
  command_t action;
} command_action_t;

// force upper case
void toUpper(char *s) {
  while (*s) {
    if (islower(*s)) {
      *s = toupper(*s);
    }
    s++;
  }
}

void sendKeyboard(char *aLine) {
  if (aLine == NULL) return;
  //Serial.println(aLine);
  // aLine example "Now is the good time\r"
  // Convert characters such as '\r' to binary.
  char *pRead = aLine;
  char *pWrite = aLine;
  while (*pRead) {
    if (*pRead == '\\') {
      pRead++;
      switch (*pRead) {
        case 'r':
          *pWrite++ = '\r';
          break;
        case 'n':
          *pWrite++ = '\n';
          break;
        case 'b':
          *pWrite++ = '\b';
          break;
        case 't':
          *pWrite++ = '\t';
          break;
        case '\\':
          *pWrite++ = '\\';
          break;
        case '?':
          *pWrite++ = '?';
          break;
        default:
          *pWrite++ = '\\';
          *pWrite++ = *pRead;
          break;
      }
      pRead++;
    }
    else {
      *pWrite++ = *pRead++;
    }
  }
  *pWrite = '\0';

  Keyboard.print(aLine);
}

void sendHIDReport(char *aLine) {
  uint8_t keys[8];  // USB keyboard HID report

  if (aLine == NULL) return;
  memset(keys, 0, sizeof(keys));
  // aLine example: "00-00-04-00-00-00-00-00"
  char *p = strtok(aLine, "-");
  for (size_t i = 0; p && (i < sizeof(keys)); i++) {
    keys[i] = strtoul(p, NULL, 16);
    p = strtok(NULL, "-");
  }
  // Send keyboard hid report.
#ifdef TEENSYDUINO
  // This is Teensy specific so will not work on Arduino boards.
  Keyboard.set_modifier(keys[0]);
  Keyboard.set_key1(keys[2]);
  Keyboard.set_key2(keys[3]);
  Keyboard.set_key3(keys[4]);
  Keyboard.set_key4(keys[5]);
  Keyboard.set_key5(keys[6]);
  Keyboard.set_key6(keys[7]);
  Keyboard.send_now();
#else
  HID().SendReport(2, keys, sizeof(keys));
#endif
  ATCMDPORT.println("OK");
}

// TODO Teensy supports this and international keyboards
void sendConsumerKey(char *aLine)
{
  if (aLine == NULL) return;
  if (memcmp("0X", aLine, 2) != 0) {
    ATCMDPORT.println("ERROR");
    return;
  }
  Serial.println(strtoul(&aLine[2], NULL, 16), HEX);
  ATCMDPORT.println("ERROR");
}

/*
 * Examples of legal input
 * 10,-10
 * 0,0,5
 * ,,5
 * 1,2,3,4
 */
void sendMouseMove(char *aLine)
{
  int8_t mousemove[4];

  if (aLine == NULL) return;
  memset(mousemove, 0, sizeof(mousemove));
  char *p=aLine;
  for (size_t i = 0; p && (i < sizeof(mousemove)); i++) {
    mousemove[i] = strtol(p, &p, 10);
    if (*p != ',') break;
    p++;
  }
#ifdef TEENSYDUINO
  Mouse.move(mousemove[0], mousemove[1], mousemove[2], mousemove[3]);
#else
  Mouse.move(mousemove[0], mousemove[1], mousemove[2]);
#endif
  ATCMDPORT.println("OK");
}

/*
 * Examples of legal input
 * L    left button press
 * LR   left and right button press
 * 0    all buttons release
 */
void sendMouseButton(char *aLine)
{
  uint8_t buttons[5]; // left, middle, right, back, forward
  if (aLine == NULL) return;

  memset(buttons, 0, sizeof(buttons));
  char *p = aLine;
  char ch;
  while ((ch = *p++)) {
    switch (toupper(ch)) {
      case 'L':
        buttons[0] = 1;
        break;
      case 'M':
        buttons[1] = 1;
        break;
      case 'R':
        buttons[2] = 1;
        break;
      case 'B':
        buttons[3] = 1;
        break;
      case 'F':
        buttons[4] = 1;
        break;
      default:
        break;
    }
  }
#ifdef TEENSYDUINO
  Mouse.set_buttons(buttons[0], buttons[1], buttons[2], buttons[3], buttons[4]);
#else
  const uint8_t buttonMap[] = {1, 4, 2};
  for (size_t i = 0; i < 3; i++) {
    if (buttons[i]) {
      Mouse.press(buttonMap[i]);
    }
    else {
      Mouse.release(buttonMap[i]);
    }
  }
#endif
  ATCMDPORT.println("OK");
}

const command_action_t commands[] = {
  // Name of command user types, function that implements the command.
  // TODO add other commands, some day
  {"+USBKEYBOARD", sendKeyboard},
  {"+USBKEYBOARDCODE", sendHIDReport},
  {"+USBHIDCONTROLKEY", sendConsumerKey},
  {"+USBHIDMOUSEBUTTON", sendMouseButton},
  {"+USBHIDMOUSEMOVE", sendMouseMove},
};

void do_command(char *aLine) {
  if (aLine == NULL || *aLine == '\0') return;
  char *cmd = strtok(aLine, "=");
  if (cmd == NULL || *cmd == '\0') return;
  toUpper(cmd);
  if (memcmp(cmd, "AT", 2) != 0) return;
  if (strlen(cmd) == 2) {
    ATCMDPORT.println("OK");
    return;
  }
  for (size_t i = 0; i < sizeof(commands)/sizeof(commands[0]); i++) {
    if (strcmp(&cmd[2], commands[i].command) == 0) {
      commands[i].action(strtok(NULL, "="));
      return;
    }
  }
  ATCMDPORT.println("ERROR");
}

void cli_loop()
{
  static uint8_t bytesIn;
  static char aLine[80+1];

  while (ATCMDPORT.available() > 0) {
    int b = ATCMDPORT.read();
    if (b != -1) {
      switch (b) {
        case '\r':
          ATCMDPORT.println();
          aLine[bytesIn] = '\0';
          do_command(aLine);
          bytesIn = 0;
          break;
        case '\b':  // backspace
          if (bytesIn > 0) {
            bytesIn--;
            ATCMDPORT.print((char)b); ATCMDPORT.print(' '); ATCMDPORT.print((char)b);
          }
          break;
        default:
          ATCMDPORT.print((char)b);
          aLine[bytesIn++] = (char)b;
          if (bytesIn >= sizeof(aLine)-1) {
            aLine[bytesIn] = '\0';
            do_command(aLine);
            bytesIn = 0;
          }
          break;
      }
    }
  }
}

void loop()
{
  cli_loop();

  char buf[128];
  int bytesIn;

  // Bi-directional pass through Serial and Serial1
  if ((bytesIn = Serial.available()) > 0) {
    bytesIn = Serial.readBytes(buf, min(bytesIn, sizeof(buf)));
    if (bytesIn > 0) Serial1.write(buf, bytesIn);
  }
  if ((bytesIn = Serial1.available()) > 0) {
    bytesIn = Serial1.readBytes(buf, min(bytesIn, sizeof(buf)));
    if (bytesIn > 0) Serial.write(buf, bytesIn);
  }
}
