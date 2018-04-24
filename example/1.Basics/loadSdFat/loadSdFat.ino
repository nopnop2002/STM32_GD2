#include <EEPROM.h>
#include <SPI.h>
#include <SdFat.h> // https://github.com/greiman/SdFat
#include <STM32_GD2.h>

File file;
SdFat SD2(2);
const uint8_t SD2_CS = PB12;   // chip select for sd2

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
  //SD2.begin(SD2_CS, SD_SCK_MHZ(18));
  SD2.begin(SD2_CS);
  file = SD2.open("tree.jpg");  
  Serial.println("file=" + String(file));

  GD.begin(GD_NONE);  // SD2 class is NOT defined inside the library.
  if (!file) GD.alert("file not found");
  GD.Clear();
  draw_progress();
  GD.cmd_loadimage(0, 0);
  GD.loadSdFat(file, progress); 
}

void loop()
{
  GD.Clear();
  GD.Begin(BITMAPS);
  GD.Vertex2ii(0, 0);
  GD.swap();
}
