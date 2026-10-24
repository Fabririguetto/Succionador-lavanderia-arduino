/**
 * ==============================================================================
 * PROYECTO: CONTROLADOR DE DOBLE COMPUERTA PARA SUCCIONADORA INDUSTRIAL
 * ARCHIVO: SuctionController.h
 * ==============================================================================
 * Descripción:
 * Máquina de Estados Finitos (FSM) y lógica de enclavamiento mutuo (Interlock)
 * para el control de compuertas y motor de succión.
 * ==============================================================================
 */

#ifndef SUCTION_CONTROLLER_H
#define SUCTION_CONTROLLER_H

#include <Arduino.h>
#include "Config.h"
#include "DebouncedSensor.h"
#include "GateActuator.h"

// ==============================================================================
// DEFINICIÓN DE ESTADOS DE LA MÁQUINA DE ESTADOS FINITOS (FSM)
// ==============================================================================
enum SystemState {
    STATE_IDLE,                 // Reposo: motor apagado, ambas compuertas cerradas
    STATE_GATE1_ACTIVE,         // C1 activa: Compuerta 1 abierta, motor encendido, C2 bloqueada
    STATE_GATE1_POST_SUCTION,   // C1 post-succión: Temporizador de evacuación de prenda en curso
    STATE_GATE1_SETTLING,       // C1 retardo mecánico: Motor apagado, cerrando C1, enclavamiento total
    STATE_GATE2_ACTIVE,         // C2 activa: Compuerta 2 abierta, motor encendido, C1 bloqueada
    STATE_GATE2_POST_SUCTION,   // C2 post-succión: Temporizador de evacuación de prenda en curso
    STATE_GATE2_SETTLING,       // C2 retardo mecánico: Motor apagado, cerrando C2, enclavamiento total
    STATE_SAFETY_TIMEOUT        // Alarma de seguridad: Sensor obstruido de forma continua (> tiempo límite)
};

class SuctionController {
public:
    SuctionController(DebouncedSensor& sensor1, 
                      DebouncedSensor& sensor2,
                      GateActuator& gate1,
                      GateActuator& gate2,
                      uint8_t motorRelayPin,
                      uint8_t alarmLedPin);

    void begin();
    void update();

    // Consultas de estado
    SystemState getState() const;
    const char* getStateName() const;
    bool isMotorRunning() const;
    unsigned long getActiveElapsedTime() const;

private:
    DebouncedSensor& _sensor1;
    DebouncedSensor& _sensor2;
    GateActuator& _gate1;
    GateActuator& _gate2;
    const uint8_t _motorRelayPin;
    const uint8_t _alarmLedPin;

    SystemState _currentState;
    unsigned long _stateTimer;           // Timestamp de inicio del temporizador actual
    unsigned long _continuousRunTimer;   // Timestamp para detección de sensor atascado
    uint8_t _lastServedGate;             // 1 o 2 (para arbitraje equitativo en empates)

    // Métodos auxiliares de hardware
    void startMotor();
    void stopMotor();
    void setAlarm(bool active);
    void changeState(SystemState newState);

    // Métodos de manejo por estado
    void handleIdle();
    void handleGateActive(uint8_t gateNum);
    void handlePostSuction(uint8_t gateNum);
    void handleSettling(uint8_t gateNum);
    void handleSafetyTimeout();

    // Resolución de prioridades en caso de activación simultánea
    uint8_t resolveSimultaneousTrigger();
};

#endif // SUCTION_CONTROLLER_H
