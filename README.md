# Arduino UNO fast alpha blending library and clock

By Riccardo Macri


I created this to see how far an Uno could be pushed short of
programming it in assembly.

The ARGB library supports 1, 2 or 3  8x8  RGB panels  (up to 576 channels)
with alpha blended animations, scrolling text overlay, fast display refresh,
and real-time clock accuracy if your Arduino has a crystal.

The clock uses the library and can work standalone with 2 buttons
to set/configure it and/or over USB with serial control and
ability to send text and graphics that will scroll blended with
the clock/animations.

The clock has a colour cycling time display with rolling digits,
a byte-dance display for the seconds, plus animation and
test pattern modes.

The graphic refresh and clock timing is all interrupt driven.
The clock can be very accurate if your Arduino has a crystal and
includes a calibration value you can set for your board that is
stored in EEPROM.

