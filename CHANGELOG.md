# CHANGELOG - LightHouse Project

## [1.1.0] - 2026-03-28

### ✨ Nuevas Funcionalidades
- **Soporte OTA (Over-The-Air):** Se añadió la librería `ArduinoOTA`. Ahora es posible actualizar el firmware de forma inalámbrica a través de la red local (`faro.local`).
- **Estado Inicial Sincronizado:** La interfaz web ahora solicita el estado actual al ESP32 al cargar (`/state`), asegurando que los sliders coincidan con la realidad del dispositivo.
- **API JSON:** Implementación de un endpoint `/state` que devuelve la configuración en formato JSON, mejorando la interoperabilidad.

### 🚀 Optimización y Rendimiento
- **Debounce en Sliders:** Se implementó un retraso de 100ms en el navegador. Ya no se satura el ESP32 con peticiones HTTP mientras se arrastra un slider.
- **Loop No Bloqueante:** Se eliminó `delay(10)` y se reemplazó por un control basado en `millis()`. Esto permite que el servidor web sea mucho más responsivo.
- **Eficiencia de Memoria:** Se eliminó el uso excesivo de `String.replace()` en el servidor. El HTML ahora es estático y el ESP32 consume menos RAM y CPU.
- **Gestión de Temperatura:** `FastLED.setTemperature()` ahora solo se ejecuta cuando el usuario cambia el valor de calidez, no en cada iteración del loop.
- **Limpieza de Renderizado:** Se eliminó `FastLED.clear()` redundante dentro del loop para ahorrar ciclos de CPU.

### 🎨 Mejoras de UI
- Corrección terminológica: "Calides" -> "Calidez".
- Feedback inmediato: Los valores numéricos al lado de los sliders se actualizan instantáneamente al moverlos, antes de enviar la petición al servidor.
