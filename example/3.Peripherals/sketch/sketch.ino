#include <EEPROM.h>
#include <SPI.h>
#include <STM32_GD2.h>

void setup()
{
  delay(1000);Serial.begin(9600);
  GD.begin();
  GD.cmd_memset(0, 0, long(GD.w) * GD.h);   // clear the bitmap
  GD.Clear();                           // draw the bitmap

  GD.ColorRGB(0x202030);
  GD.cmd_text(240, 136, 31, OPT_CENTER, "sketch demo");

  GD.BitmapLayout(L8, GD.w, GD.h);
  GD.BitmapSize(NEAREST, BORDER, BORDER, GD.w, GD.h);
  GD.Begin(BITMAPS);
  GD.ColorRGB(0xffffff);
  GD.Vertex2ii(0, 0);
  GD.swap();
  GD.cmd_sketch(0, 0, GD.w, GD.h, 0, L8); // start sketching
  GD.finish();                          // flush all commands
}

void loop() { }
