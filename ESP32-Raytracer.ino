
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


#include "FS.h"
#include <SD_MMC.h>
#include "tinyraytracer.h" // a modified version of https://github.com/ssloy/tinyraytracer
#include "tiny_jpeg_encoder.h" // a modified version of https://github.com/serge-rgb/TinyJPEG


#define MY_USER_SELECT_SETUP "F:\git\ESP32-Raytracer\src\bodmer_deps.h"

#include MY_USER_SELECT_SETUP


#include <TFT_eSPI-ST7789.h>

#include <Adafruit_NeoPixel.h>


#ifdef USE_ADAFRUIT
  #include <Arduino_ST7789.h> // Hardware-specific library for ST7789 (with or without CS pin)
  Arduino_ST7789 tft = Arduino_ST7789(TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK); //for display without CS pin
#else



  #include <TFT_eSPI.h>
  #include <JPEGDecoder.h>
  TFT_eSPI tft = TFT_eSPI(240, 240);
  //#include "bodmer_deps.h"

  // Color definitions
  #define  BLACK   0x0000
  #define BLUE    0x001F
  #define RED     0xF800
  #define GREEN   0x07E0
  #define CYAN    0x07FF
  #define MAGENTA 0xF81F
  #define YELLOW  0xFFE0
  #define WHITE   0xFFFF
  //#define tft.drawJpgFile drawJpgFile

  
  void jpegInfo() {
    // Print information extracted from the JPEG file
    Serial.println("JPEG image info");
    Serial.println("===============");
    Serial.print("Width      :");
    Serial.println(JpegDec.width);
    Serial.print("Height     :");
    Serial.println(JpegDec.height);
    Serial.print("Components :");
    Serial.println(JpegDec.comps);
    Serial.print("MCU / row  :");
    Serial.println(JpegDec.MCUSPerRow);
    Serial.print("MCU / col  :");
    Serial.println(JpegDec.MCUSPerCol);
    Serial.print("Scan type  :");
    Serial.println(JpegDec.scanType);
    Serial.print("MCU width  :");
    Serial.println(JpegDec.MCUWidth);
    Serial.print("MCU height :");
    Serial.println(JpegDec.MCUHeight);
    Serial.println("===============");
    Serial.println("");
  }
  
  
  void showTime(uint32_t msTime) {
    Serial.print(F(" JPEG drawn in "));
    Serial.print(msTime);
    Serial.println(F(" ms "));
  }
  
  
  uint32_t jpegRender(int xpos, int ypos) {
    uint16_t *pImg;
    uint16_t mcu_w = JpegDec.MCUWidth;
    uint16_t mcu_h = JpegDec.MCUHeight;
    uint32_t max_x = JpegDec.width;
    uint32_t max_y = JpegDec.height;
    bool swapBytes = tft.getSwapBytes();
    tft.setSwapBytes(true);
    // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
    // Typically these MCUs are 16x16 pixel blocks
    // Determine the width and height of the right and bottom edge image blocks
    uint32_t min_w = min(mcu_w, max_x % mcu_w);
    uint32_t min_h = min(mcu_h, max_y % mcu_h);
      // save the current image block size
    uint32_t win_w = mcu_w;
    uint32_t win_h = mcu_h;
    // record the current time so we can measure how long it takes to draw an image
    uint32_t drawTime = millis();
    // save the coordinate of the right and bottom edges to assist image cropping
    // to the screen size
    max_x += xpos;
    max_y += ypos;
    // Fetch data from the file, decode and display
    while (JpegDec.read()) {    // While there is more data in the file
      pImg = JpegDec.pImage ;   // Decode a MCU (Minimum Coding Unit, typically a 8x8 or 16x16 pixel block)
      // Calculate coordinates of top left corner of current MCU
      int mcu_x = JpegDec.MCUx * mcu_w + xpos;
      int mcu_y = JpegDec.MCUy * mcu_h + ypos;
      // check if the image block size needs to be changed for the right edge
      if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
      else win_w = min_w;
      // check if the image block size needs to be changed for the bottom edge
      if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
      else win_h = min_h;
      // copy pixels into a contiguous block
      if (win_w != mcu_w) {
        uint16_t *cImg;
        int p = 0;
        cImg = pImg + win_w;
        for (int h = 1; h < win_h; h++) {
          p += mcu_w;
          for (int w = 0; w < win_w; w++) {
            *cImg = *(pImg + w + p);
            cImg++;
          }
        }
      }
      // calculate how many pixels must be drawn
      uint32_t mcu_pixels = win_w * win_h;
      // draw image MCU block only if it will fit on the screen
      if (( mcu_x + win_w ) <= tft.width() && ( mcu_y + win_h ) <= tft.height())
        tft.pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
      else if ( (mcu_y + win_h) >= tft.height())
        JpegDec.abort(); // Image has run off bottom of screen so abort decoding
    }
    tft.setSwapBytes(swapBytes);
    //showTime(millis() - drawTime); // These lines are for sketch testing only
    return millis() - drawTime;
  }


  uint32_t drawJpgFile(const char* fileName, int xpos=0, int ypos=0) {
    // Open the named file (the Jpeg decoder library will close it)
    File jpegFile = SD_MMC.open( fileName, FILE_READ);  // or, file handle reference for SD library
    // Use one of the following methods to initialise the decoder:
    boolean decoded = JpegDec.decodeSdFile(jpegFile);  // Pass the SD file handle to the decoder,
    //boolean decoded = JpegDec.decodeSdFile(filename);  // or pass the filename (String or character array)
    if (decoded) {
      return jpegRender(xpos, ypos);
    } else {
      Serial.println("Jpeg file format not supported!");
    }
    return 0;
  }
 
#endif


Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);


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

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  colorWipe(strip.Color(0, 0, 0), 50);

  #ifdef USE_ADAFRUIT
    tft.init(240, 240);   // initialize a ST7789 chip, 240x240 pixels
    tft.fillScreen( BLACK );
    tft.drawJpgFile(SD_MMC, "/out2.jpg", 0, 0, 100, 100, 0, 0, JPEG_DIV_NONE);
  #else
    tft.begin();
    tft.fillRect(0, 0, tft.width(), tft.height(), BLACK);
    //drawJpgFile("/out2.jpg");
  #endif

  //while(1) { ; }

  tinyRayTracerInit();
  tinyJpegEncoderInit();
 
}

bool rendered = false;

void loop() {

  uint16_t x = 36;
  uint16_t y = 72;
  uint16_t width = 128;
  uint16_t height = 128;

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

      //if(SD_MMC.exists( fName )) {
      //  continue;
      //}

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
        float framelen = ( (millis() - started) / (framenum+1) ) / 1000;
        int remaining = (looplength - framenum) * framelen;

        #ifdef USE_ADAFRUIT
          tft.drawJpgFile(SD_MMC, jpegFileName/*, x, y, width, height, 0, 0, JPEG_DIV_NONE*/);
        #else
          uint32_t timeToRender = drawJpgFile(jpegFileName, x, y/*, width, height, 0, 0, JPEG_DIV_NONE*/);
        #endif

/*
        Serial.printf("[%d / %d] Rendered %d out of %d (%d) as %s. Estimated time remaining: %d seconds\n", 
          ESP.getFreeHeap(), 
          ESP.getFreePsram(), 
          framenum+1, 
          looplength, 
          framelen, 
          jpegFileName, 
          remaining
        );
*/

        /*
        tft.setCursor(0,10);
        tft.fillRect(0,10,tft.width(), 20, 0);
        tft.printf("Rendered %d out of %d", framenum+1, looplength);
        tft.setCursor(0,20);
        tft.printf("Estimated time remaining: %d seconds", remaining);
        */
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
    #ifdef USE_ADAFRUIT
      tft.drawJpgFile(SD_MMC, jpegFileName, x, y/*, width, height, 0, 0, JPEG_DIV_NONE*/);
    #else
      drawJpgFile(jpegFileName, x, y/*, width, height, 0, 0, JPEG_DIV_NONE*/);
    #endif
  }

  
}
