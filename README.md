# STM32_GD2
FT800/FT81X TFT Library for Arduino_STM32

I ported from here.   

https://github.com/lightcalamar/GD23STM32_F103x   

You can play video.   

----

# Software requirement    

Core library    
https://github.com/rogerclarkmelbourne/Arduino_STM32   

Arduino FAT16/FAT32 Library   
https://github.com/greiman/SdFat   

----

# Wirering for FT8XX TFT   


|TFT||STM32F103||SD-CARD|
|:-:|:-:|:-:|:-:|:-:|
|GND|--|GND|
|MISO|--|PA6|
|CS|--|PA4(*)|
|PD|--|PB1(*)|
|5V|--|5V|
|SCK|--|PA5|
|MISO|--|PA7|
|INT|--|N/C|
|AUDIO|--|(**)|
|||PB12(*)|--|CS|
|||PB13|--|SCK|
|||PB15|--|MOSI|
|||PB14|--|MISO|
|||5V|--|VCC|
|||GND|--|GND|


\*You can change any pin.  
Pin define is "STM32_GD2.h"   

\**If you want to hear Audio, You need External Audio Circuit.  
The complete Audio Circuit is here.   
https://www.soselectronic.cz/a_info/resource/c/FTDI/AN_252_FT800_Audio_Primer.pdf   

**All wiring has to be the length below the 5CM.**  
**When wiring is the length beyond 5CM, it doesn't work mostly.**   

----

# Tested TFT    
5" Graphical LCD Touchscreen, 480x272, SPI, FT800   
https://www.hotmcu.com/5-graphical-lcd-touchscreen-480x272-spi-ft800-p-124.html?cPath=6_16&zenid=kfemikenqkhmjdmc0evd16j1m4   

5" Graphical LCD Touchscreen, 800x480, SPI, FT810   
https://www.hotmcu.com/5-graphical-lcd-touchscreen-800x480-spi-ft810-p-286.html?cPath=6_16&zenid=kfemikenqkhmjdmc0evd16j1m4   

----

# How to use   

Download Gameduino2sd.zip from here & unzip it & copy all file to your SD-CARD.   
http://excamera.com/sphinx/gameduino2/code.html   


Wireing TFT & SD-CARD reader to STM32F103.   

Try example.   

----

# The limitation of FT800    

FT800 have 482x272 resolution.   
FT800 doesn't have the function which plays the video.   
FT800 doesn't have the function which revolves around a screen.   
You need a stylus pen for touch panel operation.   

----

# The limitation of FT810    

FT810 have 800x480 resolution.   
You need a stylus pen for touch panel operation.   

----

# begin options   

You can use there option in begin().   

GD_CALIBRATE   
Execute Calibration of a touch sensor.   
A screen for calibration is indicated.   
If there is calibration data already, nothing is done.   
Calibration data is preserved by EEPROM.   

GD_TRIM   
Execute sound tests.   

GD_STORAGE   
A SD card is initialized.   
When you use a SD card, it's always necessary.   

FORCE_CALIBRATE   
Execute Calibration of a touch sensor unconditionally.   
A screen for calibration is indicated.   
Calibration data is preserved by EEPROM.   

GD_NONE   
Nothing is done.   

---

# Demo Video

https://youtu.be/yPd8jYOi1hY   
