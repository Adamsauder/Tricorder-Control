// Alternative color order test - if strips show wrong colors
// Copy this section to replace the FastLED initialization in main.cpp

  // Initialize LED strips - TESTING RGB COLOR ORDER
  FastLED.addLeds<WS2812B, LED_PIN_1, GRB>(leds1, NUM_LEDS_1);  // Strip 1: GPIO10, 7 LEDs (keep GRB - working)
  FastLED.addLeds<WS2812, LED_PIN_2, RGB>(leds2, NUM_LEDS_2);   // Strip 2: GPIO6, 4 LEDs (try RGB)
  FastLED.addLeds<WS2812, LED_PIN_3, RGB>(leds3, NUM_LEDS_3);   // Strip 3: GPIO7, 4 LEDs (try RGB)

/* Alternative options to try:
  
  // Option 1: All RGB
  FastLED.addLeds<WS2812, LED_PIN_1, RGB>(leds1, NUM_LEDS_1);
  FastLED.addLeds<WS2812, LED_PIN_2, RGB>(leds2, NUM_LEDS_2);
  FastLED.addLeds<WS2812, LED_PIN_3, RGB>(leds3, NUM_LEDS_3);

  // Option 2: All GBR  
  FastLED.addLeds<WS2812B, LED_PIN_1, GBR>(leds1, NUM_LEDS_1);
  FastLED.addLeds<WS2812B, LED_PIN_2, GBR>(leds2, NUM_LEDS_2);
  FastLED.addLeds<WS2812B, LED_PIN_3, GBR>(leds3, NUM_LEDS_3);

  // Option 3: Mixed (if different strip types)
  FastLED.addLeds<WS2812B, LED_PIN_1, GRB>(leds1, NUM_LEDS_1);  // Working strip
  FastLED.addLeds<WS2812, LED_PIN_2, RGB>(leds2, NUM_LEDS_2);   // Different type
  FastLED.addLeds<WS2812, LED_PIN_3, RGB>(leds3, NUM_LEDS_3);   // Different type
*/
