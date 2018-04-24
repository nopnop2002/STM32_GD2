/*
 * FT800 doesn't have the function which revolves around a screen.
 */
#include <EEPROM.h>
#include <SPI.h>
#include <STM32_GD2.h>

void setup()
{
  delay(1000);Serial.begin(9600);
  GD.begin(0);
}

void loop()
{
  static int rotate = 0;
  GD.ClearColorRGB(0x103000);
  GD.Clear();
  Serial.println(GD.model);
  if (GD.model != 800) GD.cmd_setrotate(rotate);
  GD.cmd_text(GD.w / 2, GD.h / 2, 31, OPT_CENTER, "Hello world");
  GD.swap();
  if (GD.model != 800) {
    rotate++;
    if (rotate == 4) rotate = 0;
  }
  delay(5000);

}
