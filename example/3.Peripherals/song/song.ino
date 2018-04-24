/*
 * If you want to hear Audio, You need External Audio Circuit.
 * The complete Audio Circuit is here.
 * 
 * Application Note AN_252 FT800 Audio Primer
 * https://www.soselectronic.cz/a_info/resource/c/FTDI/AN_252_FT800_Audio_Primer.pdf
 */

#include <EEPROM.h>
#include <SPI.h>
#include <STM32_GD2.h>

#define MUSICFILE   "mesmeriz.ima"

static Streamer stream;

void setup()
{
  delay(1000);Serial.begin(9600);
  GD.begin(GD_TRIM | GD_STORAGE);
  stream.begin(MUSICFILE);
}

void loop()
{
  static int r = 1;

  if (r == 1) {
    GD.cmd_gradient(0, 40, 0x282830,
                    0, GD.h, 0x606040);
    GD.cmd_text(GD.w/2, GD.h/3, 31, OPT_CENTER, MUSICFILE);
    uint16_t val, range;
    stream.progress(val, range);
//    GD.cmd_slider(30, 160, 420, 8, 0, val, range);
    int delta = (GD.w - 480) / 3;
    GD.cmd_slider((30+delta), GD.h/2, (420+delta), 8, 0, val, range);
    GD.swap();
    GD.finish();
    r = stream.feed();
    //Serial.println("r=" + String(r));
  }
}
