#include <FastLED.h>

#define LED_PIN 2
#define NUM_LEDS 12
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

#define ROTATION_TIME 4000 // 1 vuelta cada 12 s

/* Parámetros ópticos
La cantidad mínima de luz que siempre está encendida, aun cuando no pasa ni un
foco de luz ni la sombra. 40–80 → faro realista, industrial 120–180 →
decorativo, suave 0 → searchlight puro (solo haz)
*/
#define BASE_BRIGHTNESS 0

/*Cuánto refuerza el brillo cada foco de luz cuando pasa por un LED.
80–120 → suave / realista
150–220 → foco potente
>230 → efecto show
*/
#define LIGHT_PEAK 200

/*Cuánto se resta de brillo cuando pasa el obturador (sombra).
120–160 → sombra leve
180–220 → obturador real
>230 → blackout total
*/
#define SHADOW_DEPTH 200

/*El ancho angular del foco o de la sombra, expresado como fracción de vuelta:
0.06  corte brutal
0.10  faro clásico
0.15  realista
0.20  decorativo
>0.25 difuso
*/
#define FOCUS_WIDTH 0.20

#define LIGHT_MODE true // true = se mueve la luz, false = se mueve la sombra

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

    // 3 focos de luz
    for (uint8_t f = 0; f < 3; f++) {
      float focusPos = f * 0.25 + rot;
      if (focusPos >= 1.0)
        focusPos -= 1.0;

      float d = circDist(ledPos, focusPos);
      lightSum += gaussian(d, FOCUS_WIDTH);
    }

    // foco de sombra
    float shadowPos = 0.75 + rot;
    if (shadowPos >= 1.0)
      shadowPos -= 1.0;

    shadowVal = gaussian(circDist(ledPos, shadowPos), FOCUS_WIDTH * 1.3);

    // dentro del loop(), reemplazá SOLO el bloque de brightness por este

    float brightness;

    if (!LIGHT_MODE) {
      // ===== SOMBRA GIRATORIA (como ya te gusta) =====
      brightness =
          BASE_BRIGHTNESS + lightSum * LIGHT_PEAK - shadowVal * SHADOW_DEPTH;

    } else {
      // ===== LUZ GIRATORIA REAL =====

      // haz principal (solo uno)
      float mainBeam = gaussian(circDist(ledPos, rot), FOCUS_WIDTH * 0.7);

      // focos secundarios (muy suaves)
      float secondary = lightSum * 0.25;

      brightness = mainBeam * 255 + // haz dominante
                   secondary * 60 - // relleno leve
                   shadowVal * 40;  // sombra casi pasiva
    }

    brightness = constrain(brightness, 0, 255);
    brightness = pow(brightness / 255.0, 2.2) * 255.0;

    leds[i] = CRGB(brightness, brightness, brightness - 10);
  }

  FastLED.show();
  delay(10);
}
