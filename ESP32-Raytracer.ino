
#include <Adafruit_GFX.h>   // Core graphics library
#include "WROVER_KIT_LCD.h" // Latest version must have the VScroll def patch: https://github.com/espressif/WROVER_KIT_LCD/pull/3/files
WROVER_KIT_LCD tft;

#include "tinyraytracer.h" // a modified version of https://github.com/ssloy/tinyraytracer


void raytrace() {
  Material      ivory(1.0, Vec4f(0.6,  0.3, 0.1, 0.0), Vec3f(0.4, 0.4, 0.3),   50.);
  Material      glass(1.5, Vec4f(0.0,  0.5, 0.1, 0.8), Vec3f(0.6, 0.7, 0.8),  125.);
  Material red_rubber(1.0, Vec4f(0.9,  0.1, 0.0, 0.0), Vec3f(0.3, 0.1, 0.1),   10.);
  Material     mirror(1.0, Vec4f(0.0, 10.0, 0.8, 0.0), Vec3f(1.0, 1.0, 1.0), 1425.);

  std::vector<Sphere> spheres;
  spheres.push_back(Sphere(Vec3f(-3,    0,   -16), 2,      ivory));
  spheres.push_back(Sphere(Vec3f(-1.0, -1.5, -12), 2,      glass));
  spheres.push_back(Sphere(Vec3f( 1.5, -0.5, -18), 3, red_rubber));
  spheres.push_back(Sphere(Vec3f( 7,    5,   -18), 4,     mirror));

  std::vector<Light>  lights;
  lights.push_back(Light(Vec3f(-20, 20,  20), 1.5));
  lights.push_back(Light(Vec3f( 30, 50, -25), 1.8));
  lights.push_back(Light(Vec3f( 30, 20,  30), 1.7));

  render(spheres, lights);
}


void setup() {
  Serial.begin(115200);

  tft.begin();
  tft.setRotation( 0 );
  tft.setTextColor(WROVER_YELLOW);
  tft.fillScreen(WROVER_BLACK);
  
  raytrace();
  Serial.println("Setup done");
}

void loop() {
  //  
}
