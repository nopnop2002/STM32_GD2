#include <EEPROM.h>
#include <SPI.h>
#include <STM32_GD2.h>

#define RAM_STM32A 0

static const PROGMEM prog_uchar STM32a[] = {
  #include "STM32a.h"
};

int minX = 135, minY = 15;
//int maxX = 480 - minX, maxY = 272 - minY;  //FT80X
//int maxX = 800 - minX, maxY = 480 - minY;  //FT81X
int maxX, maxY;

int startX = maxX / 2, startY = maxY / 2;
int deltaX = 2, deltaY = 2;

void EntornoMP()
{
  GD.cmd_inflate(RAM_STM32A);
  GD.copy(STM32a, sizeof(STM32a));
  GD.BitmapHandle(0);
  GD.BitmapSource(0);
  GD.BitmapLayout(ARGB1555, 960, 175);
  GD.BitmapSize(NEAREST, BORDER, BORDER, 480, 175);
}


void setup() {
  GD.begin(GD_NONE);
  maxX = GD.w - minX;
  maxY = GD.h - minY;
  EntornoMP();
}

void loop()
{
 GD.ClearColorRGB(0, 0, 0);
 GD.Clear();

 GD.SaveContext();
 GD.Begin(BITMAPS);
 GD.Vertex2ii(160, 152, 0, 0);
 GD.RestoreContext();
 
 GD.ColorRGB(0x00,0xff,0x00);
 GD.cmd_text(startX, startY, 31, OPT_CENTER, "Hello World");
 
 startX = startX + deltaX;   startY = startY + deltaY;
   if (startX >= maxX)
       deltaX = -deltaX;
   if (startX <= minX)
       deltaX = abs(deltaX);
   if (startY >= maxY)
       deltaY = -deltaY;
   if (startY <= minY)
       deltaY = abs(deltaY);
 GD.swap();
}
