# Manual de Integración Eléctrica e Inmunidad a Ruido Industrial

Este documento detalla las directrices de conexionado eléctrico, aislamiento galvánico y supresión de interferencia electromagnética (EMI) para el **Controlador de Doble Compuerta de Succionadora Industrial**, conforme a las recomendaciones de la norma **IEC 60204-1** (Seguridad de las máquinas - Equipo eléctrico de las máquinas).

---

## 1. Arquitectura General de Potencia y Alimentación

En un entorno de lavandería industrial con motores trifásicos de succión (turbinas de canal lateral o sopladores centrífugos) y electroválvulas neumáticas, conviven altas corrientes y transitorios inductivos severos. Para evitar bloqueos, reinicios espontáneos o daño en el ATmega328P, se implementa una **topología de fuentes desacopladas**:

```
 [Línea 230V/400V AC]
         |
         +---------------------------------------------> [Guardamotor / Contactor Motor Succión]
         |                                                       |
         |                                                       v
         |                                             [Motor Trifásico / Blower]
         |
         +--> [Fuente Conmutada DIN 24V DC (Field Power)]
         |           |
         |           +---------------------------------> [Sensores Fotoeléctricos / Proximidad 24V]
         |           +---------------------------------> [Electroválvulas Neumáticas 24V DC]
         |           +---------------------------------> [Bobinas de Relés si son 24V / JD-VCC]
         |
         +--> [Fuente Regulada Lineal/Conmutada 5V DC (Logic Power)]
                     |
                     +---------------------------------> [VCC Arduino Uno/Nano (Pin 5V)]
                     +---------------------------------> [VCC Lógica Módulo de Relés]
```

> **REGLA DE ORO DE PUESTA A TIERRA (ESTRELLA):**
> Nunca encadene los retornos de tierra de potencia (bobinas de contactores, electroválvulas) con el plano de masa digital (GND) del microcontrolador. Todo debe converger en un único borne central de tierra equipotencial (Star Ground).

---

## 2. Acondicionamiento de Sensores de Proximidad Industriales (24V a 5V)

Los sensores industriales estándar (ópticos, inductivos o ultrasónicos) operan con tensiones de 10 a 30 V DC (típicamente 24V DC). No deben conectarse directamente a las entradas de 5V del Arduino.

### Opción Recomendada: Aislamiento Óptico mediante PC817 / Módulo Octoacoplador

```
 +24V DC (Fuente Campo)
    │
    ├───────────────┐ [Marrón / Brown: +24V Sensor]
    │             ┌─┴─────────────┐
    │             │ SENSOR ÓPTICO │
    │             │   TIPO NPN    │
    │             └─┬─────────────┘
    │               │ [Negro / Black: Señal NPN Open-Collector]
    │               │
    │               │     +5V DC (Arduino)
    │               │        │
    │               │        [R2: 4.7kΩ]
    │               │        │
    │               │        ├───────────────> PIN D2 / D3 (Arduino)
    │   [R1: 2.2kΩ] │        │
    │   ┌───[====]──┘      │ / C
    │   │                  │/  Fototransistor
    │   ▼  Ánodo        ───┤
    │  ─── Cátodo          │\  PC817
    │   │                  │ \ E
    │   │                    │
 0V ────┴────────────────────┴───────────────> GND (Arduino)
 (Campo 24V)
```

**Si se usa sensor PNP (salida 24V activo):**
- El cable negro (Señal PNP) se conecta al ánodo del LED del optoacoplador en serie con una resistencia $R = \frac{24V - 1.2V}{10mA} \approx 2.2\text{ k}\Omega$ (1/2 W).
- El cátodo se conecta a 0V (Campo 24V).
- El colector del fototransistor va a la entrada de Arduino con pull-up, y el emisor a GND de Arduino.

---

## 3. Módulo de Relés con Aislamiento Total (Uso del Jumper JD-VCC)

Los módulos de relés de 2, 4 u 8 canales incluyen un puente extraíble (**Jumper VCC - JD-VCC**). Para obtener aislamiento galvánico real:

1. **Retire el puente (jumper) físico entre VCC y JD-VCC.**
2. Conecte **VCC** del módulo al pin **5V de Arduino**.
3. Conecte **IN1, IN2, IN3** a los pines digitales de Arduino (D4, D5, D6).
4. Conecte **JD-VCC** al positivo de una fuente externa de 5V (o 12V según especificación de las bobinas).
5. Conecte **GND** del módulo de relés **ÚNICAMENTE** al negativo de la fuente externa de bobinas.
6. **NO CONECTE EL GND DEL ARDUINO AL GND DEL MÓDULO DE RELÉS.** De esta manera, el retorno de corriente de las bobinas inductivas no circulará por la masa del microcontrolador.

```
 ARDUINO MEGA 2560                 MÓDULO DE RELÉS OPTOACOPLADO
 ┌───────────────┐                 ┌─────────────────────────────────┐
 │            5V ├─────────────────┤ VCC (Alimenta LEDs internos)    │
 │       PIN D4  ├─────────────────┤ IN1 (Motor Succión - Active LOW)│
 │       PIN D5  ├─────────────────┤ IN2 (Compuerta 1)               │
 │       PIN D6  ├─────────────────┤ IN3 (Compuerta 2)               │
 │               │                 │                                 │
 │     [GND]     │ (SIN CONECTAR)  │ GND (Retorno bobinas)           │
 └───────────────┘                 └──────────────┬──────────────────┘
                                                  │
                                            ┌─────┴──────┐
                                            │ JD-VCC GND │
                                            └───┬────┬───┘
                                                │    │
                                      +5V / 12V │    │ 0V (Retorno)
                                                ▼    ▼
                                       [FUENTE EXTERNA BOBINAS]
```

---

## 4. Supresión de Cargas Inductivas (Snubbers y Diodos Flyback)

Las cargas inductivas generan picos de alta tensión (Fuerza Contra-Electromotriz o Back-EMF) al desconectarse ($V = -L \frac{di}{dt}$), capaces de destruir contactos de relé, microcontroladores y generar arcos voltaicos con ruido de radiofrecuencia (EMI).

### A. Electroválvulas Neumáticas DC (Bobinas 24V DC)
Instale en paralelo directo a los bornes de cada solenoide un **diodo de libre circulación (Flyback)**:
- **Diodo:** 1N4007 o UF4007 (Ultra-rápido).
- **Polaridad:** Cátodo (franja plateada) al positivo (+24V) y Ánodo al negativo (0V/masa conmutada).

```
 +24V DC ─────────────────────────┐
                                  │
                                ┌─┴─────────┐
                                │ +  BOBINA │
                     1N4007     │  VÁLVULA  │
                      ┌───▲───┐ │  NEUMÁTICA│
                      │   │   │ │ -         │
                      └───┴───┘ └─┬─────────┘
                                  │
 Conmutación por Relé (NO) ───────┘
```

### B. Contactor de Potencia AC (Motor de Succión 230V / 400V)
El relé del Arduino conmuta la **bobina del contactor de potencia** (típicamente 110V o 230V AC). Debe colocarse una red **RC Snubber** en paralelo con los bornes A1-A2 del contactor:

- **Resistencia:** 100 $\Omega$ (2W, antichispa / oxido metálico).
- **Condensador:** 0.1 $\mu\text{F}$ (100 nF), tipo poliéster metalizado **Clase X2** (voltaje nominal $\ge 630\text{V DC} / 275\text{V AC}$).
- **Varistor (MOV):** S14K275 en paralelo con la red RC para supresión de sobretensiones transitorias.

```
 Línea Fase (L) ────[Contacto Relé Arduino]────┐
                                              │
                                            ┌─┴──────────┐
                                            │     A1     │
                           RED SNUBBER RC   │   BOBINA   │
                         ┌───[ 100Ω 2W ]──┐ │  CONTACTOR │
                         │                │ │  POTENCIA  │
                         └───┤|( 100nF )──┘ │     A2     │
                                            └─┬──────────┘
 Línea Neutro (N) ────────────────────────────┴───────────
```

---

## 5. Cuadro de Cableado y Mapeo de Terminales (Bornas / Clemas)

| Borna / Clema | Señal / Función | Destino en Arduino | Nivel / Tipo de Señal |
| :--- | :--- | :--- | :--- |
| **TB-01** | +24V DC (Fuente de Campo) | No conectado a MCU | +24V DC continuo |
| **TB-02** | 0V DC (Fuente de Campo) | No conectado a MCU | Retorno 24V |
| **TB-03** | Señal Sensor Compuerta 1 | Entrada Optoacoplador -> Pin D2 | NPN/PNP 24V -> 5V Lógico |
| **TB-04** | Señal Sensor Compuerta 2 | Entrada Optoacoplador -> Pin D3 | NPN/PNP 24V -> 5V Lógico |
| **TB-05** | Salida Mando Contactor Succión | Salida Relé 1 (Pin D4) | Contacto seco NA (Active LOW) |
| **TB-06** | Salida Válvula Compuerta 1 | Salida Relé 2 / Servo (Pin D5) | 24V DC a Solenoide / PWM 5V |
| **TB-07** | Salida Válvula Compuerta 2 | Salida Relé 3 / Servo (Pin D6) | 24V DC a Solenoide / PWM 5V |
| **TB-08** | Lámpara LED Compuerta 1 | Salida Pin A0 (Opcional) | 5V DC (20mA máx) o relé indicador |
| **TB-09** | Lámpara LED Compuerta 2 | Salida Pin A1 (Opcional) | 5V DC (20mA máx) o relé indicador |
| **TB-10** | Lámpara LED Alarma / Falla | Salida Pin A2 (Opcional) | 5V DC (20mA máx) o relé alarma |
| **PE** | Tierra de Protección General | Conectado a chasis y barra PE | Conexión verde/amarillo $\ge 2.5\text{ mm}^2$ |

---

## 6. Blindaje y Mangueras de Cableado

1. **Separación de canaletas:** Mantenga los cables de potencia (230V/400V hacia el motor) separados por al menos 15 cm de los cables de señal de los sensores y buses digitales.
2. **Cables blindados para sensores:** Utilice manguera apantallada ($3 \times 0.5\text{ mm}^2$ tipo LiYCY).
   - Conecte la malla metálica a tierra (**PE**) en **UN SOLO EXTREMO** (del lado del cuadro de control).
   - Deje la malla flotante del lado del sensor para evitar lazos de corriente por masa.
