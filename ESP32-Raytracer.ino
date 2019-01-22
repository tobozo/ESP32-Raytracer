
#include <Adafruit_GFX.h>   // Core graphics library
#include "WROVER_KIT_LCD.h" // Latest version must have the VScroll def patch: https://github.com/espressif/WROVER_KIT_LCD/pull/3/files
WROVER_KIT_LCD tft;

#include "tinyraytracer.h"

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  tft.begin();
  tft.setRotation( 0 ); // required to get smooth scrolling
  tft.setTextColor(WROVER_YELLOW);
  tft.fillScreen(WROVER_BLACK);
  
  raytrace();
  Serial.println("Setup done");
}

void loop() {
  // put your main code here, to run repeatedly:
 
}
