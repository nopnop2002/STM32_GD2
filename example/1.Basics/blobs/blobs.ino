#include <EEPROM.h>
#include <SPI.h>
#include <STM32_GD2.h>

#define NBLOBS      128

xy blobs[NBLOBS];
const xy offscreen = {-16384, -16384};

void setup()
{
  delay(1000);Serial.begin(9600);
  //GD.begin(GD_CALIBRATE);  // On demand Calibrate
  GD.begin(FORCE_CALIBRATE);  // Force Calibrate

  for (int i = 0; i < NBLOBS; i++)
    blobs[i] = offscreen;
}

void loop()
{
  static byte blob_i;
  GD.get_inputs();
  if (GD.inputs.x != -32768)
    blobs[blob_i] = GD.inputs.xytouch;
  else
    blobs[blob_i] = offscreen;
  blob_i = (blob_i + 1) & (NBLOBS - 1);

  GD.ClearColorRGB(0xe0e0e0);
  GD.Clear();

  GD.ColorRGB(0xa0a0a0);
  GD.cmd_text(240, 136, 31, OPT_CENTER, "touch to draw");


  GD.Begin(POINTS);
  for (int i = 0; i < NBLOBS; i++) {
    // Blobs fade away and swell as they age
    GD.ColorA(i << 1);
    GD.PointSize((1024 + 16) - (i << 3));

    // Random color for each blob, keyed from (blob_i + i)
    uint8_t j = (blob_i + i) & (NBLOBS - 1);
    byte r = j * 17;
    byte g = j * 23;
    byte b = j * 147;
    GD.ColorRGB(r, g, b);

    // Draw it!
    blobs[j].draw();
  }
  GD.swap();
}
