#define RGB_PIN 48  
#define NUM_LEDS 1
#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel leds(NUM_LEDS, RGB_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  leds.begin();
  leds.setBrightness(30); // 
}

void loop() {
  leds.setPixelColor(0, leds.Color(255,0,0)); leds.show(); delay(500); // 红
  leds.setPixelColor(0, leds.Color(0,255,0)); leds.show(); delay(500); // 绿
  leds.setPixelColor(0, leds.Color(0,0,255)); leds.show(); delay(500); // 蓝
}