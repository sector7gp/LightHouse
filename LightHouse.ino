#include <ESPmDNS.h>
#include <FastLED.h>
#include <WebServer.h>
#include <WiFiManager.h>

#define LED_PIN 2
#define NUM_LEDS 12
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

int rotationTime = 4000; // 1 rotation every 12 s

/* Optical parameters */
int baseBrightness = 0;
int lightPeak = 200;
int shadowDepth = 200;
float focusWidth = 0.20;
bool lightMode = true;

// Web Server and Control Variables
WebServer server(80);
int warmthValue = 50; // 0 (Cold) to 100 (Warm)

// HTML Page with Sliders for all parameters
const char *htmlPage = R"rawliteral(
<!DOCTYPE html><html>
<head><meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body { font-family: Arial; text-align: center; margin: 0; padding: 10px; background-color: #222; color: white; }
h3 { color: #f39c12; margin: 10px 0; }
.control-row { display: flex; align-items: center; justify-content: space-between; margin-bottom: 8px; padding: 5px; background: #333; border-radius: 8px; }
.label { font-size: 0.9rem; margin: 0; min-width: 80px; text-align: left; }
.slider-container { flex-grow: 1; margin: 0 10px; }
.slider { -webkit-appearance: none; width: 100%; height: 10px; background: #555; outline: none; opacity: 0.8; border-radius: 5px; }
.slider:hover { opacity: 1; }
.slider::-webkit-slider-thumb { -webkit-appearance: none; appearance: none; width: 20px; height: 20px; background: #f39c12; cursor: pointer; border-radius: 50%; }
.val { color: #ccc; font-size: 0.9rem; min-width: 40px; text-align: right; }
input[type=checkbox] { transform: scale(1.5); }
</style></head>
<body>
  <h3>Lighthouse</h3>

  <div class="control-row">
    <p class="label">Warmth</p>
    <div class="slider-container"><input type="range" min="0" max="100" value="%WARMTH%" class="slider" oninput="update('warmth', this.value)"></div>
    <p class="val" id="warmth_val">%WARMTH%</p>
  </div>

  <div class="control-row">
    <p class="label">Time (s)</p>
    <div class="slider-container"><input type="range" min="1000" max="10000" value="%ROT%" class="slider" oninput="update('rot', this.value)"></div>
    <p class="val" id="rot_val">%ROT_DISP%</p>
  </div>

  <div class="control-row">
    <p class="label">Bright</p>
    <div class="slider-container"><input type="range" min="0" max="255" value="%BASE%" class="slider" oninput="update('base', this.value)"></div>
    <p class="val" id="base_val">%BASE%</p>
  </div>

  <div class="control-row">
    <p class="label">Peak</p>
    <div class="slider-container"><input type="range" min="80" max="255" value="%PEAK%" class="slider" oninput="update('peak', this.value)"></div>
    <p class="val" id="peak_val">%PEAK%</p>
  </div>

  <div class="control-row">
    <p class="label">Shadow</p>
    <div class="slider-container"><input type="range" min="100" max="235" value="%SHADOW%" class="slider" oninput="update('shadow', this.value)"></div>
    <p class="val" id="shadow_val">%SHADOW%</p>
  </div>

  <div class="control-row">
    <p class="label">Focus</p>
    <div class="slider-container"><input type="range" min="6" max="25" value="%FOCUS%" class="slider" oninput="update('focus', this.value)"></div>
    <p class="val" id="focus_val">%FOCUS_DISP%</p>
  </div>

  <div class="control-row" style="justify-content: center;">
    <label for="mode_box" style="margin-right: 10px;">Luz / Sombra</label>
    <input type="checkbox" id="mode_box" onchange="update('mode', this.checked ? 1 : 0)" %CHECKED%>
  </div>

<script>
function update(name, val) {
  if (name !== 'mode') {
      if (name === 'focus') {
          document.getElementById(name + "_val").innerHTML = (val / 100.0).toFixed(2);
      } else if (name === 'rot') {
          document.getElementById(name + "_val").innerHTML = (val / 1000.0).toFixed(1);
      } else {
          document.getElementById(name + "_val").innerHTML = val;
      }
  }
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/set?" + name + "=" + val, true);
  xhr.send();
}
</script>
</body></html>
)rawliteral";

void handleRoot() {
  String page = htmlPage;
  page.replace("%WARMTH%", String(warmthValue));
  page.replace("%ROT%", String(rotationTime));
  page.replace("%ROT_DISP%", String(rotationTime / 1000.0, 1));
  page.replace("%BASE%", String(baseBrightness));
  page.replace("%PEAK%", String(lightPeak));
  page.replace("%SHADOW%", String(shadowDepth));
  page.replace("%FOCUS%",
               String((int)(focusWidth * 100))); // Display as integer 6-25
  page.replace("%FOCUS_DISP%", String(focusWidth));
  page.replace("%CHECKED%", lightMode ? "checked" : "");
  server.send(200, "text/html", page);
}

void handleSet() {
  if (server.hasArg("warmth"))
    warmthValue = server.arg("warmth").toInt();
  if (server.hasArg("rot"))
    rotationTime = server.arg("rot").toInt();
  if (server.hasArg("base"))
    baseBrightness = server.arg("base").toInt();
  if (server.hasArg("peak"))
    lightPeak = server.arg("peak").toInt();
  if (server.hasArg("shadow"))
    shadowDepth = server.arg("shadow").toInt();
  if (server.hasArg("focus"))
    focusWidth = server.arg("focus").toInt() / 100.0;
  if (server.hasArg("mode"))
    lightMode = (server.arg("mode").toInt() == 1);

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
  // 0 = CarbonArc (Cold/Blueish) or OvercastSky
  // 100 = Candle (Very Warm/Orange)

  CRGB colorCold = OvercastSky;
  CRGB colorWarm = Candle; // Changed to Candle for more warmth per user request

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
  // or autoConnect.  // autoConnect will try to connect to saved wifi, or
  // create AP "LightHouseAP" if failed.
  wifiManager.autoConnect("LightHouseAP");

  if (MDNS.begin("faro")) {
    MDNS.addService("http", "tcp", 80);
  }

  // Web Server
  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.begin();
}

void loop() {
  server.handleClient();

  applyTemperature(); // Constantly update temp in case it changed

  float rot = (millis() % rotationTime) / (float)rotationTime;

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
      lightSum += gaussian(d, focusWidth);
    }

    // shadow focus
    float shadowPos = 0.75 + rot;
    if (shadowPos >= 1.0)
      shadowPos -= 1.0;

    shadowVal = gaussian(circDist(ledPos, shadowPos), focusWidth * 1.3);

    float brightness;

    if (!lightMode) {
      // ===== ROTATING SHADOW =====
      brightness =
          baseBrightness + lightSum * lightPeak - shadowVal * shadowDepth;

    } else {
      // ===== REAL ROTATING LIGHT =====

      // main beam (only one)
      float mainBeam = gaussian(circDist(ledPos, rot), focusWidth * 0.7);

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
