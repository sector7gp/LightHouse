#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <FastLED.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <time.h>

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

// Scheduler and Power Variables
bool masterPower = true;
uint8_t scheduledDays = 127; // Binary mask: 0b01111111 (All days) - bit0=Mon, ..., bit6=Sun
int startHour = 19, startMin = 0;
int endHour = 23, endMin = 15;
bool isScheduled = false;

// NTP Settings
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -10800; // GMT-3 (Argentina)
const int   daylightOffset_sec = 0;

// HTML Page with Sliders for all parameters
const char *htmlPage = R"rawliteral(
<!DOCTYPE html><html>
<head><meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body { font-family: 'Segoe UI', Arial; text-align: center; margin: 0; padding: 10px; background-color: #1a1a1a; color: white; }
h3 { color: #f39c12; margin: 10px 0; font-weight: 300; letter-spacing: 2px; }
.card { background: #2c2c2c; border-radius: 12px; padding: 15px; margin-bottom: 15px; box-shadow: 0 4px 6px rgba(0,0,0,0.3); }
.control-row { display: flex; align-items: center; justify-content: space-between; margin-bottom: 8px; padding: 8px; background: #383838; border-radius: 8px; }
.label { font-size: 0.85rem; margin: 0; min-width: 70px; text-align: left; color: #bbb; }
.slider-container { flex-grow: 1; margin: 0 12px; }
.slider { -webkit-appearance: none; width: 100%; height: 6px; background: #555; outline: none; border-radius: 3px; }
.slider::-webkit-slider-thumb { -webkit-appearance: none; appearance: none; width: 18px; height: 18px; background: #f39c12; cursor: pointer; border-radius: 50%; box-shadow: 0 0 5px rgba(0,0,0,0.5); }
.val { color: #f39c12; font-size: 0.9rem; min-width: 40px; text-align: right; font-weight: bold; }

/* Scheduler styles */
.chip-container { display: flex; justify-content: space-around; margin: 15px 0; }
.chip { width: 35px; height: 35px; border-radius: 50%; border: 2px solid #555; display: flex; align-items: center; justify-content: center; cursor: pointer; font-size: 0.8rem; transition: 0.3s; color: #888; }
.chip.active { background: #f39c12; border-color: #f39c12; color: #1a1a1a; font-weight: bold; box-shadow: 0 0 10px rgba(243,156,18,0.4); }
.time-container { display: flex; justify-content: center; align-items: center; gap: 10px; margin-top: 10px; }
input[type=time] { background: #383838; color: white; border: 1px solid #555; border-radius: 5px; padding: 5px; font-family: inherit; font-size: 1rem; }

/* Toggle Switch */
.switch { position: relative; display: inline-block; width: 50px; height: 26px; }
.switch input { opacity: 0; width: 0; height: 0; }
.slider-toggle { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #555; transition: .4s; border-radius: 34px; }
.slider-toggle:before { position: absolute; content: ""; height: 18px; width: 18px; left: 4px; bottom: 4px; background-color: white; transition: .4s; border-radius: 50%; }
input:checked + .slider-toggle { background-color: #f39c12; }
input:checked + .slider-toggle:before { transform: translateX(24px); }
</style></head>
<body>
  <h3>LIGHTHOUSE v1.2</h3>

  <div class="card">
    <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 15px;">
      <span style="font-weight: bold; color: #f39c12;">MASTER POWER</span>
      <label class="switch">
        <input type="checkbox" id="master_pwr" onchange="updateVal('pwr', this.checked ? 1 : 0)">
        <span class="slider-toggle"></span>
      </label>
    </div>

    <div class="chip-container">
      <div class="chip" id="day0" onclick="toggleDay(0)">L</div>
      <div class="chip" id="day1" onclick="toggleDay(1)">M</div>
      <div class="chip" id="day2" onclick="toggleDay(2)">X</div>
      <div class="chip" id="day3" onclick="toggleDay(3)">J</div>
      <div class="chip" id="day4" onclick="toggleDay(4)">V</div>
      <div class="chip" id="day5" onclick="toggleDay(5)">S</div>
      <div class="chip" id="day6" onclick="toggleDay(6)">D</div>
    </div>

    <div class="time-container">
      <input type="time" id="time_start" onchange="updateTime('start', this.value)">
      <span style="color: #888;">hasta</span>
      <input type="time" id="time_end" onchange="updateTime('end', this.value)">
    </div>
  </div>

  <div class="card">
    <div class="control-row">
      <p class="label">Calidez</p>
      <div class="slider-container"><input type="range" min="0" max="100" id="warmth" class="slider" oninput="updateLabels()"></div>
      <p class="val" id="warmth_val">0</p>
    </div>
    <!-- ... Rest of sliders ... -->
    <div class="control-row">
      <p class="label">Tiempo (s)</p>
      <div class="slider-container"><input type="range" min="1000" max="10000" id="rot" class="slider" oninput="updateLabels()"></div>
      <p class="val" id="rot_val">0</p>
    </div>
    <div class="control-row">
      <p class="label">Brillo</p>
      <div class="slider-container"><input type="range" min="0" max="255" id="base" class="slider" oninput="updateLabels()"></div>
      <p class="val" id="base_val">0</p>
    </div>
    <div class="control-row">
      <p class="label">Peak</p>
      <div class="slider-container"><input type="range" min="80" max="255" id="peak" class="slider" oninput="updateLabels()"></div>
      <p class="val" id="peak_val">0</p>
    </div>
    <div class="control-row">
      <p class="label">Sombra</p>
      <div class="slider-container"><input type="range" min="100" max="235" id="shadow" class="slider" oninput="updateLabels()"></div>
      <p class="val" id="shadow_val">0</p>
    </div>
    <div class="control-row">
      <p class="label">Focus</p>
      <div class="slider-container"><input type="range" min="6" max="25" id="focus" class="slider" oninput="updateLabels()"></div>
      <p class="val" id="focus_val">0</p>
    </div>
    <div class="control-row" style="justify-content: center; background: transparent;">
      <label for="mode_box" style="margin-right: 10px; font-size: 0.9rem; color: #888;">Modo Luz / Sombra</label>
      <input type="checkbox" id="mode_box" onchange="updateMode(this.checked)">
    </div>
  </div>

<script>
let debounceTimer;
let currentDays = 0;

function updateLabels() {
  document.getElementById("warmth_val").innerText = document.getElementById("warmth").value;
  document.getElementById("rot_val").innerText = (document.getElementById("rot").value / 1000.0).toFixed(1);
  document.getElementById("base_val").innerText = document.getElementById("base").value;
  document.getElementById("peak_val").innerText = document.getElementById("peak").value;
  document.getElementById("shadow_val").innerText = document.getElementById("shadow").value;
  document.getElementById("focus_val").innerText = (document.getElementById("focus").value / 100.0).toFixed(2);
  
  // Debounce generic slider updates
  const activeSlider = document.activeElement;
  if(activeSlider && activeSlider.type === 'range') {
     clearTimeout(debounceTimer);
     debounceTimer = setTimeout(() => {
        sendVal(activeSlider.id, activeSlider.value);
     }, 150);
  }
}

function sendVal(name, val) {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/set?" + name + "=" + val, true);
  xhr.send();
}

function updateVal(name, val) {
  sendVal(name, val);
}

function toggleDay(dayIdx) {
  currentDays ^= (1 << dayIdx);
  updateChipsUI();
  sendVal('days', currentDays);
}

function updateChipsUI() {
  for(let i=0; i<7; i++) {
    document.getElementById("day"+i).className = (currentDays & (1 << i)) ? "chip active" : "chip";
  }
}

function updateTime(type, val) {
  const [h, m] = val.split(':');
  sendVal(type + 'h', h);
  sendVal(type + 'm', m);
}

function updateMode(checked) {
  sendVal('mode', checked ? 1 : 0);
}

window.onload = function() {
  fetch('/state').then(r => r.json()).then(data => {
    document.getElementById("master_pwr").checked = (data.pwr === 1);
    document.getElementById("warmth").value = data.warmth;
    document.getElementById("rot").value = data.rot;
    document.getElementById("base").value = data.base;
    document.getElementById("peak").value = data.peak;
    document.getElementById("shadow").value = data.shadow;
    document.getElementById("focus").value = data.focus;
    document.getElementById("mode_box").checked = (data.mode === 1);
    
    currentDays = data.days;
    updateChipsUI();
    
    document.getElementById("time_start").value = `${String(data.sh).padStart(2, '0')}:${String(data.sm).padStart(2, '0')}`;
    document.getElementById("time_end").value = `${String(data.eh).padStart(2, '0')}:${String(data.em).padStart(2, '0')}`;
    
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
  json += "\"pwr\":" + String(masterPower ? 1 : 0) + ",";
  json += "\"warmth\":" + String(warmthValue) + ",";
  json += "\"rot\":" + String(rotationTime) + ",";
  json += "\"base\":" + String(baseBrightness) + ",";
  json += "\"peak\":" + String(lightPeak) + ",";
  json += "\"shadow\":" + String(shadowDepth) + ",";
  json += "\"focus\":" + String((int)(focusWidth * 100)) + ",";
  json += "\"mode\":" + String(lightMode ? 1 : 0) + ",";
  json += "\"days\":" + String(scheduledDays) + ",";
  json += "\"sh\":" + String(startHour) + ",";
  json += "\"sm\":" + String(startMin) + ",";
  json += "\"eh\":" + String(endHour) + ",";
  json += "\"em\":" + String(endMin);
  json += "}";
  server.send(200, "application/json", json);
}

void applyTemperature();

void handleSet() {
  if (server.hasArg("pwr")) masterPower = (server.arg("pwr").toInt() == 1);
  if (server.hasArg("warmth")) {
    warmthValue = server.arg("warmth").toInt();
    applyTemperature();
  }
  if (server.hasArg("rot")) rotationTime = server.arg("rot").toInt();
  if (server.hasArg("base")) baseBrightness = server.arg("base").toInt();
  if (server.hasArg("peak")) lightPeak = server.arg("peak").toInt();
  if (server.hasArg("shadow")) shadowDepth = server.arg("shadow").toInt();
  if (server.hasArg("focus")) focusWidth = server.arg("focus").toInt() / 100.0;
  if (server.hasArg("mode")) lightMode = (server.arg("mode").toInt() == 1);
  
  if (server.hasArg("days")) scheduledDays = server.arg("days").toInt();
  if (server.hasArg("starth")) startHour = server.arg("starth").toInt();
  if (server.hasArg("startm")) startMin = server.arg("startm").toInt();
  if (server.hasArg("endh")) endHour = server.arg("endh").toInt();
  if (server.hasArg("endm")) endMin = server.arg("endm").toInt();

  server.send(200, "text/plain", "OK");
}

void checkSchedule() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    isScheduled = true; // Si no hay hora, dejamos prendido por defecto
    return;
  }

  // Wday en struct tm es 0=Sun, 1=Mon...6=Sat
  // En mi bitmask puse 0=Mon...6=Sun. Re-mapeamos:
  int myWday = (timeinfo.tm_wday == 0) ? 6 : (timeinfo.tm_wday - 1);
  
  bool dayActive = (scheduledDays & (1 << myWday));
  
  int currentTotalMin = timeinfo.tm_hour * 60 + timeinfo.tm_min;
  int startTotalMin = startHour * 60 + startMin;
  int endTotalMin = endHour * 60 + endMin;

  bool timeActive = false;
  if (startTotalMin < endTotalMin) {
    timeActive = (currentTotalMin >= startTotalMin && currentTotalMin < endTotalMin);
  } else {
    // Caso de medianoche (ej: 22:00 a 06:00)
    timeActive = (currentTotalMin >= startTotalMin || currentTotalMin < endTotalMin);
  }

  isScheduled = dayActive && timeActive;
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
  wifiManager.autoConnect("LightHouseAP");

  // NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

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
  static unsigned long lastScheduleCheck = 0;
  
  if (millis() - lastUpdate >= 10) {
    lastUpdate = millis();

    // Check schedule every 5 seconds
    if (millis() - lastScheduleCheck >= 5000) {
       lastScheduleCheck = millis();
       checkSchedule();
    }

    if (!masterPower || !isScheduled) {
       FastLED.clear();
       FastLED.show();
       return;
    }

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
