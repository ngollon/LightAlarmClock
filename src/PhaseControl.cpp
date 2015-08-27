#include <Arduino.h>
#include <Math.h>
#include "PhaseControl.h"

#define CLOCK_SPEED 16000000
#define ZERO_CROSSINGS_PER_SECOND 100
#define TICKS_PER_PHASE 20000
#define INT_PIN 2

int PhaseControl::_outputPin = 1;
volatile unsigned int PhaseControl::_phaseTicks = 0;
float PhaseControl::_phase = 0;


void PhaseControl::initialize(int outputPin)
{
    // On Digispark Prom INT 0 is linked to pin 0.
    _outputPin = outputPin;

    pinMode(INT_PIN, INPUT);
    pinMode(_outputPin, OUTPUT);

    cli();                                  // Disable interrupts for configuration
    // Configure zero cross detection
    EIMSK |= (1 << INT0);                   // Enable INT0 interrupt
    EICRA |= (0 << ISC01) | (1 << ISC00);   // Trigger on any change

    // Configure Timer1 to provide the delayed triac trigger
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS11);    // Pre-Scaler: 8, Clear Timer Compare

    // Enable interrupts
    sei();
}

void PhaseControl::setPhase(float phase)
{
    if(phase == 0 && _phase > 0)
    {
        cli();
        TIMSK1 = 0;                         // Disable Timer1
        sei();
    }
    else if( phase > 0 && _phase == 0)
    {
        cli();
        TIMSK1 = (1 << OCIE1A);             // Enable Timer1
        sei();
    }

    _phase = phase;
    // Set _phaseTicks here based on timer frequency and sine wave calculations
    // We want the area below the cut sine phase to be a fraction of the total area 2
    float startTime = acos(2 * phase - 1) / M_PI;
    _phaseTicks = (unsigned int)(startTime * TICKS_PER_PHASE);
}

void PhaseControl::handleZeroCrossing()
{
    cli();                  // Disable inrterrupts for timer configuration
    OCR1A = _phaseTicks;
    TCNT1 = 0;
    sei();
}

void PhaseControl::handleTimer()
{
    // Signal the triac to fire
    digitalWrite(_outputPin, HIGH);
    delayMicroseconds(5);
    digitalWrite(_outputPin, LOW);
}

ISR(INT0_vect)
{
    PhaseControl::handleZeroCrossing();
}

ISR(TIMER1_COMPA_vect)
{
    PhaseControl::handleTimer();
}
