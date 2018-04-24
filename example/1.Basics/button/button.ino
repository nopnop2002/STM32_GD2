#include <EEPROM.h>
#include <SPI.h>
#include <STM32_GD2.h>

void creatMenu(int pos, uint16_t options) {
  char title[10];
  for (byte i = 0; i<6; i++) {
    int x = (i % 3) * 160;
    int y = (i / 3) * 68;
    GD.Tag(i + 1);
    sprintf(title,"Button%d",i);
    if (i == pos) {
      GD.cmd_fgcolor(0x008000);
      GD.ColorRGB(0x000000);
    } else {
      GD.cmd_fgcolor(0x800000);
      GD.ColorRGB(0xffff00);
    }
    //GD.cmd_button(x, y, 150, 60, 27, OPT_FLAT, title);
    GD.cmd_button(x, y, 150, 60, 27, options, title);
  }
  GD.ColorRGB(0xF5B041);
  GD.cmd_text(GD.w / 2, GD.h - 40, 31, OPT_CENTER, "Push Button!!");

}

void setup()
{
  delay(1000);Serial.begin(9600);
  GD.begin(FORCE_CALIBRATE);  // Force Calibrate

  GD.Clear();
  creatMenu(-1, OPT_FLAT);
  GD.swap();
}

void loop()
{
  static int dtime = 0;
  GD.get_inputs();
  //Serial.println(GD.inputs.x);
  if (GD.inputs.tag != 0) {
    int pos = GD.inputs.tag;
    //Serial.println(pos);
    GD.Clear();
    creatMenu(pos-1, 0);
    GD.swap();
    dtime = 100;
  }
  if (dtime) dtime--;
  if (dtime == 0 && GD.inputs.x != -32768) {
    GD.Clear();
    creatMenu(-1, OPT_FLAT);
    GD.swap();
  }
}
