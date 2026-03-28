# Manual de Uso - Faro Isla Pingüino (ESP32)

Este proyecto recrea el efecto lumínico del faro de Isla Pingüino utilizando un ESP32 y una tira de LEDs WS2812B.

## ⚙️ Configuración Inicial

1. **Conexión:** Al encenderse por primera vez (o si no encuentra red), el ESP32 creará un punto de acceso Wi-Fi llamado `LightHouseAP`.
2. **Wi-Fi:** Conéctate con tu celular y sigue los pasos para vincularlo a tu red Wi-Fi local.
3. **Acceso:** Una vez conectado, abre tu navegador y entra a:
   - [http://faro.local](http://faro.local) (o usa la IP asignada por tu router).

## 🎛️ Panel de Control

El faro cuenta con los siguientes parámetros ajustables en tiempo real:

- **Calidez (0-100):** Ajusta el color desde un blanco frío/azulado (tipo arco de carbón) hasta un naranja cálido (tipo vela).
- **Tiempo (1s-10s):** Controla la velocidad de rotación del haz de luz.
- **Brillo (0-255):** Intensidad de la luz ambiental de base.
- **Peak (80-255):** Intensidad máxima del foco principal.
- **Sombra (100-235):** Profundidad del efecto de sombra/obstrucción.
- **Focus (0.06-0.25):** Ancho del haz de luz. Menor valor produce un haz más concentrado.
- **Modo Luz / Sombra:** Alterna entre un modo de rotación de luz principal y un modo donde predomina el efecto de sombra rotativa.

## 🆙 Actualizaciones (OTA)

El faro soporta actualizaciones inalámbricas. 

> [!IMPORTANT]
> Para activar esta función, debes realizar una **primera carga vía USB** con la versión v1.1.0. 

A partir de allí, el dispositivo aparecerá en tu Arduino IDE (o VS Code) como un puerto de red bajo el nombre `faro`. Podrás subir código sin necesidad de cables.

## 🛠️ Detalles Técnicos
- **Hostname mDNS:** `faro.local`
- **Puerto Web:** 80
- **Puerto OTA:** 3232
- **Librerías principales:** FastLED, WiFiManager, WebServer, ArduinoOTA.
