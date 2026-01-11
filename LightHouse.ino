#include <FastLED.h>
#include <WebServer.h>
#include <WiFiManager.h>

#define LED_PIN 2
#define NUM_LEDS 12
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

#define ROTATION_TIME 4000 // 1 rotation every 12 s

/* Optical parameters */
#define BASE_BRIGHTNESS 0
#define LIGHT_PEAK 200
#define SHADOW_DEPTH 200
#define FOCUS_WIDTH 0.20
#define LIGHT_MODE true

// Web Server and Control Variables
WebServer server(80);
int warmthValue = 50; // 0 (Cold) to 100 (Warm)

// HTML Page with Slider
const char *htmlPage = R"rawliteral(
<!DOCTYPE html><html>
<head><meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body { font-family: Arial; text-align: center; margin-top: 50px; background-color: #222; color: white; }
h2 { color: #f39c12; }
.slidecontainer { width: 100%; }
.slider { -webkit-appearance: none; width: 80%; height: 25px; background: linear-gradient(90deg, #aaddff 0%, #ffaa33 100%); outline: none; opacity: 0.9; border-radius: 12px; }
.slider:hover { opacity: 1; }
.slider::-webkit-slider-thumb { -webkit-appearance: none; appearance: none; width: 35px; height: 35px; background: #fff; cursor: pointer; border-radius: 50%; }
</style></head>
<body>
  <h2>Lighthouse Control</h2>
  <p>Warmth Control</p>
  <div class="slidecontainer">
    <input type="range" min="0" max="100" value="%VALUE%" class="slider" id="myRange" oninput="updateVal(this.value)">
  </div>
  <p>Value: <span id="demo">%VALUE%</span></p>
<script>
function updateVal(val) {
  document.getElementById("demo").innerHTML = val;
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/set?val=" + val, true);
  xhr.send();
}
</script>
</body></html>
)rawliteral";

void handleRoot() {
  String page = htmlPage;
  page.replace("%VALUE%", String(warmthValue));
  server.send(200, "text/html", page);
}

void handleSet() {
  if (server.hasArg("val")) {
    warmthValue = server.arg("val").toInt();
    if (warmthValue < 0)
      warmthValue = 0;
    if (warmthValue > 100)
      warmthValue = 100;
  }
  server.send(200, "text/plain", "OK");
}

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

// Map 0-100 to color temperature
void applyTemperature() {
  // 0 = CarbonArc (Cold/Blueish), 100 = Tungsten100W (Warm/Orange)
  // We can also use raw kelvin values.
  // CarbonArc ~ 5200K (Clean White), Tungsten100W ~ 2850K (Warm)
  // OvercastSky ~ 6500K (Cold)

  // Let's mix between OvercastSky (Cold) and Tungsten40W (Very Warm)
  uint32_t cold = OvercastSky; // 0
  uint32_t warm = Tungsten40W; // 100

  // FastLED doesn't have a direct lerp for ColorTemperature enum constants
  // easily accessible as simple integers in a linear way slightly. But
  // setTemperature accepts a CRGB / uint32_t. We can blend between two CRGB
  // representations of these temperatures.

  CRGB colorCold = OvercastSky;
  CRGB colorWarm = Tungsten40W;

  CRGB mixed = blend(colorCold, colorWarm, map(warmthValue, 0, 100, 0, 255));
  FastLED.setTemperature(mixed);
}

void setup() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();

  applyTemperature();

  // WiFiManager
  WiFiManager wifiManager;
  // Make sure to set a timeout so it doesn't block forever if you want it to
  // run without wifi eventually, but for a portal we usually want it to block
  // or autoConnect. autoConnect will try to connect to saved wifi, or create AP
  // "LightHouseAP" if failed.
  wifiManager.autoConnect("LightHouseAP");

  // Web Server
  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.begin();
}

void loop() {
  server.handleClient();

  applyTemperature(); // Constantly update temp in case it changed

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

    float brightness;

    if (!LIGHT_MODE) {
      // ===== ROTATING SHADOW =====
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

    leds[i] = CRGB(brightness, brightness, brightness);
  }

  FastLED.show();
  delay(10);
}
