/**
 * ==============================================================================
 * PROYECTO: CONTROLADOR DE DOBLE COMPUERTA PARA SUCCIONADORA INDUSTRIAL
 * ARCHIVO PRINCIPAL: succionadora-industrial-arduino.ino
 * MICROCONTROLADOR: Arduino Uno / Nano (ATmega328P @ 16 MHz)
 * ==============================================================================
 * SISTEMA:
 * - Sistema de extracción neumática/vacío para lavandería industrial.
 * - 2 Sensores de proximidad para detección de prendas por compuerta.
 * - 2 Compuertas mecánicas (actuadores por relé o servomotores).
 * - 1 Motor de turbina/succión de alta potencia común (relé de potencia Active LOW).
 * - Enclavamiento mutuo estricto (Interlock) por FSM no bloqueante.
 * - Temporización de post-succión (2.8s) y retardo de descompresión/sellado mecánico.
 * - Perro guardián de hardware (Watchdog Timer - WDT 2s) para inmunidad EMI.
 * ==============================================================================
 */

#include <avr/wdt.h>
#include "Config.h"
#include "DebouncedSensor.h"
#include "GateActuator.h"
#include "SuctionController.h"

// ==============================================================================
// INSTANCIACIÓN DE SUBSISTEMAS Y COMPONENTES
// ==============================================================================

// Sensores de proximidad digitales con filtro antirrebote
DebouncedSensor sensor1(PIN_SENSOR_1, SENSOR_PIN_MODE, SENSOR_ACTIVE_STATE, DEBOUNCE_TIME_MS);
DebouncedSensor sensor2(PIN_SENSOR_2, SENSOR_PIN_MODE, SENSOR_ACTIVE_STATE, DEBOUNCE_TIME_MS);

// Actuadores de compuertas (Relé o Servo según Config.h)
GateActuator gate1(PIN_ACTUATOR_GATE_1, "Compuerta 1");
GateActuator gate2(PIN_ACTUATOR_GATE_2, "Compuerta 2");

// Controlador principal con lógica FSM y enclavamiento
SuctionController controller(sensor1, sensor2, gate1, gate2, PIN_RELAY_MOTOR, PIN_LED_ALARM);

// ==============================================================================
// VARIABLES DE TELEMETRÍA Y CONTROL TEMPORAL
// ==============================================================================
static unsigned long lastHeartbeatTime = 0;
static unsigned long lastTelemetryTime = 0;
static bool heartbeatLedState = false;

// ==============================================================================
// SETUP (INICIALIZACIÓN DE HARDWARE)
// ==============================================================================
void setup() {
    // 1. Inicialización de la consola de diagnóstico industrial
#if ENABLE_SERIAL_DIAGNOSTICS
    Serial.begin(SERIAL_BAUD_RATE);
    while (!Serial && millis() < 1000); // Espera máx 1 seg para conexión USB
    Serial.println();
    Serial.println(F("========================================================"));
    Serial.println(F("  SISTEMA INDUSTRIAL DE SUCCIÓN - DOBLE COMPUERTA       "));
    Serial.println(F("  Firmware v1.0.0 | Arduino ATmega328P                  "));
    Serial.println(F("========================================================"));
#endif

    // 2. Configurar pines de señalización LED
    pinMode(PIN_LED_HEARTBEAT, OUTPUT);
    pinMode(PIN_LED_GATE_1, OUTPUT);
    pinMode(PIN_LED_GATE_2, OUTPUT);
    digitalWrite(PIN_LED_HEARTBEAT, LOW);
    digitalWrite(PIN_LED_GATE_1, LOW);
    digitalWrite(PIN_LED_GATE_2, LOW);

    // 3. Inicializar sensores y actuadores
    sensor1.begin();
    sensor2.begin();
    gate1.begin();
    gate2.begin();

    // 4. Inicializar controlador FSM
    controller.begin();

    // 5. Configurar Perro Guardián de Hardware (Watchdog Timer - WDT a 2 Segundos)
    // En entornos industriales con contactores de alta potencia y descargas inducidas,
    // el WDT previene bloqueos por perturbaciones electromagnéticas (EMI).
    wdt_enable(WDTO_2S);

#if ENABLE_SERIAL_DIAGNOSTICS
    Serial.println(F("[OK] Hardware inicializado. Perro Guardián (WDT) activo (2s)."));
    Serial.println(F("[OK] Modo de actuador: "));
#if defined(ACTUATOR_MODE_SERVO)
    Serial.println(F("     -> SERVOMOTORES PWM"));
#else
    Serial.println(F("     -> ELECTROVÁLVULAS NEUMÁTICAS (RELÉ)"));
#endif
#endif
}

// ==============================================================================
// LOOP PRINCIPAL (100% NO BLOQUEANTE)
// ==============================================================================
void loop() {
    // 1. Refrescar el perro guardián en cada iteración del bucle
    wdt_reset();

    // 2. Muestreo y filtrado antirrebote de entradas digitales
    sensor1.update();
    sensor2.update();

    // 3. Actualización de la máquina de estados finitos y enclavamiento
    controller.update();

    // 4. Actualización de LEDs indicadores de tablero
    digitalWrite(PIN_LED_GATE_1, gate1.isOpen() ? HIGH : LOW);
    digitalWrite(PIN_LED_GATE_2, gate2.isOpen() ? HIGH : LOW);

    // 5. Latido de vida (Heartbeat LED)
    unsigned long currentMillis = millis();
    if (currentMillis - lastHeartbeatTime >= HEARTBEAT_PERIOD_MS) {
        lastHeartbeatTime = currentMillis;
        heartbeatLedState = !heartbeatLedState;
        digitalWrite(PIN_LED_HEARTBEAT, heartbeatLedState ? HIGH : LOW);
    }

    // 6. Telemetría periódica por puerto serie (opcional para SCADA / Depuración)
#if ENABLE_SERIAL_DIAGNOSTICS
    if (currentMillis - lastTelemetryTime >= 2000UL) {
        lastTelemetryTime = currentMillis;

        Serial.print(F("[ESTADO] FSM: "));
        Serial.print(controller.getStateName());
        Serial.print(F(" | Motor: "));
        Serial.print(controller.isMotorRunning() ? F("ON") : F("OFF"));
        Serial.print(F(" | C1: "));
        Serial.print(gate1.isOpen() ? F("ABIERTA") : F("CERRADA"));
        Serial.print(F(" (S1: "));
        Serial.print(sensor1.isActive() ? F("ACT)") : F("INACT)"));
        Serial.print(F(" | C2: "));
        Serial.print(gate2.isOpen() ? F("ABIERTA") : F("CERRADA"));
        Serial.print(F(" (S2: "));
        Serial.print(sensor2.isActive() ? F("ACT)") : F("INACT)"));
        Serial.println();
    }
#endif
}
