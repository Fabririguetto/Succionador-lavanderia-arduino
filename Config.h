/**
 * ==============================================================================
 * PROYECTO: CONTROLADOR DE DOBLE COMPUERTA PARA SUCCIONADORA INDUSTRIAL
 * ARCHIVO: Config.h
 * AUTOR: Experto en Firmware Embebido & Automatización Industrial
 * COMPATIBILIDAD: Arduino Uno / Nano (Microchip ATmega328P @ 16 MHz, 5V)
 * ==============================================================================
 * Descripción:
 * Definiciones de hardware, pines, mapeo lógico, tiempos de ciclo,
 * parámetros de antirrebote y constantes del sistema.
 * ==============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==============================================================================
// 1. SELECCIÓN DEL TIPO DE ACTUADOR DE COMPUERTA
// ==============================================================================
// Descomenta SOLO UNA de las siguientes dos líneas según el hardware instalado:
// - ACTUATOR_MODE_RELAY: Electroválvulas neumáticas controladas por módulo de relé.
// - ACTUATOR_MODE_SERVO: Servomotores de modelismo o industriales (PWM 50Hz).
#define ACTUATOR_MODE_RELAY
//#define ACTUATOR_MODE_SERVO

// ==============================================================================
// 2. DEFINICIÓN DE PINES DE HARDWARE (PINOUT)
// ==============================================================================
// Entradas digitales (Sensores de proximidad fotoeléctricos/inductivos)
const uint8_t PIN_SENSOR_1          = 2;  // Sensor compuerta 1 (Capaz de INT0)
const uint8_t PIN_SENSOR_2          = 3;  // Sensor compuerta 2 (Capaz de INT1)

// Salida de potencia principal
const uint8_t PIN_RELAY_MOTOR       = 4;  // Relé contactor motor succión (Active LOW)

// Salidas de control para actuadores de compuerta
const uint8_t PIN_ACTUATOR_GATE_1   = 5;  // Válvula 1 / Señal PWM Servo 1 (Timer3/OC0B)
const uint8_t PIN_ACTUATOR_GATE_2   = 6;  // Válvula 2 / Señal PWM Servo 2 (Timer0/OC0A)

// Indicadores luminosos de estado (Opcionales para tablero o diagnóstico)
const uint8_t PIN_LED_HEARTBEAT     = 13; // LED integrado de Arduino (latido / WDT)
const uint8_t PIN_LED_GATE_1        = A0; // Indicador óptico Compuerta 1 activa
const uint8_t PIN_LED_GATE_2        = A1; // Indicador óptico Compuerta 2 activa
const uint8_t PIN_LED_ALARM         = A2; // Indicador óptico de falla / bloqueo

// ==============================================================================
// 3. NIVELES LÓGICOS Y POLARIDADES (ACTIVE HIGH / ACTIVE LOW)
// ==============================================================================
// Módulos de relé industriales típicos con aislamiento por optoacoplador:
// Nivel LOW activa el optoacoplador y energiza la bobina.
const uint8_t RELAY_ACTIVE_LEVEL    = LOW;
const uint8_t RELAY_INACTIVE_LEVEL  = HIGH;

// Sensores de proximidad industriales:
// Si se utiliza optoacoplador NPN o pull-up interno: detección = LOW.
// Si se utiliza lógica positiva directa (5V activo): detección = HIGH.
const uint8_t SENSOR_ACTIVE_STATE   = LOW;

// Modo de configuración del pin de entrada (INPUT o INPUT_PULLUP)
const uint8_t SENSOR_PIN_MODE       = INPUT_PULLUP;

// Configuración para Servomotores (solo si ACTUATOR_MODE_SERVO está activo)
const int SERVO_GATE_CLOSED_ANGLE   = 0;   // Ángulo de compuerta cerrada (grados)
const int SERVO_GATE_OPEN_ANGLE     = 90;  // Ángulo de compuerta abierta (grados)

// ==============================================================================
// 4. PARÁMETROS TEMPORALES DEL SISTEMA (MILISEGUNDOS)
// ==============================================================================
// Antirrebote digital (Debounce):
// Filtra ruidos eléctricos inductivos y fluctuaciones de la prenda frente al sensor.
const unsigned long DEBOUNCE_TIME_MS           = 50UL;

// Temporizador de Post-Succión (Overrun / Run-on Timer):
// Tiempo que la compuerta y el motor continúan succionando después de que
// la prenda dejó de ser detectada por el sensor (Asegura transporte completo por ducto).
// Rango requerido: 2500 a 3000 ms.
const unsigned long POST_SUCTION_TIME_MS       = 2800UL;

// Retardo de transición mecánica y descompresión (Interlock Settling / Dead-Time):
// Margen de seguridad tras apagar el motor y cerrar la compuerta activa.
// Garantiza que la compuerta esté mecánicamente sellada antes de habilitar
// el enclavamiento hacia la otra compuerta, evitando pérdidas de vacío.
const unsigned long INTERLOCK_SETTLING_TIME_MS = 600UL;

// Temporizador de seguridad por bloqueo prolongado (Jam / Sensor Stuck Timeout):
// Si una prenda se atasca permanentemente sobre el sensor o el sensor falla cerrado,
// el sistema no debe quemar el motor. Tras este tiempo continuo, aborta a estado seguro.
const unsigned long MAX_CONTINUOUS_RUN_TIME_MS = 60000UL; // 60 segundos máx

// ==============================================================================
// 5. POLÍTICA DE RESOLUCIÓN DE EMPATES (SIMULTANEOUS TRIGGER TIE-BREAK)
// ==============================================================================
enum TieBreakPolicy {
    TIE_PRIORITY_GATE_1,   // En caso de activación en el mismo ms, gana compuerta 1
    TIE_PRIORITY_GATE_2,   // En caso de activación en el mismo ms, gana compuerta 2
    TIE_ROUND_ROBIN        // Alterna la prioridad equitativamente entre compuertas
};

const TieBreakPolicy SYSTEM_TIE_POLICY = TIE_ROUND_ROBIN;

// ==============================================================================
// 6. COMUNICACIÓN SERIE Y DIAGNÓSTICO
// ==============================================================================
#define ENABLE_SERIAL_DIAGNOSTICS 1     // 1: Habilita log serie; 0: Máximo rendimiento/silencioso
const unsigned long SERIAL_BAUD_RATE    = 115200;
const unsigned long HEARTBEAT_PERIOD_MS = 500UL; // Frecuencia de parpadeo del LED Heartbeat

#endif // CONFIG_H
