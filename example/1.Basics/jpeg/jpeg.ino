#include <EEPROM.h>
#include <SPI.h>
#include <STM32_GD2.h>

#define PROGRESS_Y  136

static void progress(long a, long b)
{
  GD.wr32(REG_MACRO_0, VERTEX2II(40 + 399 * a / b, PROGRESS_Y, 0, 0));
}

static void draw_progress()
{
  GD.Begin(LINES);

  GD.ColorRGB(0xc0c0c0);
  GD.LineWidth(16 * 5);
  GD.Vertex2ii(40, PROGRESS_Y);
  GD.Vertex2ii(440, PROGRESS_Y);

  GD.ColorRGB(0x000000);
  GD.LineWidth(16 * 3);
  GD.Vertex2ii(40, PROGRESS_Y);
  GD.Vertex2ii(440, PROGRESS_Y);

  GD.ColorRGB(0xffffff);
  GD.LineWidth(16 * 2);
  GD.Vertex2ii(40, PROGRESS_Y);
  GD.Macro(0);
  GD.swap();
}

void setup()
{
  delay(1000);Serial.begin(9600);
  GD.begin(GD_STORAGE);  // SD2 class is defined inside the library.
  GD.Clear();
  draw_progress();
  GD.cmd_loadimage(0, 0);
  if (!GD.load("tree.jpg", progress)) GD.alert("file not found");
}

void loop()
{
  GD.Clear();
  GD.Begin(BITMAPS);
  GD.Vertex2ii(0, 0);
  GD.swap();
}
