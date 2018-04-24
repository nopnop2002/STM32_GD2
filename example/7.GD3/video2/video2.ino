/*
 * FT800 doesn't have the function which plays the video.
 */
#include <EEPROM.h>
#include <SPI.h>
#include <STM32_GD2.h>

char avis[][ 13 ] =
{
  "70s_tv09.avi",
  "70s_tv11.avi",
  "70s_tv18.avi",
  "car-1500.avi",
  "demo2.avi",
  "dj-1500.avi",
  "fish.avi",
  "fun-1500.avi",
  "spa-1500.avi",
  "tra-1500.avi",
  "tub-1500.avi"
};

char *pickfile(char *ext)
{
  GD.Clear();
  for (byte i = 0; i < 11; i++) {
    int x = (i % 3) * 160;
    int y = (i / 3) * 68;
    GD.Tag(i + 1);
    GD.cmd_button(x, y, 150, 60, 27, OPT_FLAT, avis[i]);
  }
  GD.swap();

  do {
    GD.get_inputs();
  } while (!GD.inputs.tag);

  Serial.println("GD.inputs.tag=" + String(GD.inputs.tag));
  return avis[GD.inputs.tag - 1];
}

void setup()
{
  delay(1000);Serial.begin(9600);
  GD.begin(FORCE_CALIBRATE + GD_STORAGE);
}

void loop()
{
  char *avi = pickfile("avi");
  Serial.println(avi);
  GD.Clear();
  GD.cmd_text(GD.w / 2, GD.h / 2 - 30, 31, OPT_CENTER, "playing");
  GD.cmd_text(GD.w / 2, GD.h / 2 + 30, 31, OPT_CENTER, avi);
  GD.swap();

  MoviePlayer mp;
  mp.begin(avi);
  mp.play();
}
