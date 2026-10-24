/**
 * ==============================================================================
 * PROYECTO: CONTROLADOR DE DOBLE COMPUERTA PARA SUCCIONADORA INDUSTRIAL
 * ARCHIVO: GateActuator.h
 * ==============================================================================
 * Descripción:
 * Abstracción de control para actuadores de compuerta.
 * Soporta de manera transparente tanto electroválvulas neumáticas (vía relé)
 * como servomotores mecánicos (vía modulación PWM).
 * ==============================================================================
 */

#ifndef GATE_ACTUATOR_H
#define GATE_ACTUATOR_H

#include <Arduino.h>
#include "Config.h"

#if defined(ACTUATOR_MODE_SERVO)
  #include <Servo.h>
#endif

class GateActuator {
public:
    GateActuator(uint8_t pin, const char* name)
        : _pin(pin),
          _name(name),
          _isOpen(false) {}

    void begin() {
#if defined(ACTUATOR_MODE_SERVO)
        _servo.attach(_pin);
        close();
#else
        // Modo Relé / Electroválvula Neumática
        pinMode(_pin, OUTPUT);
        // En reposo, compuerta cerrada (relé desenergizado)
        digitalWrite(_pin, RELAY_INACTIVE_LEVEL);
        _isOpen = false;
#endif
    }

    void open() {
        if (_isOpen) return;

#if defined(ACTUATOR_MODE_SERVO)
        _servo.write(SERVO_GATE_OPEN_ANGLE);
#else
        // Activa la electroválvula neumática para abrir la trampilla
        digitalWrite(_pin, RELAY_ACTIVE_LEVEL);
#endif
        _isOpen = true;
    }

    void close() {
        if (!_isOpen) {
            // Asegurar el estado físico en el inicio
#if defined(ACTUATOR_MODE_SERVO)
            _servo.write(SERVO_GATE_CLOSED_ANGLE);
#else
            digitalWrite(_pin, RELAY_INACTIVE_LEVEL);
#endif
            return;
        }

#if defined(ACTUATOR_MODE_SERVO)
        _servo.write(SERVO_GATE_CLOSED_ANGLE);
#else
        // Desenergiza la electroválvula (retorno por resorte neumático a posición cerrada)
        digitalWrite(_pin, RELAY_INACTIVE_LEVEL);
#endif
        _isOpen = false;
    }

    bool isOpen() const {
        return _isOpen;
    }

    const char* getName() const {
        return _name;
    }

private:
    const uint8_t _pin;
    const char* _name;
    bool _isOpen;

#if defined(ACTUATOR_MODE_SERVO)
    Servo _servo;
#endif
};

#endif // GATE_ACTUATOR_H
