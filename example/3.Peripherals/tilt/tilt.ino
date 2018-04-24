/*
 * Gameduino 2 has a 3-axis accelerometer sensor connected to the Arduino's analog inputs A0, A1 and A2.
 * FT800/FT81x don't have 3-axis accelerometer sensor.
 * This sketch doesn't work in FT800CB/FT810CB
 */

#include <EEPROM.h>
#include <SPI.h>
#include <STM32_GD2.h>

void setup()
{
  GD.begin(FORCE_CALIBRATE);
}

void loop()
{
  GD.get_inputs();
  int x, y, z;
  GD.get_accel(x, y, z);
  GD.Clear();
  GD.LineWidth(16 * 3);
  int xp = 240 + x;
  int yp = 136 + y;
  GD.Begin(LINES);
  GD.Vertex2f(16 * 240, 16 * 136);
  GD.Vertex2f(16 * xp, 16 * yp);

  GD.PointSize(16 * 40);
  GD.Begin(POINTS);
  GD.Vertex2f(16 * xp, 16 * yp);

  GD.swap();
}
