#include <EEPROM.h>
#include <SPI.h>
#include <STM32_GD2.h>

#include "tiled_assets.h"

void setup()
{
  delay(1000);Serial.begin(9600);
  GD.begin(GD_STORAGE);
  LOAD_ASSETS();
}

void loop()
{
  GD.Clear();
  GD.Begin(BITMAPS);
  GD.BitmapSize(BILINEAR, REPEAT, REPEAT, 480, 272);
  GD.cmd_rotate(3333);
  GD.cmd_setmatrix();
  GD.Vertex2ii(0, 0);
  GD.swap();
}
