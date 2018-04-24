#include <EEPROM.h>
#include <SPI.h>
#include <STM32_GD2.h>


void setup()
{
  GD.begin(GD_NONE);
}

void loop()
{
  GD.Clear();
  GD.Begin(POINTS);
  for (int i = 0; i < 100; i++) {
    GD.PointSize(GD.random(2 * GD.w));
    GD.ColorRGB(GD.random(), GD.random(), GD.random());
    GD.ColorA(GD.random());
    GD.Vertex2f(GD.random(PIXELS(GD.w)), GD.random(PIXELS(GD.h)));
  }
  GD.swap();
}
