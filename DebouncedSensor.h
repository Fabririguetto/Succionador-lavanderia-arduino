/**
 * ==============================================================================
 * PROYECTO: CONTROLADOR DE DOBLE COMPUERTA PARA SUCCIONADORA INDUSTRIAL
 * ARCHIVO: DebouncedSensor.h
 * ==============================================================================
 * Descripción:
 * Manejador no bloqueante para sensores digitales industriales con filtro
 * antirrebote temporal (Debounce) y detección de flancos.
 * ==============================================================================
 */

#ifndef DEBOUNCED_SENSOR_H
#define DEBOUNCED_SENSOR_H

#include <Arduino.h>

class DebouncedSensor {
public:
    DebouncedSensor(uint8_t pin, uint8_t pinModeVal, uint8_t activeLevel, unsigned long debounceDelayMs)
        : _pin(pin),
          _pinMode(pinModeVal),
          _activeLevel(activeLevel),
          _debounceDelayMs(debounceDelayMs),
          _rawState(HIGH),
          _debouncedState(HIGH),
          _lastRawState(HIGH),
          _lastDebounceTime(0),
          _justActivated(false),
          _justDeactivated(false) {}

    void begin() {
        pinMode(_pin, _pinMode);
        _rawState = digitalRead(_pin);
        _debouncedState = _rawState;
        _lastRawState = _rawState;
        _lastDebounceTime = millis();
        _justActivated = false;
        _justDeactivated = false;
    }

    /**
     * Debe llamarse en cada iteración del bucle principal.
     * Evalúa el pin de forma no bloqueante y actualiza los flags de flanco.
     */
    void update() {
        _justActivated = false;
        _justDeactivated = false;

        uint8_t reading = digitalRead(_pin);
        unsigned long currentMillis = millis();

        // Si la lectura instantánea difiere de la última registrada, reiniciamos el cronómetro
        if (reading != _lastRawState) {
            _lastDebounceTime = currentMillis;
        }

        // Si ha transcurrido el tiempo de debounce requerido sin cambios, aceptamos el estado
        if ((currentMillis - _lastDebounceTime) >= _debounceDelayMs) {
            if (reading != _debouncedState) {
                _debouncedState = reading;

                if (_debouncedState == _activeLevel) {
                    _justActivated = true;
                } else {
                    _justDeactivated = true;
                }
            }
        }

        _lastRawState = reading;
        _rawState = reading;
    }

    /**
     * Retorna verdadero si el sensor se encuentra establemente en su estado activo.
     */
    bool isActive() const {
        return (_debouncedState == _activeLevel);
    }

    /**
     * Retorna verdadero si ocurrió la transición inactiva -> activa en esta iteración.
     */
    bool isJustActivated() const {
        return _justActivated;
    }

    /**
     * Retorna verdadero si ocurrió la transición activa -> inactiva en esta iteración.
     */
    bool isJustDeactivated() const {
        return _justDeactivated;
    }

    /**
     * Retorna la lectura instantánea sin filtrar (para diagnóstico o telemetría).
     */
    uint8_t readRaw() const {
        return _rawState;
    }

private:
    const uint8_t _pin;
    const uint8_t _pinMode;
    const uint8_t _activeLevel;
    const unsigned long _debounceDelayMs;

    uint8_t _rawState;
    uint8_t _debouncedState;
    uint8_t _lastRawState;
    unsigned long _lastDebounceTime;

    bool _justActivated;
    bool _justDeactivated;
};

#endif // DEBOUNCED_SENSOR_H
