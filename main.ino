#include <FastLED.h>

#define LED_PIN 2
#define NUM_LEDS 12
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

#define ROTATION_TIME 4000 // 1 rotation every 12 s

/* Optical parameters
The minimum amount of light that is always on, even when neither a light focus
nor the shadow passes. 40–80 → realistic, industrial lighthouse 120–180 →
decorative, soft 0 → pure searchlight (beam only)
*/
#define BASE_BRIGHTNESS 0

/* How much each light focus boosts brightness when passing an LED.
80–120 → soft / realistic
150–220 → powerful focus
>230 → show effect
*/
#define LIGHT_PEAK 200

/* How much brightness is subtracted when the shutter (shadow) passes.
120–160 → slight shadow
180–220 → real shutter
>230 → total blackout
*/
#define SHADOW_DEPTH 200

/* The angular width of the focus or shadow, expressed as a fraction of a turn:
0.06  brutal cut
0.10  classic lighthouse
0.15  realistic
0.20  decorative
>0.25 diffuse
*/
#define FOCUS_WIDTH 0.20

#define LIGHT_MODE true // true = light moves, false = shadow moves

float circDist(float a, float b) {
  float d = fabs(a - b);
  if (d > 0.5)
    d = 1.0 - d;
  return d;
}

float gaussian(float d, float w) {
  float x = d / w;
  return exp(-x * x * 2.0);
}

void setup() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();
  FastLED.setTemperature(Tungsten100W);
  // FastLED.setTemperature(Candle);
}

void loop() {
  float rot = (millis() % ROTATION_TIME) / (float)ROTATION_TIME;

  FastLED.clear();

  for (uint8_t i = 0; i < NUM_LEDS; i++) {

    float ledPos = i / (float)NUM_LEDS;

    float lightSum = 0.0;
    float shadowVal = 0.0;

    // 3 light focuses
    for (uint8_t f = 0; f < 3; f++) {
      float focusPos = f * 0.25 + rot;
      if (focusPos >= 1.0)
        focusPos -= 1.0;

      float d = circDist(ledPos, focusPos);
      lightSum += gaussian(d, FOCUS_WIDTH);
    }

    // shadow focus
    float shadowPos = 0.75 + rot;
    if (shadowPos >= 1.0)
      shadowPos -= 1.0;

    shadowVal = gaussian(circDist(ledPos, shadowPos), FOCUS_WIDTH * 1.3);

    // inside loop(), replace ONLY the brightness block with this one

    float brightness;

    if (!LIGHT_MODE) {
      // ===== ROTATING SHADOW (as you already like) =====
      brightness =
          BASE_BRIGHTNESS + lightSum * LIGHT_PEAK - shadowVal * SHADOW_DEPTH;

    } else {
      // ===== REAL ROTATING LIGHT =====

      // main beam (only one)
      float mainBeam = gaussian(circDist(ledPos, rot), FOCUS_WIDTH * 0.7);

      // secondary focuses (very soft)
      float secondary = lightSum * 0.25;

      brightness = mainBeam * 255 + // dominant beam
                   secondary * 60 - // slight fill
                   shadowVal * 40;  // almost passive shadow
    }

    brightness = constrain(brightness, 0, 255);
    brightness = pow(brightness / 255.0, 2.2) * 255.0;

    leds[i] = CRGB(brightness, brightness, brightness - 10);
  }

  FastLED.show();
  delay(10);
}
