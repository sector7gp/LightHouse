#include <ESPmDNS.h>
#include <FastLED.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>

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
    <p class="label">Calidez</p>
    <div class="slider-container"><input type="range" min="0" max="100" id="warmth" class="slider" oninput="updateVal('warmth', this.value)"></div>
    <p class="val" id="warmth_val">0</p>
  </div>

  <div class="control-row">
    <p class="label">Tiempo (s)</p>
    <div class="slider-container"><input type="range" min="1000" max="10000" id="rot" class="slider" oninput="updateVal('rot', this.value)"></div>
    <p class="val" id="rot_val">0</p>
  </div>

  <div class="control-row">
    <p class="label">Brillo</p>
    <div class="slider-container"><input type="range" min="0" max="255" id="base" class="slider" oninput="updateVal('base', this.value)"></div>
    <p class="val" id="base_val">0</p>
  </div>

  <div class="control-row">
    <p class="label">Peak</p>
    <div class="slider-container"><input type="range" min="80" max="255" id="peak" class="slider" oninput="updateVal('peak', this.value)"></div>
    <p class="val" id="peak_val">0</p>
  </div>

  <div class="control-row">
    <p class="label">Sombra</p>
    <div class="slider-container"><input type="range" min="100" max="235" id="shadow" class="slider" oninput="updateVal('shadow', this.value)"></div>
    <p class="val" id="shadow_val">0</p>
  </div>

  <div class="control-row">
    <p class="label">Focus</p>
    <div class="slider-container"><input type="range" min="6" max="25" id="focus" class="slider" oninput="updateVal('focus', this.value)"></div>
    <p class="val" id="focus_val">0</p>
  </div>

  <div class="control-row" style="justify-content: center;">
    <label for="mode_box" style="margin-right: 10px;">Luz / Sombra</label>
    <input type="checkbox" id="mode_box" onchange="updateMode(this.checked)">
  </div>

<script>
let debounceTimer;

function updateLabels() {
  document.getElementById("warmth_val").innerText = document.getElementById("warmth").value;
  document.getElementById("rot_val").innerText = (document.getElementById("rot").value / 1000.0).toFixed(1);
  document.getElementById("base_val").innerText = document.getElementById("base").value;
  document.getElementById("peak_val").innerText = document.getElementById("peak").value;
  document.getElementById("shadow_val").innerText = document.getElementById("shadow").value;
  document.getElementById("focus_val").innerText = (document.getElementById("focus").value / 100.0).toFixed(2);
}

function updateVal(name, val) {
  // Update UI immediately
  updateLabels();
  
  // Debounce the XHR request (100ms)
  clearTimeout(debounceTimer);
  debounceTimer = setTimeout(function() {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/set?" + name + "=" + val, true);
    xhr.send();
  }, 100);
}

function updateMode(checked) {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/set?mode=" + (checked ? 1 : 0), true);
  xhr.send();
}

// Fetch initial state when page loads
window.onload = function() {
  fetch('/state').then(r => r.json()).then(data => {
    document.getElementById("warmth").value = data.warmth;
    document.getElementById("rot").value = data.rot;
    document.getElementById("base").value = data.base;
    document.getElementById("peak").value = data.peak;
    document.getElementById("shadow").value = data.shadow;
    document.getElementById("focus").value = data.focus;
    document.getElementById("mode_box").checked = (data.mode === 1);
    updateLabels();
  });
};
</script>
</body></html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleState() {
  String json = "{";
  json += "\"warmth\":" + String(warmthValue) + ",";
  json += "\"rot\":" + String(rotationTime) + ",";
  json += "\"base\":" + String(baseBrightness) + ",";
  json += "\"peak\":" + String(lightPeak) + ",";
  json += "\"shadow\":" + String(shadowDepth) + ",";
  json += "\"focus\":" + String((int)(focusWidth * 100)) + ",";
  json += "\"mode\":" + String(lightMode ? 1 : 0);
  json += "}";
  server.send(200, "application/json", json);
}

void applyTemperature();

void handleSet() {
  if (server.hasArg("warmth")) {
    warmthValue = server.arg("warmth").toInt();
    applyTemperature();
  }
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
  server.on("/state", handleState);
  server.on("/set", handleSet);
  server.begin();

  // OTA
  ArduinoOTA.setHostname("faro");
  ArduinoOTA.begin();
}

void loop() {
  server.handleClient();
  ArduinoOTA.handle();

  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate >= 10) {
    lastUpdate = millis();

    float rot = (millis() % rotationTime) / (float)rotationTime;

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
  }
}
