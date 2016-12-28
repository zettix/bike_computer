# bike_computer
Bike Computer Arduino Project.
==============================
Parts:
Nano
DS3231 Real Time Clock i2c board.
OLED i2c board: device 0x3C
button board.

Buttons are in D pad and fire button configuration, like an atari joystick.
The wire map is:

Open up "Examples:?:Button", change around to discover map.

   [6]
[5]   [3]   [7]
   [4]

Use i2c_scanner to detect screen...

Software:
---------
TIME/TEMP:
  Write library to populate char[] with time and temperature:
  TimeBoard
      void GetTime(char *buf) {

  i2c device 0x57
  i2c device 0x3C
  DS3231 RTC board.  Wants battery:
    24C32 addresses can be shorted A0/A1/A2 modify default address is 0x57
    rechargable cr2032.  $2.12 as of 10/29/2016

  EEPROM for save/load:
    not saving...

DISPLAY:
  3-5V 0.96" 12C Serial 128X64 OLED LCD LED Display Module for Arduino White SP
Description
1.Module Size :27.0MM*27.0MM*4.1MM
2. Resolution: 128 x 64
3. Visual Angle: >160Â°
4. Input Voltage: 3.3V ~ 6V
5. Compatible I/O Level: 3.3V, 5V
6.Viewing angle:>160
7. Only Need 2 I/O Port to Control
8. Drive IC :SSD1306
9.Full Compatible Arduino, 51 Series,MSP430 series,STM32/2,CSR IC,etc
  u8glib
    see sketches/oled_* for more info.

  i2c device 0x68
    128x64 board address 0x3C i2c OLED

What inverse: https://forum.arduino.cc/index.php?topic=333348.0
  maybe u8glib has it.
  color?

Buttons:
  
  D-pad shape and fire button.
  Use innertube and case like digital-8.
  find thin plastic.
  ***** Memory leak or something, button malfunctions.

Test:
  sketchbook/OLED_buttons_testrig
