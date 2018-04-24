#include <EEPROM.h>
#include <SPI.h>
#include <STM32_GD2.h>

void setup() {
  GD.begin(GD_NONE);

  GD.ClearColorRGB(0xff0000);
  GD.Clear();

  // Draw Frame
  GD.ColorRGB(0x00ff00);
  GD.LineWidth(1 * 16);
  GD.Begin(RECTS);
  GD.Vertex2f(0 * 16, 0* 16);
  GD.Vertex2f((GD.w-1) * 16, (GD.h-1) * 16);

  // Draw Inner
  GD.ColorRGB(0x000000);
  GD.Vertex2f(1 * 16, 1* 16);
  GD.Vertex2f((GD.w-2) * 16, (GD.h-2) * 16);

  char TXP[50];
  GD.ColorRGB(0x00ff00);
  sprintf(TXP,"F_CPU: %d MHz", (F_CPU/1000000));  GD.cmd_text(GD.w / 2, GD.h / 5, 31, OPT_CENTER, TXP);
  sprintf(TXP,"Model:  FT%d ", GD.model);  GD.cmd_text(GD.w / 2, (GD.h / 5) * 2, 31, OPT_CENTER, TXP);
  sprintf(TXP,"Width:  %d Pixel", GD.w);  GD.cmd_text(GD.w / 2, (GD.h / 5) * 3, 31, OPT_CENTER, TXP);
  sprintf(TXP,"Height:  %d Pixel", GD.h);  GD.cmd_text(GD.w / 2, (GD.h / 5) * 4, 31, OPT_CENTER, TXP);
  GD.swap();

}

void loop() {
}
