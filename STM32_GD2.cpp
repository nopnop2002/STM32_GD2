/*
 * FT800/81X TFT Library for STM32F103
 * based on GD23STM32_F103x by lightcalamar
 * https://github.com/lightcalamar/GD23STM32_F103x
 *
 * GD23STM32_F103x has been developed based on Gameduino 2 library.
 * Copyright (C) 2013-2016 by James Bowman <jamesb@excamera.com>
 * Gameduino 2 library for Arduino, Arduino Due, Raspberry Pi.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <Arduino.h>
#include <EEPROM.h>
#include <SPI.h>
#include <SdFat.h> // https://github.com/greiman/SdFat
#include <STM32_GD2.h>
#include "transports/wiring.h"


SdFat SD(2);

byte ft8xx_model; // 0:FT800/1:FT81x
uint16_t FT800_model; // 0x00:FT800/0x10:FT810/0x11:FT811/0x12:FT812/0x13:FT813


////////////////////////////////////////////////////////////////////////

void xy::set(int _x, int _y)
{
  x = _x;
  y = _y;
}

void xy::rmove(int distance, int angle)
{
  x -= GD.rsin(distance, angle);
  y += GD.rcos(distance, angle);
}

int xy::angleto(class xy &other)
{
  int dx = other.x - x, dy = other.y - y;
  return GD.atan2(dy, dx);
}

void xy::draw(byte offset)
{
  GD.Vertex2f(x - PIXELS(offset), y - PIXELS(offset));
}

int xy::onscreen(void)
{
  return (0 <= x) &&
         (x < PIXELS(GD.w)) &&
         (0 <= y) &&
         (y < PIXELS(GD.h));
}

class xy xy::operator+=(class xy &other)
{
  x += other.x;
  y += other.y;
  return *this;
}

int xy::nearer_than(int distance, xy &other)
{
  int lx = abs(x - other.x);
  if (lx > distance)
    return 0;
  int ly = abs(y - other.y);
  if (ly > distance)
    return 0;

  // trivial accept: 5/8 is smaller than 1/sqrt(2)
  int d2 = (5 * distance) >> 3;
  if ((lx < d2) && (ly < d2))
    return 1;

  return ((lx * lx) + (ly * ly)) < (distance * distance);
}


////////////////////////////////////////////////////////////////////////

static GDTransport GDTR;

GDClass GD;

////////////////////////////////////////////////////////////////////////

// The GD3 has a tiny configuration EEPROM - AT24C01D
// It is programmed at manufacturing time with the setup
// commands for the connected panel. The SCL,SDA lines
// are connected to thye FT81x GPIO0, GPIO1 signals.
// This is a read-only driver for it.  A single method
// 'read()' initializes the RAM and reads all 128 bytes
// into an array.

class ConfigRam {
private:
  uint8_t gpio, gpio_dir, sda;
  void set_SDA(byte n)
  {
    if (sda != n) {
      GDTR.__wr16(REG_GPIO_DIR, gpio_dir | (0x03 - n));    // Drive SCL, SDA low
      sda = n;
    }
  }

  void set_SCL(byte n)
  {
    GDTR.__wr16(REG_GPIO, gpio | (n << 1));
  }

  int get_SDA(void)
  {
    return GDTR.__rd16(REG_GPIO) & 1;
  }

  void i2c_start(void) 
  {
    set_SDA(1);
    set_SCL(1);
    set_SDA(0);
    set_SCL(0);
  }

  void i2c_stop(void) 
  {
    set_SDA(0);
    set_SCL(1);
    set_SDA(1);
    set_SCL(1);
  }

  int i2c_rx1()
  {
    set_SDA(1);
    set_SCL(1);
    byte r = get_SDA();
    set_SCL(0);
    return r;
  }

  void i2c_tx1(byte b)
  {
    set_SDA(b);
    set_SCL(1);
    set_SCL(0);
  }

  int i2c_tx(byte x)
  {
    for (byte i = 0; i < 8; i++, x <<= 1)
      i2c_tx1(x >> 7);
    return i2c_rx1();
  }

  int i2c_rx(int nak)
  {
    byte r = 0;
    for (byte i = 0; i < 8; i++)
      r = (r << 1) | i2c_rx1();
    i2c_tx1(nak);
    return r;
  }

  


public:
  void read(byte *v)
  {
    GDTR.__end();
    gpio = GDTR.__rd16(REG_GPIO) & ~3;
    gpio_dir = GDTR.__rd16(REG_GPIO_DIR) & ~3;
    sda = 2;

    // 2-wire software reset
    i2c_start();
    i2c_rx(1);
    i2c_start();
    i2c_stop();

    int ADDR = 0xa0;

    i2c_start();
    if (i2c_tx(ADDR))
      return;
    if (i2c_tx(0))
      return;

    i2c_start();
    if (i2c_tx(ADDR | 1))
      return;
    for (int i = 0; i < 128; i++) {
      *v++ = i2c_rx(i == 127);
      // Serial.println(v[-1], DEC);
    }
    i2c_stop();
    GDTR.resume();
  }
};

//RndMnkIII
const uint16_t GDClass::TAM_BUFFER_SD=8192;
const uint16_t GDClass::TAM_BUFFER_FT=2048;
byte GDClass::buf[TAM_BUFFER_SD];
byte GDClass::FTbuf[TAM_BUFFER_FT];

void GDClass::flush(void)
{
  GDTR.flush();
}

void GDClass::swap(void) {
  Display();
  cmd_swap();
  cmd_loadidentity();
  cmd_dlstart();
  GDTR.flush();
#ifdef DUMPDEV
  GDTR.swap();
#endif
}

uint32_t GDClass::measure_freq(void)
{
  unsigned long t0 = GDTR.rd32(REG_CLOCK);
  delayMicroseconds(15625);
  unsigned long t1 = GDTR.rd32(REG_CLOCK);
  // Serial.println((t1 - t0) << 6);
  return (t1 - t0) << 6;
}



#if 0
#if (TFT_FT81X_ENABLE == 0)
  #define LOW_FREQ_BOUND  47040000UL
#endif

#if (TFT_FT81X_ENABLE == 1)
  #define LOW_FREQ_BOUND  58800000UL
#endif
// #define LOW_FREQ_BOUND  32040000UL
#endif

void GDClass::tune(void)
{

  uint32_t LOW_FREQ_BOUND = 32040000;
  if (FT800_model == 0)
    LOW_FREQ_BOUND = 47040000;

  uint32_t f;
  for (byte i = 0; (i < 31) && ((f = measure_freq()) < LOW_FREQ_BOUND); i++) {
    GDTR.wr(REG_TRIM, i);
  }
  GDTR.wr32(REG_FREQUENCY, f);
}

void GDClass::begin(uint8_t options) {
//Serial.print("begin start options=");
//Serial.println(options,HEX);
  GDTR.begin0();
  //GDTR.ios(); // ios called from begin0
  //if (STORAGE && (options & GD_STORAGE)) {
  if (options & GD_STORAGE) {
//Serial.println("setup STORAGE");
    pinMode(SD_PIN, OUTPUT);
    digitalWrite(SD_PIN, HIGH);
    //SD.begin(SD_PIN);
    SD.begin(SD_PIN, SD_SCK_MHZ(18));
  }

  FT800_model = GDTR.begin1();
//Serial.print("FT800_model=");
//Serial.println(FT800_model,HEX);

//Serial.print("ID REGISTER:");
//Serial.println(GDTR.rd(REG_ID), HEX);

  GDTR.wr(REG_PWM_DUTY, 0);
  GDTR.wr(REG_GPIO_DIR, 0x83);
  GDTR.wr(REG_GPIO, GDTR.rd(REG_GPIO) | 0x80);

  ConfigRam cr;
  byte v8[128] = {0};
  cr.read(v8);

#if 0
  if ((v8[1] == 0xff) && (v8[2] == 0x01)) {
    options &= ~(GD_TRIM | GD_CALIBRATE);
    if (v8[3] & 2) {
      GDTR.__end();
      GDTR.hostcmd(0x44); // switch to external crystal
      GDTR.resume();
    }
    copyram(v8 + 4, 124);
    finish();
  } else {
#endif

// ******************************************************************************************

  if (FT800_model == 00) // FT800 Series
  {
	   GDTR.wr(REG_PCLK_POL, 1);
	   GDTR.wr(REG_PCLK, 5);
	    // Avoid inverted colours on alternative-FT800 boards
	   #if PROTO == 1
	      GDTR.wr(REG_SWIZZLE, 3);
	   #endif
	   GDTR.wr(REG_ROTATE, ROTACION);
  }


// ******************************************************************************************

  if(FT800_model == 0x10) // FT810
  {
	GD.wr32(REG_HCYCLE, 900);//548
	GD.wr32(REG_HOFFSET, 43);
	GD.wr32(REG_HSIZE, 800);
	GD.wr32(REG_HSYNC0, 0);
	GD.wr32(REG_HSYNC1, 41);
	GD.wr32(REG_VCYCLE, 500);
	GD.wr32(REG_VOFFSET, 12);
	GD.wr32(REG_VSIZE, 480);
	GD.wr32(REG_VSYNC0, 0);
	GD.wr32(REG_VSYNC1, 10);
	GD.wr32(REG_DITHER, 1);
	GD.wr32(REG_PCLK_POL, 1);//1
	GD.wr32(REG_PCLK, 3);//5
	GD.wr(REG_ROTATE, 0);
	GD.wr(REG_SWIZZLE, 0);//3 for GD2
  }

// ******************************************************************************************

  if(FT800_model == 0x11) // FT811
  {
	GD.wr32(REG_HCYCLE, 1000);//548
	GD.wr32(REG_HOFFSET, 43);
	GD.wr32(REG_HSIZE, 800);
	GD.wr32(REG_HSYNC0, 0);
	GD.wr32(REG_HSYNC1, 41);
	GD.wr32(REG_VCYCLE, 500);
	GD.wr32(REG_VOFFSET, 12);
	GD.wr32(REG_VSIZE, 480);
	GD.wr32(REG_VSYNC0, 0);
	GD.wr32(REG_VSYNC1, 10);
	GD.wr32(REG_DITHER, 1);
	GD.wr32(REG_PCLK_POL, 1);//1
	GD.wr32(REG_PCLK, 3);//5
	GD.wr(REG_ROTATE, 0);
	GD.wr(REG_SWIZZLE, 0);//3 for gameduino 2
  }


// ******************************************************************************************

  if(FT800_model == 0x13) // FT813
  {
	GD.wr32(REG_HSIZE, 800);
	GD.wr32(REG_VSIZE, 480);
	GD.wr32(REG_HCYCLE, 1056);//1000
	GD.wr32(REG_HOFFSET, 46);
	GD.wr32(REG_HSYNC0, 0);
	GD.wr32(REG_HSYNC1, 10); //41
	GD.wr32(REG_VCYCLE, 525);
	GD.wr32(REG_VOFFSET, 23);
	GD.wr32(REG_VSYNC0, 0);
	GD.wr32(REG_VSYNC1, 10);
	GD.wr32(REG_PCLK, 2);//5
	GD.wr32(REG_SWIZZLE, 0);//3 for GD2
	GD.wr32(REG_PCLK_POL, 1);//1
	GD.wr32(REG_CSPREAD, 0);
	GD.wr32(REG_DITHER, 0);
	//GD.wr(REG_ROTATE, 0);

	//GD.wr32(REG_HSIZE, 800);
	//GD.wr32(REG_VSIZE, 480);
	//GD.wr32(REG_HCYCLE, 1000);//548
	//GD.wr32(REG_HOFFSET, 46);
	//GD.wr32(REG_HSYNC0, 0);
	//GD.wr32(REG_HSYNC1, 41);
	//GD.wr32(REG_VCYCLE, 525);
	//GD.wr32(REG_VOFFSET, 23);
	//GD.wr32(REG_VSYNC0, 0);
	//GD.wr32(REG_VSYNC1, 10);
	//GD.wr32(REG_PCLK, 2);//5

	//GD.wr32(REG_DITHER, 0);
	//GD.wr32(REG_PCLK_POL, 1);//1
	//GD.wr(REG_ROTATE, 0);
	//GD.wr(REG_SWIZZLE, 0);//3 for GD2
  }

// ******************************************************************************************

  if(FT800_model == 0x14) // FT814
  {
    GD.wr32(REG_HCYCLE, 1056);    // 900 //548
    GD.wr32(REG_HOFFSET, 88);     // 46
    GD.wr32(REG_HSIZE, 800);
    GD.wr32(REG_HSYNC0, 0);
    GD.wr32(REG_HSYNC1, 10);      // 41
    GD.wr32(REG_VCYCLE, 525);     // 500
    GD.wr32(REG_VOFFSET, 32);     // 23
    GD.wr32(REG_VSIZE, 480);
    GD.wr32(REG_VSYNC0, 0);
    GD.wr32(REG_VSYNC1, 10);

    GD.wr32(REG_PCLK, 2);
    GD.wr32(REG_PCLK_POL, 0);
    GD.wr32(REG_CSPREAD, 0);       // 1
    GD.wr32(REG_DITHER, 1);        // 1


	//GD.wr32(REG_HSIZE, 800);
	//GD.wr32(REG_HCYCLE, 928);//548
	//GD.wr32(REG_HOFFSET, 88);
	//GD.wr32(REG_HSYNC0, 0);
	//GD.wr32(REG_HSYNC1, 48);

	//GD.wr32(REG_VSIZE, 480);
	//GD.wr32(REG_VCYCLE, 525);
	//GD.wr32(REG_VOFFSET, 32);
	//GD.wr32(REG_VSYNC0, 0);
	//GD.wr32(REG_VSYNC1, 3);

	//GD.wr32(REG_PCLK, 2);//5
	//GD.wr32(REG_PCLK_POL, 0);//1
	//GD.wr32(REG_CSPREAD, 0);//1
	//GD.wr32(REG_DITHER, 1);
  }

// ******************************************************************************************

  if(FT800_model == 0x15) // FT815
  {
	GD.wr32(REG_HSIZE, 480);
	GD.wr32(REG_HCYCLE, 548);
	GD.wr32(REG_HOFFSET, 43);
	GD.wr32(REG_HSYNC0, 0);
	GD.wr32(REG_HSYNC1, 41);

	GD.wr32(REG_VSIZE, 272);
	GD.wr32(REG_VCYCLE, 292);
	GD.wr32(REG_VOFFSET, 12);
	GD.wr32(REG_VSYNC0, 0);
	GD.wr32(REG_VSYNC1, 10);

	GD.wr32(REG_PCLK, 5);//5
	GD.wr32(REG_SWIZZLE, 0);//3 for GD2
	GD.wr32(REG_PCLK_POL, 1);//1
	GD.wr32(REG_CSPREAD, 1);//1
	GD.wr32(REG_DITHER, 1);
	//GD.wr(REG_ROTATE, 0);
  }


// ******************************************************************************************

  if(FT800_model == 0x16) // FT816
  {
	GD.wr32(REG_HSIZE, 320);
	GD.wr32(REG_HCYCLE, 408);
	GD.wr32(REG_HOFFSET, 70);
	GD.wr32(REG_HSYNC0, 0);
	GD.wr32(REG_HSYNC1, 10);

	GD.wr32(REG_VSIZE, 240);
	GD.wr32(REG_VCYCLE, 263);
	GD.wr32(REG_VOFFSET, 13);
	GD.wr32(REG_VSYNC0, 0);
	GD.wr32(REG_VSYNC1, 2);

	GD.wr32(REG_PCLK, 6);//5
	GD.wr32(REG_SWIZZLE, 2);//3 for GD2
	GD.wr32(REG_PCLK_POL, 1);//1
	GD.wr32(REG_CSPREAD, 0);//1
	GD.wr32(REG_DITHER, 1);
	//GD.wr(REG_ROTATE, 0);
  }


  w = GDTR.rd16(REG_HSIZE);
  h = GDTR.rd16(REG_VSIZE);
  model = 800;
  if (FT800_model != 0) model = 800 + (FT800_model - 6);
  //loadptr = 0;

  // Work-around issue with bitmap sizes not being reset
  for (byte i = 0; i < 32; i++) {
    BitmapHandle(i);
    cI(0x28000000UL);
    cI(0x29000000UL);
  }

  Clear(); swap();
  Clear(); swap();
  Clear(); swap();

  if (FT800_model == 0x14)
  {
    cmd_regwrite(REG_PWM_DUTY, 102);
  }else{
    cmd_regwrite(REG_PWM_DUTY, 128);
  }

//FT81XMANIA TEAM (TFTLCDCyg)
  if (FT800_model != 0)
  {
    cmd_setrotate(ORIENTACION);
  }

  if (FT800_model == 0)
  {
    GDTR.wr(REG_ROTATE, ROTACION);
  }
//FT81XMANIA TEAM (TFTLCDCyg)

  GD.flush();

  int calibrate = 0;
  if (options & GD_CALIBRATE) calibrate = 1;
  if (options & FORCE_CALIBRATE) calibrate = 2;
//Serial.println("calibrate=" + String(calibrate));


//  if (CALIBRATION & (options & GD_CALIBRATE)){
//  if (CALIBRATION & (calibrate != 0)) {
  if (calibrate != 0) {
    uint16 Status;
    uint16_t Data;
    Status = EEPROM.read(0x10,&Data);
//Serial.print("Data.EEPROM[10]=");
//Serial.println(Data,HEX);
//Serial.println("Status=" + String(Status));
    if (Data != 0x7c || calibrate == 2) {
      self_calibrate();
      EEPROM.format();
      for (int i = 0; i < 24; i++) { 
		Data = GDTR.rd(REG_TOUCH_TRANSFORM_A + i);
//Serial.print("Data.rd[" + String(i) + "]=");
//Serial.println(Data, HEX);
        Status = EEPROM.write(0x20 + i, Data);
      }
      Status = EEPROM.write(0x10, 0x7c);  // is written!
//Serial.println("Status=" + String(Status));
    } else {
      for (int i = 0; i < 24; i++) {
		Status=EEPROM.read(0x20+i,&Data);
//Serial.print("Data.EEPROM[=" + String(i) + "]=");
//Serial.println(Data,HEX);
        GDTR.wr(REG_TOUCH_TRANSFORM_A + i, Data);
      }
    }

  }

  GDTR.wr16(REG_TOUCH_RZTHRESH, 1200);

  rseed = 0x77777777;

//  if ((BOARD == BOARD_GAMEDUINO23) && (options & GD_TRIM)) {
  if (options & GD_TRIM) {
    tune();
  }

//Serial.println("begin end");

}

void GDClass::storage(void) {
  GDTR.__end();
  //SD.begin(SD_PIN);
  SD.begin(SD_PIN,SD_SCK_MHZ(18));
  GDTR.resume();
}

void GDClass::self_calibrate(void) {
  cmd_dlstart();
  Clear();
  cmd_text(240, 100, 30, OPT_CENTERX, "please tap on the dot");
  cmd_calibrate();
  finish();
  cmd_loadidentity();
  cmd_dlstart();
  GDTR.flush();
}

void GDClass::seed(uint16_t n) {
  rseed = n ? n : 7;
}

uint16_t GDClass::random() {
  rseed ^= rseed << 2;
  rseed ^= rseed >> 5;
  rseed ^= rseed << 1;
  return rseed;
}

uint16_t GDClass::random(uint16_t n) {
  uint16_t p = random();
  if (n == (n & -n))
    return p & (n - 1);
  return (uint32_t(p) * n) >> 16;
}

// >>> [int(65535*math.sin(math.pi * 2 * i / 1024)) for i in range(257)]
static const PROGMEM uint16_t sintab[257] = {
0, 402, 804, 1206, 1608, 2010, 2412, 2813, 3215, 3617, 4018, 4419, 4821, 5221, 5622, 6023, 6423, 6823, 7223, 7622, 8022, 8421, 8819, 9218, 9615, 10013, 10410, 10807, 11203, 11599, 11995, 12390, 12785, 13179, 13573, 13966, 14358, 14750, 15142, 15533, 15923, 16313, 16702, 17091, 17479, 17866, 18252, 18638, 19023, 19408, 19791, 20174, 20557, 20938, 21319, 21699, 22078, 22456, 22833, 23210, 23585, 23960, 24334, 24707, 25079, 25450, 25820, 26189, 26557, 26924, 27290, 27655, 28019, 28382, 28744, 29105, 29465, 29823, 30181, 30537, 30892, 31247, 31599, 31951, 32302, 32651, 32999, 33346, 33691, 34035, 34378, 34720, 35061, 35400, 35737, 36074, 36409, 36742, 37075, 37406, 37735, 38063, 38390, 38715, 39039, 39361, 39682, 40001, 40319, 40635, 40950, 41263, 41574, 41885, 42193, 42500, 42805, 43109, 43411, 43711, 44010, 44307, 44603, 44896, 45189, 45479, 45768, 46055, 46340, 46623, 46905, 47185, 47463, 47739, 48014, 48287, 48558, 48827, 49094, 49360, 49623, 49885, 50145, 50403, 50659, 50913, 51165, 51415, 51664, 51910, 52155, 52397, 52638, 52876, 53113, 53347, 53580, 53810, 54039, 54265, 54490, 54712, 54933, 55151, 55367, 55581, 55793, 56003, 56211, 56416, 56620, 56821, 57021, 57218, 57413, 57606, 57796, 57985, 58171, 58355, 58537, 58717, 58894, 59069, 59242, 59413, 59582, 59748, 59912, 60074, 60234, 60391, 60546, 60699, 60849, 60997, 61143, 61287, 61428, 61567, 61704, 61838, 61970, 62100, 62227, 62352, 62474, 62595, 62713, 62828, 62941, 63052, 63161, 63267, 63370, 63472, 63570, 63667, 63761, 63853, 63942, 64029, 64114, 64196, 64275, 64353, 64427, 64500, 64570, 64637, 64702, 64765, 64825, 64883, 64938, 64991, 65042, 65090, 65135, 65178, 65219, 65257, 65293, 65326, 65357, 65385, 65411, 65435, 65456, 65474, 65490, 65504, 65515, 65523, 65530, 65533, 65535
};

int16_t GDClass::rsin(int16_t r, uint16_t th) {
  th >>= 6; // angle 0-1023
  // return int(r * sin((2 * M_PI) * th / 1024.));
  int th4 = th & 511;
  if (th4 & 256)
    th4 = 512 - th4; // 256->256 257->255, etc
  uint16_t s = pgm_read_word_near(sintab + th4);
  int16_t p = ((uint32_t)s * r) >> 16;
  if (th & 512)
    p = -p;
  return p;
}

int16_t GDClass::rcos(int16_t r, uint16_t th) {
  return rsin(r, th + 0x4000);
}

void GDClass::polar(int &x, int &y, int16_t r, uint16_t th) {
  x = (int)(-GD.rsin(r, th));
  y = (int)( GD.rcos(r, th));
}

// >>> [int(round(1024 * math.atan(i / 256.) / math.pi)) for i in range(256)]
static const PROGMEM uint8_t atan8[] = {
0,1,3,4,5,6,8,9,10,11,13,14,15,17,18,19,20,22,23,24,25,27,28,29,30,32,33,34,36,37,38,39,41,42,43,44,46,47,48,49,51,52,53,54,55,57,58,59,60,62,63,64,65,67,68,69,70,71,73,74,75,76,77,79,80,81,82,83,85,86,87,88,89,91,92,93,94,95,96,98,99,100,101,102,103,104,106,107,108,109,110,111,112,114,115,116,117,118,119,120,121,122,124,125,126,127,128,129,130,131,132,133,134,135,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,177,178,179,180,181,182,183,184,185,186,187,188,188,189,190,191,192,193,194,195,195,196,197,198,199,200,201,201,202,203,204,205,206,206,207,208,209,210,211,211,212,213,214,215,215,216,217,218,219,219,220,221,222,222,223,224,225,225,226,227,228,228,229,230,231,231,232,233,234,234,235,236,236,237,238,239,239,240,241,241,242,243,243,244,245,245,246,247,248,248,249,250,250,251,251,252,253,253,254,255,255
};

uint16_t GDClass::atan2(int16_t y, int16_t x)
{
  uint16_t a;
  uint16_t xx = 0;

  /* These values are tricky. So pretend they are not */
  if (x == -32768)
    x++;
  if (y == -32768)
    y++;

  if ((x <= 0) ^ (y > 0)) {
    int16_t t; t = x; x = y; y = t;
    xx ^= 0x4000;
  }
  if (x <= 0) {
    x = -x;
  } else {
    xx ^= 0x8000;
  }
  y = abs(y);
  if (x > y) {
    int16_t t; t = x; x = y; y = t;
    xx ^= 0x3fff;
  }
  while ((x | y) & 0xff80) {
    x >>= 1;
    y >>= 1;
  }
  if (y == 0) {
    a = 0;
  } else if (x == y) {
    a = 0x2000;
  } else {
    // assert(x <= y);
    int r = ((x << 8) / y);
    // assert(0 <= r);
    // assert(r < 256);
    a = pgm_read_byte(atan8 + r) << 5;
  }
  a ^= xx;
  return a;
}

void GDClass::align(byte n) {
  while ((n++) & 3)
    GDTR.cmdbyte(0);
}

void GDClass::cH(uint16_t v) {
  GDTR.cmdbyte(v & 0xff);
  GDTR.cmdbyte((v >> 8) & 0xff);
}

void GDClass::ch(int16_t v) {
  cH((uint16_t)v);
}

void GDClass::cI(uint32_t v) {
  GDTR.cmd32(v);
}

void GDClass::cFFFFFF(byte v) {
  union {
    uint32_t c;
    uint8_t b[4];
  };
  b[0] = v;
  b[1] = 0xff;
  b[2] = 0xff;
  b[3] = 0xff;
  GDTR.cmd32(c);
}

void GDClass::ci(int32_t v) {
  cI((uint32_t) v);
}

void GDClass::cs(const char *s) {
  int count = 0;
  while (*s) {
    char c = *s++;
    GDTR.cmdbyte(c);
    count++;
  }
  GDTR.cmdbyte(0);
  align(count + 1);
}

void GDClass::copy(const PROGMEM uint8_t *src, int count) {
  byte a = count & 3;
  while (count--) {
    GDTR.cmdbyte(pgm_read_byte_near(src));
    src++;
  }
  align(a);
}

void GDClass::copyram(byte *src, int count) {
  byte a = count & 3;
  GDTR.cmd_n(src, count);
  align(a);
}

void GDClass::AlphaFunc(byte func, byte ref) {
  cI((9UL << 24) | ((func & 7L) << 8) | ((ref & 255L) << 0));
}
void GDClass::Begin(byte prim) {
  cI((31UL << 24) | prim);
}
void GDClass::BitmapHandle(byte handle) {
  cI((5UL << 24) | handle);
}
void GDClass::BitmapLayout(byte format, uint16_t linestride, uint16_t height) {
  // cI((7UL << 24) | ((format & 31L) << 19) | ((linestride & 1023L) << 9) | ((height & 511L) << 0));
  union {
    uint32_t c;
    uint8_t b[4];
  };
  b[0] = height;
  b[1] = (1 & (height >> 8)) | (linestride << 1);
  b[2] = (7 & (linestride >> 7)) | (format << 3);
  b[3] = 7;
  cI(c);
}
void GDClass::BitmapSize(byte filter, byte wrapx, byte wrapy, uint16_t width, uint16_t height) {
  byte fxy = (filter << 2) | (wrapx << 1) | (wrapy);
  // cI((8UL << 24) | ((uint32_t)fxy << 18) | ((width & 511L) << 9) | ((height & 511L) << 0));
  union {
    uint32_t c;
    uint8_t b[4];
  };
  b[0] = height;
  b[1] = (1 & (height >> 8)) | (width << 1);
  b[2] = (3 & (width >> 7)) | (fxy << 2);
  b[3] = 8;
  cI(c);
  if (ft8xx_model) {
    b[0] = ((width >> 9) << 2) | (3 & (height >> 9));
    b[3] = 0x29;
    cI(c);
  }
}
void GDClass::BitmapSource(uint32_t addr) {
  cI((1UL << 24) | ((addr & 1048575L) << 0));
}
void GDClass::BitmapTransformA(int32_t a) {
  cI((21UL << 24) | ((a & 131071L) << 0));
}
void GDClass::BitmapTransformB(int32_t b) {
  cI((22UL << 24) | ((b & 131071L) << 0));
}
void GDClass::BitmapTransformC(int32_t c) {
  cI((23UL << 24) | ((c & 16777215L) << 0));
}
void GDClass::BitmapTransformD(int32_t d) {
  cI((24UL << 24) | ((d & 131071L) << 0));
}
void GDClass::BitmapTransformE(int32_t e) {
  cI((25UL << 24) | ((e & 131071L) << 0));
}
void GDClass::BitmapTransformF(int32_t f) {
  cI((26UL << 24) | ((f & 16777215L) << 0));
}
void GDClass::BlendFunc(byte src, byte dst) {
  cI((11UL << 24) | ((src & 7L) << 3) | ((dst & 7L) << 0));
}
void GDClass::Call(uint16_t dest) {
  cI((29UL << 24) | ((dest & 2047L) << 0));
}
void GDClass::Cell(byte cell) {
  cI((6UL << 24) | ((cell & 127L) << 0));
}
void GDClass::ClearColorA(byte alpha) {
  cI((15UL << 24) | ((alpha & 255L) << 0));
}
void GDClass::ClearColorRGB(byte red, byte green, byte blue) {
  cI((2UL << 24) | ((red & 255L) << 16) | ((green & 255L) << 8) | ((blue & 255L) << 0));
}
void GDClass::ClearColorRGB(uint32_t rgb) {
  cI((2UL << 24) | (rgb & 0xffffffL));
}
void GDClass::Clear(byte c, byte s, byte t) {
  byte m = (c << 2) | (s << 1) | t;
  cI((38UL << 24) | m);
}
void GDClass::Clear(void) {
  cI((38UL << 24) | 7);
}
void GDClass::ClearStencil(byte s) {
  cI((17UL << 24) | ((s & 255L) << 0));
}
void GDClass::ClearTag(byte s) {
  cI((18UL << 24) | ((s & 255L) << 0));
}
void GDClass::ColorA(byte alpha) {
  cI((16UL << 24) | ((alpha & 255L) << 0));
}
void GDClass::ColorMask(byte r, byte g, byte b, byte a) {
  cI((32UL << 24) | ((r & 1L) << 3) | ((g & 1L) << 2) | ((b & 1L) << 1) | ((a & 1L) << 0));
}
void GDClass::ColorRGB(byte red, byte green, byte blue) {
  // cI((4UL << 24) | ((red & 255L) << 16) | ((green & 255L) << 8) | ((blue & 255L) << 0));
  union {
    uint32_t c;
    uint8_t b[4];
  };
  b[0] = blue;
  b[1] = green;
  b[2] = red;
  b[3] = 4;
  cI(c);
}
void GDClass::ColorRGB(uint32_t rgb) {
  cI((4UL << 24) | (rgb & 0xffffffL));
}
void GDClass::Display(void) {
  cI((0UL << 24));
}
void GDClass::End(void) {
  cI((33UL << 24));
}
void GDClass::Jump(uint16_t dest) {
  cI((30UL << 24) | ((dest & 2047L) << 0));
}
void GDClass::LineWidth(uint16_t width) {
  cI((14UL << 24) | ((width & 4095L) << 0));
}
void GDClass::Macro(byte m) {
  cI((37UL << 24) | ((m & 1L) << 0));
}
void GDClass::PointSize(uint16_t size) {
  cI((13UL << 24) | ((size & 8191L) << 0));
}
void GDClass::RestoreContext(void) {
  cI((35UL << 24));
}
void GDClass::Return(void) {
  cI((36UL << 24));
}
void GDClass::SaveContext(void) {
  cI((34UL << 24));
}
void GDClass::ScissorSize(uint16_t width, uint16_t height) {
  if (ft8xx_model == 0)
    cI((28UL << 24) | ((width & 1023L) << 10) | ((height & 1023L) << 0));
  else
    cI((28UL << 24) | ((width & 4095L) << 12) | ((height & 4095L) << 0));
}
void GDClass::ScissorXY(uint16_t x, uint16_t y) {
  if (ft8xx_model == 0)
    cI((27UL << 24) | ((x & 511L) << 9) | ((y & 511L) << 0));
  else
    cI((27UL << 24) | ((x & 2047L) << 11) | ((y & 2047L) << 0));
}
void GDClass::StencilFunc(byte func, byte ref, byte mask) {
  cI((10UL << 24) | ((func & 7L) << 16) | ((ref & 255L) << 8) | ((mask & 255L) << 0));
}
void GDClass::StencilMask(byte mask) {
  cI((19UL << 24) | ((mask & 255L) << 0));
}
void GDClass::StencilOp(byte sfail, byte spass) {
  cI((12UL << 24) | ((sfail & 7L) << 3) | ((spass & 7L) << 0));
}
void GDClass::TagMask(byte mask) {
  cI((20UL << 24) | ((mask & 1L) << 0));
}
void GDClass::Tag(byte s) {
  cI((3UL << 24) | ((s & 255L) << 0));
}
void GDClass::Vertex2f(int16_t x, int16_t y) {
  // x = int(16 * x);
  // y = int(16 * y);
  cI((1UL << 30) | ((x & 32767L) << 15) | ((y & 32767L) << 0));
}
void GDClass::Vertex2ii(uint16_t x, uint16_t y, byte handle, byte cell) {
  // cI((2UL << 30) | ((x & 511L) << 21) | ((y & 511L) << 12) | ((handle & 31L) << 7) | ((cell & 127L) << 0));
  union {
    uint32_t c;
    uint8_t b[4];
  };
  b[0] = cell | ((handle & 1) << 7);
  b[1] = (handle >> 1) | (y << 4);
  b[2] = (y >> 4) | (x << 5);
  b[3] = (2 << 6) | (x >> 3);
  cI(c);
}
void GDClass::VertexFormat(byte frac) {
  cI((39UL << 24) | (((frac) & 7) << 0));
}
void GDClass::BitmapLayoutH(byte linestride, byte height) {
  cI((40 << 24) | (((linestride) & 3) << 2) | (((height) & 3) << 0));
}
void GDClass::BitmapSizeH(byte width, byte height) {
  cI((41UL << 24) | (((width) & 3) << 2) | (((height) & 3) << 0));
}
void GDClass::PaletteSource(uint32_t addr) {
  cI((42UL << 24) | (((addr) & 4194303UL) << 0));
}
void GDClass::VertexTranslateX(uint32_t x) {
  cI((43UL << 24) | (((x) & 131071UL) << 0));
}
void GDClass::VertexTranslateY(uint32_t y) {
  cI((44UL << 24) | (((y) & 131071UL) << 0));
}
void GDClass::Nop(void) {
  cI((45UL << 24));
}

void GDClass::cmd_append(uint32_t ptr, uint32_t num) {
  cFFFFFF(0x1e);
  cI(ptr);
  cI(num);
}
void GDClass::cmd_bgcolor(uint32_t c) {
  cFFFFFF(0x09);
  cI(c);
}
void GDClass::cmd_button(int16_t x, int16_t y, uint16_t w, uint16_t h, byte font, uint16_t options, const char *s) {
  cFFFFFF(0x0d);
  ch(x);
  ch(y);
  ch(w);
  ch(h);
  ch(font);
  cH(options);
  cs(s);
}
void GDClass::cmd_calibrate(void) {
  cFFFFFF(0x15);
  cFFFFFF(0xff);
}
void GDClass::cmd_clock(int16_t x, int16_t y, int16_t r, uint16_t options, uint16_t h, uint16_t m, uint16_t s, uint16_t ms) {
  cFFFFFF(0x14);
  ch(x);
  ch(y);
  ch(r);
  cH(options);
  cH(h);
  cH(m);
  cH(s);
  cH(ms);
}
void GDClass::cmd_coldstart(void) {
  cFFFFFF(0x32);
}
void GDClass::cmd_dial(int16_t x, int16_t y, int16_t r, uint16_t options, uint16_t val) {
  cFFFFFF(0x2d);
  ch(x);
  ch(y);
  ch(r);
  cH(options);
  cH(val);
  cH(0);
}
void GDClass::cmd_dlstart(void) {
  cFFFFFF(0x00);
}
void GDClass::cmd_fgcolor(uint32_t c) {
  cFFFFFF(0x0a);
  cI(c);
}
void GDClass::cmd_gauge(int16_t x, int16_t y, int16_t r, uint16_t options, uint16_t major, uint16_t minor, uint16_t val, uint16_t range) {
  cFFFFFF(0x13);
  ch(x);
  ch(y);
  ch(r);
  cH(options);
  cH(major);
  cH(minor);
  cH(val);
  cH(range);
}
void GDClass::cmd_getmatrix(void) {
  cFFFFFF(0x33);
  ci(0);
  ci(0);
  ci(0);
  ci(0);
  ci(0);
  ci(0);
}
void GDClass::cmd_getprops(uint32_t &ptr, uint32_t &w, uint32_t &h) {
  cFFFFFF(0x25);
  ptr = GDTR.getwp();
  cI(0);
  w = GDTR.getwp();
  cI(0);
  h = GDTR.getwp();
  cI(0);
}
void GDClass::cmd_getptr(void) {
  cFFFFFF(0x23);
  cI(0);
}
void GDClass::cmd_gradcolor(uint32_t c) {
  cFFFFFF(0x34);
  cI(c);
}
void GDClass::cmd_gradient(int16_t x0, int16_t y0, uint32_t rgb0, int16_t x1, int16_t y1, uint32_t rgb1) {
  cFFFFFF(0x0b);
  ch(x0);
  ch(y0);
  cI(rgb0);
  ch(x1);
  ch(y1);
  cI(rgb1);
}
void GDClass::cmd_inflate(uint32_t ptr) {
  cFFFFFF(0x22);
  cI(ptr);
}
void GDClass::cmd_interrupt(uint32_t ms) {
  cFFFFFF(0x02);
  cI(ms);
}
void GDClass::cmd_keys(int16_t x, int16_t y, int16_t w, int16_t h, byte font, uint16_t options, const char*s) {
  cFFFFFF(0x0e);
  ch(x);
  ch(y);
  ch(w);
  ch(h);
  ch(font);
  cH(options);
  cs(s);
}
void GDClass::cmd_loadidentity(void) {
  cFFFFFF(0x26);
}
void GDClass::cmd_loadimage(uint32_t ptr, int32_t options) {
  cFFFFFF(0x24);
  cI(ptr);
  cI(options);
}
void GDClass::cmd_memcpy(uint32_t dest, uint32_t src, uint32_t num) {
  cFFFFFF(0x1d);
  cI(dest);
  cI(src);
  cI(num);
}
void GDClass::cmd_memset(uint32_t ptr, byte value, uint32_t num) {
  cFFFFFF(0x1b);
  cI(ptr);
  cI((uint32_t)value);
  cI(num);
}
uint32_t GDClass::cmd_memcrc(uint32_t ptr, uint32_t num) {
  cFFFFFF(0x18);
  cI(ptr);
  cI(num);
  uint32_t r = GDTR.getwp();
  cI(0xFFFFFFFF);
  return r;
}
void GDClass::cmd_memwrite(uint32_t ptr, uint32_t num) {
  cFFFFFF(0x1a);
  cI(ptr);
  cI(num);
}
void GDClass::cmd_regwrite(uint32_t ptr, uint32_t val) {
  cFFFFFF(0x1a);
  cI(ptr);
  cI(4UL);
  cI(val);
}
void GDClass::cmd_number(int16_t x, int16_t y, byte font, uint16_t options, uint32_t n) {
  cFFFFFF(0x2e);
  ch(x);
  ch(y);
  ch(font);
  cH(options);
  ci(n);
}

void GDClass::printNfloat(int16_t x, int16_t y, float f, int16_t Presc, byte font) {
 //Parte entera
 cmd_number(x - 2, y, font, OPT_RIGHTX | OPT_SIGNED, int(f));
 //Parte entera

 Presc = abs(Presc);
 
// Punto y parte decimal
if (Presc==1){
 cmd_text(  x,     y, font, 0, ".");
 cmd_number(x + 9, y, font, Presc, int(10 * abs(f))); // 1 decimal
 }  
if (Presc==2){
 cmd_text(  x,     y, font, 0, ".");
 cmd_number(x + 9, y, font, Presc, int(100 * abs(f))); // 2 decimales
 }
if (Presc==3){  
 cmd_text(  x,     y, font, 0, ".");
 cmd_number(x + 9, y, font, Presc, int(1000 * abs(f))); //3 decimales
 }
if (Presc==4){  
 cmd_text(  x,     y, font, 0, ".");
 cmd_number(x + 9, y, font, Presc, int(10000 * abs(f))); //4 decimales
 }
if (Presc==5){  
 cmd_text(  x,     y, font, 0, ".");
 cmd_number(x + 9, y, font, Presc, int(100000 * abs(f))); //5 decimales
 }
if (Presc==6){
 cmd_text(  x,     y, font, 0, ".");
 cmd_number(x + 9, y, font, Presc, int(1000000 * abs(f))); //6 decimales
 }
if (Presc==7){
 cmd_text(  x,     y, font, 0, ".");
 cmd_number(x + 9, y, font, Presc, int(10000000 * abs(f))); //7 decimales
 }
// Punto y parte decimal
}


void GDClass::cmd_progress(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t options, uint16_t val, uint16_t range) {
  cFFFFFF(0x0f);
  ch(x);
  ch(y);
  ch(w);
  ch(h);
  cH(options);
  cH(val);
  cH(range);
  cH(0);
}
void GDClass::cmd_regread(uint32_t ptr) {
  cFFFFFF(0x19);
  cI(ptr);
  cI(0);
}
void GDClass::cmd_rotate(int32_t a) {
  cFFFFFF(0x29);
  ci(a);
}
void GDClass::cmd_scale(int32_t sx, int32_t sy) {
  cFFFFFF(0x28);
  ci(sx);
  ci(sy);
}
void GDClass::cmd_screensaver(void) {
  cFFFFFF(0x2f);
}
void GDClass::cmd_scrollbar(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t options, uint16_t val, uint16_t size, uint16_t range) {
  cFFFFFF(0x11);
  ch(x);
  ch(y);
  ch(w);
  ch(h);
  cH(options);
  cH(val);
  cH(size);
  cH(range);
}
void GDClass::cmd_setfont(byte font, uint32_t ptr) {
  cFFFFFF(0x2b);
  cI(font);
  cI(ptr);
}
void GDClass::cmd_setmatrix(void) {
  cFFFFFF(0x2a);
}
void GDClass::cmd_sketch(int16_t x, int16_t y, uint16_t w, uint16_t h, uint32_t ptr, uint16_t format) {
  cFFFFFF(0x30);
  ch(x);
  ch(y);
  cH(w);
  cH(h);
  cI(ptr);
  cI(format);
}
void GDClass::cmd_slider(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t options, uint16_t val, uint16_t range) {
  cFFFFFF(0x10);
  ch(x);
  ch(y);
  ch(w);
  ch(h);
  cH(options);
  cH(val);
  cH(range);
  cH(0);
}
void GDClass::cmd_snapshot(uint32_t ptr) {
  cFFFFFF(0x1f);
  cI(ptr);
}
void GDClass::cmd_spinner(int16_t x, int16_t y, byte style, byte scale) {
  cFFFFFF(0x16);
  ch(x);
  ch(y);
  cH(style);
  cH(scale);
}
void GDClass::cmd_stop(void) {
  cFFFFFF(0x17);
}
void GDClass::cmd_swap(void) {
  cFFFFFF(0x01);
}
void GDClass::cmd_text(int16_t x, int16_t y, byte font, uint16_t options, const char *s) {
  cFFFFFF(0x0c);
  ch(x);
  ch(y);
  ch(font);
  cH(options);
  cs(s);
}
void GDClass::cmd_toggle(int16_t x, int16_t y, int16_t w, byte font, uint16_t options, uint16_t state, const char *s) {
  cFFFFFF(0x12);
  ch(x);
  ch(y);
  ch(w);
  ch(font);
  cH(options);
  cH(state);
  cs(s);
}
void GDClass::cmd_track(int16_t x, int16_t y, uint16_t w, uint16_t h, byte tag) {
  cFFFFFF(0x2c);
  ch(x);
  ch(y);
  ch(w);
  ch(h);
  ch(tag);
  ch(0);
}
void GDClass::cmd_translate(int32_t tx, int32_t ty) {
  cFFFFFF(0x27);
  ci(tx);
  ci(ty);
}
void GDClass::cmd_playvideo(int32_t options) {
  cFFFFFF(0x3a);
  cI(options);
}
void GDClass::cmd_romfont(uint32_t font, uint32_t romslot) {
  cFFFFFF(0x3f);
  cI(font);
  cI(romslot);
}
void GDClass::cmd_mediafifo(uint32_t ptr, uint32_t size) {
  cFFFFFF(0x39);
  cI(ptr);
  cI(size);
}
void GDClass::cmd_setbase(uint32_t b) {
  cFFFFFF(0x38);
  cI(b);
}
void GDClass::cmd_videoframe(uint32_t dst, uint32_t ptr) {
  cFFFFFF(0x41);
  cI(dst);
  cI(ptr);
}
void GDClass::cmd_snapshot2(uint32_t fmt, uint32_t ptr, int16_t x, int16_t y, int16_t w, int16_t h) {
  cFFFFFF(0x37);
  cI(fmt);
  cI(ptr);
  ch(x);
  ch(y);
  ch(w);
  ch(h);
}
void GDClass::cmd_setfont2(uint32_t font, uint32_t ptr, uint32_t firstchar) {
  cFFFFFF(0x3b);
  cI(font);
  cI(ptr);
  cI(firstchar);
}
void GDClass::cmd_setbitmap(uint32_t source, uint16_t fmt, uint16_t w, uint16_t h) {
  cFFFFFF(0x43);
  cI(source);
  ch(fmt);
  ch(w);
  ch(h);
  ch(0);
}

void GDClass::cmd_setrotate(uint32_t r) {
  cFFFFFF(0x36);
  cI(r);
  // As a special favor, update variables w and h according to this
  // rotation
  w = GDTR.rd16(REG_HSIZE);
  h = GDTR.rd16(REG_VSIZE);
  if (r & 2) {
    int t = h;
    h = w;
    w = t;
  }
}
void GDClass::cmd_videostart() {
  cFFFFFF(0x40);
}

byte GDClass::rd(uint32_t addr) {
  return GDTR.rd(addr);
}
void GDClass::wr(uint32_t addr, uint8_t v) {
  GDTR.wr(addr, v);
}
uint16_t GDClass::rd16(uint32_t addr) {
  return GDTR.rd16(addr);
}
void GDClass::wr16(uint32_t addr, uint16_t v) {
  GDTR.wr16(addr, v);
}
uint32_t GDClass::rd32(uint32_t addr) {
  return GDTR.rd32(addr);
}
void GDClass::wr32(uint32_t addr, uint32_t v) {
  GDTR.wr32(addr, v);
}
void GDClass::wr_n(uint32_t addr, byte *src, uint32_t n) {
  GDTR.wr_n(addr, src, n);
}

void GDClass::cmdbyte(uint8_t b) {
  GDTR.cmdbyte(b);
}
void GDClass::cmd32(uint32_t b) {
  GDTR.cmd32(b);
}
void GDClass::finish(void) {
  GDTR.finish();
}
//void GDClass::get_accel(int &x, int &y, int &z) {
//  static int f[3];

//  for (byte i = 0; i < 3; i++) {
//    int a = analogRead(A0 + i);
//    int s = (-160 * (a - 376)) >> 6;
//    f[i] = ((3 * f[i]) >> 2) + (s >> 2);
//  }
//  x = f[2];
//  y = f[1];
//  z = f[0];
//}
void GDClass::get_inputs(void) {
  GDTR.finish();
  byte *bi = (byte*)&inputs;
#if defined(DUMPDEV)
  extern FILE* stimfile;
  if (stimfile) {
    byte tag;
    fscanf(stimfile, "%hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx",
        &bi[0],
        &bi[1],
        &bi[2],
        &bi[3],
        &bi[4],
        &bi[5],
        &bi[6],
        &bi[7],
        &bi[8],
        &bi[9],
        &bi[10],
        &bi[11],
        &bi[12],
        &bi[13],
        &bi[14],
        &bi[15],
        &bi[16],
        &bi[17]);
    GDTR.wr(REG_TAG, tag);
  } else {
    inputs.x = inputs.y = -32768;
  }
#else
  GDTR.rd_n(bi, REG_TRACKER, 4);
  GDTR.rd_n(bi + 4, REG_TOUCH_RZ, 13);
  GDTR.rd_n(bi + 17, REG_TAG, 1);
  inputs.touching = (inputs.x != -32768);
  inputs.xytouch.set(PIXELS(inputs.x), PIXELS(inputs.y));
#ifdef DUMP_INPUTS
  for (size_t i = 0; i < sizeof(inputs); i++) {
    Serial.print(bi[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
#endif
#endif
}
void GDClass::bulkrd(uint32_t a) {
  GDTR.bulk(a);
}
void GDClass::resume(void) {
  GDTR.resume();
}
void GDClass::__end(void) {
// #if !defined(DUMPDEV) && !defined(RASPBERRY_PI)
  GDTR.__end();
// #endif
}
void GDClass::play(uint8_t instrument, uint8_t note) {
  wr16(REG_SOUND, (note << 8) | instrument);
  wr(REG_PLAY, 1);
}
void GDClass::sample(uint32_t start, uint32_t len, uint16_t freq, uint16_t format, int loop) {
  GD.wr32(REG_PLAYBACK_START, start);
  GD.wr32(REG_PLAYBACK_LENGTH, len);
  GD.wr16(REG_PLAYBACK_FREQ, freq);
  GD.wr(REG_PLAYBACK_FORMAT, format);
  GD.wr(REG_PLAYBACK_LOOP, loop);
  GD.wr(REG_PLAYBACK_PLAY, 1);
}
void GDClass::reset() {
  GDTR.__end();
  GDTR.wr(REG_CPURESET, 1);
  GDTR.wr(REG_CPURESET, 0);
  GDTR.resume();
}

// Load named file from storage
// returns 0 on failure (e.g. file not found), 1 on success

typedef void (* FUNC_POINTER)(long, long);

//byte GDClass::load(const char *filename, void (* progress)(long, long))
byte GDClass::load(const char *filename, FUNC_POINTER progress)
{
  File archivo;
  archivo = SD.open(filename);  
//Serial.println("archivo=" + String(archivo));
//  return (GD.loadSdFat(archivo)); 
  return (loadSdFat(archivo, progress)); 
}

// Load opened file from storage
// returns 0 on failure (e.g. file not found), 1 on success

//byte GDClass::loadSdFat(File& archivo, void (*progress)(long, long))
byte GDClass::loadSdFat(File& archivo, FUNC_POINTER progress)
{

  int offset=0;
  int offsetFT;
  int bytesDisponibles;
  int wbn;
  
  GD.__end();
  
  if (archivo) {
    
	int tamArchivo=archivo.available();
    
    while( offset < tamArchivo)
    {
        uint16_t m = ( (archivo.available() > TAM_BUFFER_SD) ? TAM_BUFFER_SD : archivo.available());
        archivo.read(buf,m);
        
        offsetFT=0;
        bytesDisponibles=m;
        while (offsetFT < m) 
        {
              uint16_t n = ( (bytesDisponibles > TAM_BUFFER_FT) ? TAM_BUFFER_FT : bytesDisponibles);
             
              wbn = (n + 3) & ~3;   // force 32-bit alignment

              GD.resume();
              if (progress)
                (*progress)((offset+offsetFT), tamArchivo);
#ifdef DUMMY_GDCOPYRAM
                memcpy (FTbuf, buf+offsetFT, wbn );
#else
                GD.copyram((buf+offsetFT), wbn);
#endif              
              offsetFT+=n;
              bytesDisponibles-=n;
              GDTR.stop();
        }
        offset+=m;         
    }	
            
    GD.resume();
    SPI.endTransaction();
    return 1;
  }
  GD.resume();
  SPI.endTransaction();
  return 0;
}

// Generated by mk_bsod.py. Blue screen with 'ERROR' text
static const PROGMEM uint8_t __bsod[32] = {
0, 255, 255, 255, 96, 0, 0, 2, 7, 0, 0, 38, 12, 255, 255, 255, 240, 0,
90, 0, 31, 0, 0, 6, 69, 82, 82, 79, 82, 0, 0, 0
};
static const PROGMEM uint8_t __bsod_badfile[32] = {
12, 255, 255, 255, 240, 0, 148, 0, 29, 0, 0, 6, 67, 97, 110, 110, 111,
116, 32, 111, 112, 101, 110, 32, 102, 105, 108, 101, 58, 0, 0, 0
};

// Fatal error alert.
// Show a blue screen with message.
// This method never returns.

void GDClass::alert(const char *message)
{
  begin(0);
  //copy(__bsod, sizeof(__bsod));
  //cmd_text(240, 176, 29, OPT_CENTER, message);
  cmd_dlstart();
 
  if(FT800_model != 0){cmd_setrotate(ORIENTACION);}
  if(FT800_model == 0){GDTR.wr(REG_ROTATE, ROTACION);}

  ClearColorRGB(0x650000);
  Clear();

  if (strlen(message) == 0) {
    cmd_text(GD.w / 2, (GD.h / 2)-29, 31, OPT_CENTER, "Error on STM32");
  } else {
    if (strlen(message) < 20) {
      cmd_text(GD.w / 2, (GD.h / 2)-29, 31, OPT_CENTER, message);
    } else {
      cmd_text(GD.w / 2, (GD.h / 2)-29, 29, OPT_CENTER, message);
    }
  }
  cmd_text(GD.w / 2, (GD.h / 2)+29, 29, OPT_CENTER, "Invalid operation");

  swap();
  finish();
  for (;;)
    ;
}

void GDClass::safeload(const char *filename)
{
  if (!load(filename)) {
    copy(__bsod, sizeof(__bsod));
    //copy(__bsod_badfile, sizeof(__bsod_badfile));
    //cmd_text(240, 190, 29, OPT_CENTER, filename);
    cmd_text(0, GD.h/2, 29, 0, "- Check asset file name inside of the sketch");
    cmd_text(0, GD.h/2+30, 29, 0, "- Copy *.gd2 in the SD card");
    swap();
    for (;;)
      ;
  }
}

#if 0
void GDClass::safeloadSdFat(File& archivo)
{
  if (!loadSdFat(archivo)) {
    copy(__bsod, sizeof(__bsod));
    //copy(__bsod_badfile, sizeof(__bsod_badfile));
    //cmd_text(240, 190, 29, OPT_CENTER, archivo.name());
    cmd_text(0, GD.h/2, 29, 0, "- Check asset file name inside of the sketch");
    cmd_text(0, GD.h/2+30, 29, 0, "- Copy *.gd2 in the SD card");
    swap();
    for (;;)
      ;
  }
}
#endif

#define REG_SCREENSHOT_EN    (ft8xx_model ? 0x302010UL : 0x102410UL) // Set to enable screenshot mode
#define REG_SCREENSHOT_Y     (ft8xx_model ? 0x302014UL : 0x102414UL) // Y line register
#define REG_SCREENSHOT_START (ft8xx_model ? 0x302018UL : 0x102418UL) // Screenshot start trigger
#define REG_SCREENSHOT_BUSY  (ft8xx_model ? 0x3020e8UL : 0x1024d8UL) // Screenshot ready flags
#define REG_SCREENSHOT_READ  (ft8xx_model ? 0x302174UL : 0x102554UL) // Set to enable readout
#define RAM_SCREENSHOT       (ft8xx_model ? 0x3c2000UL : 0x1C2000UL) // Screenshot readout buffer

#ifndef DUMPDEV
void GDClass::dumpscreen(void)    
{
  {
    finish();

    wr(REG_SCREENSHOT_EN, 1);
    Serial.write(0xa5);
    Serial.write(GD.w & 0xff);
    Serial.write((GD.w >> 8) & 0xff);
    Serial.write(GD.h & 0xff);
    Serial.write((GD.h >> 8) & 0xff);
    for (int ly = 0; ly < GD.h; ly++) {
      wr16(REG_SCREENSHOT_Y, ly);
      wr(REG_SCREENSHOT_START, 1);
      delay(2);
      while (rd32(REG_SCREENSHOT_BUSY) | rd32(REG_SCREENSHOT_BUSY + 4))
        ;
      wr(REG_SCREENSHOT_READ, 1);
      bulkrd(RAM_SCREENSHOT);
      SPI.transfer(0xff);
      for (int x = 0; x < GD.w; x += 8) {
        union {
          uint32_t v;
          struct {
            uint8_t b, g, r, a;
          };
        } block[8];
        for (int i = 0; i < 8; i++) {
          block[i].b = SPI.transfer(0xff);
          block[i].g = SPI.transfer(0xff);
          block[i].r = SPI.transfer(0xff);
          block[i].a = SPI.transfer(0xff);
        }
        // if (x == 0) block[0].r = 0xff;
        byte difference = 1;
        for (int i = 1, mask = 2; i < 8; i++, mask <<= 1)
          if (block[i].v != block[i-1].v)
            difference |= mask;
        Serial.write(difference);

        for (int i = 0; i < 8; i++)
          if (1 & (difference >> i)) {
            Serial.write(block[i].b);
            Serial.write(block[i].g);
            Serial.write(block[i].r);
          }
      }
      resume();
      wr(REG_SCREENSHOT_READ, 0);
    }
    wr16(REG_SCREENSHOT_EN, 0);
  }
}
#endif
