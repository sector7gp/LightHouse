# CHANGELOG - LightHouse Project

## [1.2.0] - 2026-03-29

### ✨ Nuevas Funcionalidades
- **Calendario Semanal (Scheduler):** Se implementó un sistema de encendido/apagado programado por días y horas.
- **Soporte NTP:** El dispositivo ahora sincroniza la hora automáticamente a través de internet (pool.ntp.org) para el funcionamiento del calendario.
- **Master Power Toggle:** Se añadió un interruptor general de encendido/apagado en la web.
- **Interfaz "Dark Mode" Premium:** Nueva estética visual oscura con tarjetas y sombras dinámicas.
- **Chips de Día:** Selección visual intuitiva de los días de la semana (L M X J V S D).

### 🚀 Mejoras Técnicas
- **Bitmask de Días:** Optimización de memoria usando una máscara de bits para los días de la semana.
- **Lógica de Medianoche:** El scheduler soporta rangos horarios que cruzan la medianoche (ej: 22h a 06h).
- **Persistencia mejorada:** El estado del scheduler ahora forma parte integral de la API `/state`.

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
