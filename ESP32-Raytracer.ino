
#define TFT_DC    23
#define TFT_RST   32
#define TFT_MOSI  26   // for hardware SPI data pin (all of available pins)
#define TFT_SCLK  27   // for hardware SPI sclk pin (all of available pins)
#define BUTTON_PRESS   18  
#define BUTTON_LEFT    5                         
#define BUTTON_RIGHT   19                         
#define PIXEL_PIN    25    // Digital IO pin connected to the NeoPixels.
#define PIXEL_COUNT 1
#define BUZZER 33
#include <Arduino_ST7789.h> // Hardware-specific library for ST7789 (with or without CS pin)
#include <Adafruit_NeoPixel.h>
#include "FS.h"
#include <SD_MMC.h>
Arduino_ST7789 tft = Arduino_ST7789(TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK); //for display without CS pin
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

#include "tinyraytracer.h" // a modified version of https://github.com/ssloy/tinyraytracer
#include "tiny_jpeg_encoder.h" // a modified version of https://github.com/serge-rgb/TinyJPEG

struct point {
  float initialx;
  float initialy;
  float initialz;
  float x;
  float y;
  float z;
};

struct color {
  float r;
  float g;
  float b;
};

point ivorySphereCoords = { 0, -1.5, -14 };
color ivoryColor =  {0.4, 0.4, 0.3 };

point glassSphereCoorsd = { 0, -1.5, -14 };



void raytrace(uint16_t x, uint16_t y, uint16_t width, uint16_t height, float fov) {
  Material      ivory(1.0, Vec4f(0.6,  0.3, 0.1, 0.0), Vec3f(ivoryColor.r, ivoryColor.g, ivoryColor.b),   50.);
  Material      glass(1.5, Vec4f(0.0,  0.5, 0.1, 0.8), Vec3f(0.6, 0.7, 0.8),  125.);
  //Material red_rubber(1.0, Vec4f(0.9,  0.1, 0.0, 0.0), Vec3f(0.3, 0.1, 0.1),   10.);
  Material     mirror(1.0, Vec4f(0.0, 10.0, 0.8, 0.0), Vec3f(1.0, 1.0, 1.0), 1425.);

  std::vector<Sphere> spheres;
  spheres.push_back(Sphere(Vec3f(ivorySphereCoords.x, ivorySphereCoords.y, ivorySphereCoords.z), 2, ivory));
  spheres.push_back(Sphere(Vec3f(glassSphereCoorsd.x, glassSphereCoorsd.y, glassSphereCoorsd.z), 2, glass));
  //spheres.push_back(Sphere(Vec3f( 1.5, -0.5, -18), 3, red_rubber));
  spheres.push_back(Sphere(Vec3f( 7,    5,   -18), 4,     mirror));

  std::vector<Light>  lights;
  lights.push_back(Light(Vec3f(-20, 20,  20), 1.5));
  lights.push_back(Light(Vec3f( 30, 50, -25), 1.8));
  lights.push_back(Light(Vec3f( 30, 20,  30), 1.7));

  render(x, y, width, height, spheres, lights, fov);
}


// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  if( !SD_MMC.begin() ) {
    Serial.println("SD MMC begin failed");
  } else {
    Serial.println("SD MMC OK");
  }
  if( !psramInit() ) {
    Serial.println("PSRAM FAIL");
    while(1) {
      ;
    }
  } else {
    Serial.println("PSRAM OK");
  }

  tinyRayTracerInit();
  tinyJpegEncoderInit();

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  colorWipe(strip.Color(0, 0, 0), 50);

  tft.init(240, 240);   // initialize a ST7789 chip, 240x240 pixels
  tft.fillScreen( BLACK );
 
}

bool rendered = false;

void loop() {

  uint16_t x = 0;
  uint16_t y = 0;
  uint16_t width = 240;
  uint16_t height = 240;

  char * fName = NULL;
  fName = (char*)malloc(32);

  byte looplength = 60;

  if(!rendered) {

    tft.setCursor(0,0);
    tft.print("Rendering");
    unsigned long started = millis();

    glassSphereCoorsd.x = glassSphereCoorsd.initialx;
    glassSphereCoorsd.z = glassSphereCoorsd.initialz;
    glassSphereCoorsd.y = glassSphereCoorsd.initialy;

    for(byte framenum=0; framenum<looplength;framenum++) {

      sprintf(fName, "/out%d.jpg", framenum);
      const char* jpegFileName = fName;

      ivorySphereCoords.x = ivorySphereCoords.initialx + (4*sin( ((float)framenum/looplength)*PI*2 ));
      ivorySphereCoords.z = ivorySphereCoords.initialz + (2*cos( ((float)framenum/looplength)*PI*2 ));
      ivorySphereCoords.y = ivorySphereCoords.initialy + fabs(2.75*cos( ((float)((framenum*2)%looplength)/looplength)*PI*2 ));

      //ivoryColor.r = fabs(.1*sin( ((float)framenum/looplength)*PI*2 )) + .3;
      //ivoryColor.g = ivoryColor.r;
      //ivoryColor.b = ivoryColor.r - 0.1;
  
      raytrace(x, y, width, height, 0.6);
  
      if ( !tje_encode_to_file(jpegFileName, width, height, 3 /*3=RGB,4=RGBA*/, rgbBuffer) ) {
        Serial.println("Could not write JPEG\n");
      } else {
        Serial.printf("[%d / %d] Rendering saved jpeg %s\n", ESP.getFreeHeap(), ESP.getFreePsram(), jpegFileName);
        float framelen = ( (millis() - started) / (framenum+1) ) / 1000;
        int remaining = (looplength - framenum) * framelen;
        tft.drawJpgFile(SD_MMC, jpegFileName/*, x, y, width, height, 0, 0, JPEG_DIV_NONE*/);
        tft.setCursor(0,10);
        tft.fillRect(0,10,tft.width(), 20, 0);
        tft.printf("Rendered %d out of %d", framenum+1, looplength);
        tft.setCursor(0,20);
        tft.printf("Estimated time remaining: %d seconds", remaining);
      }
    }
    rendered = true;
    unsigned long ended = (millis() - started)/1000;
    Serial.printf("Rendered animation in %d seconds\n", ended);
    tft.fillRect(0,0,tft.width(), 30, 0);

  }

  for(byte framenum=0; framenum<looplength; framenum++) {
    sprintf(fName, "/out%d.jpg", framenum);
    const char* jpegFileName = fName;
    tft.drawJpgFile(SD_MMC, jpegFileName/*, x, y, width, height, 0, 0, JPEG_DIV_NONE*/);
  }

  
}
