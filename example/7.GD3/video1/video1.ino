/*
 * FT800 doesn't have the function which plays the video.
 */
#include <EEPROM.h>
#include <SPI.h>
#include <STM32_GD2.h>

void setup()
{
  delay(1000);Serial.begin(9600);
  GD.begin(GD_STORAGE);
}

void loop()
{
  MoviePlayer mp;

  int randNumber = random(300);
  int fileNumber = randNumber % 5;
  if (fileNumber == 0) mp.begin("fun-1500.avi");
  if (fileNumber == 1) mp.begin("dj-1500.avi");
  if (fileNumber == 2) mp.begin("spa-1500.avi");
  if (fileNumber == 3) mp.begin("tra-1500.avi");
  if (fileNumber == 4) mp.begin("tub-1500.avi");
  mp.play();

//  mp.begin("spa-1500.avi");
//  mp.play();

}
