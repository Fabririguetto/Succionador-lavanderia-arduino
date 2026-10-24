/**
 * ==============================================================================
 * PROYECTO: CONTROLADOR DE DOBLE COMPUERTA PARA SUCCIONADORA INDUSTRIAL
 * ARCHIVO: SuctionController.cpp
 * ==============================================================================
 */

#include "SuctionController.h"

SuctionController::SuctionController(DebouncedSensor& sensor1, 
                                     DebouncedSensor& sensor2,
                                     GateActuator& gate1,
                                     GateActuator& gate2,
                                     uint8_t motorRelayPin,
                                     uint8_t alarmLedPin)
    : _sensor1(sensor1),
      _sensor2(sensor2),
      _gate1(gate1),
      _gate2(gate2),
      _motorRelayPin(motorRelayPin),
      _alarmLedPin(alarmLedPin),
      _currentState(STATE_IDLE),
      _stateTimer(0),
      _continuousRunTimer(0),
      _lastServedGate(2) {} // Inicializado en 2 para que en el primer empate gane 1

void SuctionController::begin() {
    // Configuración del relé del motor
    pinMode(_motorRelayPin, OUTPUT);
    stopMotor(); // Asegurar estado apagado en el arranque

    // Configuración del pin de alarma
    pinMode(_alarmLedPin, OUTPUT);
    setAlarm(false);

    // Inicializar compuertas cerradas
    _gate1.begin();
    _gate2.begin();

    _currentState = STATE_IDLE;
    _stateTimer = millis();
    _continuousRunTimer = millis();

#if ENABLE_SERIAL_DIAGNOSTICS
    Serial.println(F("[SISTEMA] Controlador inicializado en estado IDLE. Enclavamiento listo."));
#endif
}

void SuctionController::update() {
    switch (_currentState) {
        case STATE_IDLE:
            handleIdle();
            break;

        case STATE_GATE1_ACTIVE:
            handleGateActive(1);
            break;

        case STATE_GATE1_POST_SUCTION:
            handlePostSuction(1);
            break;

        case STATE_GATE1_SETTLING:
            handleSettling(1);
            break;

        case STATE_GATE2_ACTIVE:
            handleGateActive(2);
            break;

        case STATE_GATE2_POST_SUCTION:
            handlePostSuction(2);
            break;

        case STATE_GATE2_SETTLING:
            handleSettling(2);
            break;

        case STATE_SAFETY_TIMEOUT:
            handleSafetyTimeout();
            break;

        default:
            // Recuperación ante estado anómalo (Protección contra corrupción de memoria/ruido)
            stopMotor();
            _gate1.close();
            _gate2.close();
            changeState(STATE_IDLE);
            break;
    }
}

// ==============================================================================
// MANEJADORES DE ESTADO
// ==============================================================================

void SuctionController::handleIdle() {
    bool s1 = _sensor1.isActive();
    bool s2 = _sensor2.isActive();

    if (s1 && s2) {
        // Empate simultáneo: resolver de forma determinista
        uint8_t winner = resolveSimultaneousTrigger();
#if ENABLE_SERIAL_DIAGNOSTICS
        Serial.print(F("[ARBITRAJE] Empate detectado. Compuerta seleccionada por política: "));
        Serial.println(winner);
#endif
        if (winner == 1) {
            _lastServedGate = 1;
            _gate1.open();
            startMotor();
            _continuousRunTimer = millis();
            changeState(STATE_GATE1_ACTIVE);
        } else {
            _lastServedGate = 2;
            _gate2.open();
            startMotor();
            _continuousRunTimer = millis();
            changeState(STATE_GATE2_ACTIVE);
        }
    } 
    else if (s1) {
        // Activación Compuerta 1 (Compuerta 2 queda enclavada/bloqueada)
        _lastServedGate = 1;
        _gate1.open();
        startMotor();
        _continuousRunTimer = millis();
        changeState(STATE_GATE1_ACTIVE);
    } 
    else if (s2) {
        // Activación Compuerta 2 (Compuerta 1 queda enclavada/bloqueada)
        _lastServedGate = 2;
        _gate2.open();
        startMotor();
        _continuousRunTimer = millis();
        changeState(STATE_GATE2_ACTIVE);
    }
}

void SuctionController::handleGateActive(uint8_t gateNum) {
    DebouncedSensor& activeSensor = (gateNum == 1) ? _sensor1 : _sensor2;

    // Protección térmica/operativa: Sensor atascado por más tiempo del permitido
    if ((millis() - _continuousRunTimer) >= MAX_CONTINUOUS_RUN_TIME_MS) {
#if ENABLE_SERIAL_DIAGNOSTICS
        Serial.print(F("[ALARMA] Tiempo máximo de succión continua excedido en Compuerta "));
        Serial.println(gateNum);
#endif
        stopMotor();
        _gate1.close();
        _gate2.close();
        setAlarm(true);
        changeState(STATE_SAFETY_TIMEOUT);
        return;
    }

    // Mientras el sensor continúe detectando la prenda, se mantiene la succión activa
    // Si la prenda deja de estar frente al sensor, iniciamos la etapa de Post-Succión
    if (!activeSensor.isActive()) {
        _stateTimer = millis();
        changeState((gateNum == 1) ? STATE_GATE1_POST_SUCTION : STATE_GATE2_POST_SUCTION);
    }
}

void SuctionController::handlePostSuction(uint8_t gateNum) {
    DebouncedSensor& activeSensor = (gateNum == 1) ? _sensor1 : _sensor2;

    // Si otra prenda entra antes de que expire la post-succión, reanudamos succión continua
    if (activeSensor.isActive()) {
#if ENABLE_SERIAL_DIAGNOSTICS
        Serial.print(F("[POST-SUCCIÓN] Detección reanudada en Compuerta "));
        Serial.println(gateNum);
#endif
        changeState((gateNum == 1) ? STATE_GATE1_ACTIVE : STATE_GATE2_ACTIVE);
        return;
    }

    // Verificar si se completó el tiempo de post-succión requerido (ej. 2.8 s)
    if ((millis() - _stateTimer) >= POST_SUCTION_TIME_MS) {
#if ENABLE_SERIAL_DIAGNOSTICS
        Serial.print(F("[POST-SUCCIÓN] Completada para Compuerta "));
        Serial.println(gateNum);
#endif
        // Apagar motor de succión y ordenar cierre mecánico de compuerta
        stopMotor();
        if (gateNum == 1) {
            _gate1.close();
            _stateTimer = millis();
            changeState(STATE_GATE1_SETTLING);
        } else {
            _gate2.close();
            _stateTimer = millis();
            changeState(STATE_GATE2_SETTLING);
        }
    }
}

void SuctionController::handleSettling(uint8_t gateNum) {
    // Retardo muerto de seguridad mecánica:
    // Ambas compuertas cerradas, motor apagado. Se ignora cualquier entrada de sensor
    // hasta que transcurra el tiempo para garantizar el sellado físico y descompresión.
    if ((millis() - _stateTimer) >= INTERLOCK_SETTLING_TIME_MS) {
#if ENABLE_SERIAL_DIAGNOSTICS
        Serial.print(F("[SETTLING] Margen de cierre mecánico finalizado para Compuerta "));
        Serial.println(gateNum);
#endif
        changeState(STATE_IDLE);
    }
}

void SuctionController::handleSafetyTimeout() {
    // Parpadeo visual del LED de alarma mientras dure la condición de falla
    static unsigned long lastBlink = 0;
    static bool blinkState = false;
    if (millis() - lastBlink >= 200) {
        lastBlink = millis();
        blinkState = !blinkState;
        digitalWrite(_alarmLedPin, blinkState ? HIGH : LOW);
    }

    // El sistema se auto-recupera solo cuando ambos sensores se encuentren libres y estables
    if (!_sensor1.isActive() && !_sensor2.isActive()) {
#if ENABLE_SERIAL_DIAGNOSTICS
        Serial.println(F("[ALARMA] Sensores despejados. Restableciendo sistema a IDLE..."));
#endif
        setAlarm(false);
        changeState(STATE_IDLE);
    }
}

// ==============================================================================
// MÉTODOS DE SOPORTE Y CONTROL DE HARDWARE
// ==============================================================================

uint8_t SuctionController::resolveSimultaneousTrigger() {
    switch (SYSTEM_TIE_POLICY) {
        case TIE_PRIORITY_GATE_1:
            return 1;
        case TIE_PRIORITY_GATE_2:
            return 2;
        case TIE_ROUND_ROBIN:
        default:
            // Alterna la compuerta servida previamente para evitar hambruna (fairness)
            return (_lastServedGate == 1) ? 2 : 1;
    }
}

void SuctionController::startMotor() {
    digitalWrite(_motorRelayPin, RELAY_ACTIVE_LEVEL);
}

void SuctionController::stopMotor() {
    digitalWrite(_motorRelayPin, RELAY_INACTIVE_LEVEL);
}

void SuctionController::setAlarm(bool active) {
    digitalWrite(_alarmLedPin, active ? HIGH : LOW);
}

void SuctionController::changeState(SystemState newState) {
#if ENABLE_SERIAL_DIAGNOSTICS
    Serial.print(F("[FSM] "));
    Serial.print(getStateName());
    Serial.print(F(" -> "));
#endif

    _currentState = newState;

#if ENABLE_SERIAL_DIAGNOSTICS
    Serial.println(getStateName());
#endif
}

SystemState SuctionController::getState() const {
    return _currentState;
}

const char* SuctionController::getStateName() const {
    switch (_currentState) {
        case STATE_IDLE:                 return "IDLE";
        case STATE_GATE1_ACTIVE:         return "GATE_1_ACTIVE";
        case STATE_GATE1_POST_SUCTION:   return "GATE_1_POST_SUCTION";
        case STATE_GATE1_SETTLING:       return "GATE_1_SETTLING";
        case STATE_GATE2_ACTIVE:         return "GATE_2_ACTIVE";
        case STATE_GATE2_POST_SUCTION:   return "GATE_2_POST_SUCTION";
        case STATE_GATE2_SETTLING:       return "GATE_2_SETTLING";
        case STATE_SAFETY_TIMEOUT:       return "SAFETY_TIMEOUT";
        default:                         return "UNKNOWN";
    }
}

bool SuctionController::isMotorRunning() const {
    return (_currentState == STATE_GATE1_ACTIVE || 
            _currentState == STATE_GATE1_POST_SUCTION ||
            _currentState == STATE_GATE2_ACTIVE || 
            _currentState == STATE_GATE2_POST_SUCTION);
}

unsigned long SuctionController::getActiveElapsedTime() const {
    if (isMotorRunning()) {
        return millis() - _continuousRunTimer;
    }
    return 0;
}
