#include <Arduino.h>
#include <Math.h>
#include "PhaseControl.h"

#define CLOCK_SPEED 16000000
#define ZERO_CROSSINGS_PER_SECOND 100
#define TICKS_PER_PHASE 20000           // Pre-Scaler: 8

#define MIN_PHASE 0.01
#define MAX_PHASE 0.99

#define OUT_PIN 6
#define INT_PIN 7

volatile unsigned int PhaseControl::_phaseTicks = 0;
float PhaseControl::_phase = 0;
int PhaseControl::_fixedState = 0;


void PhaseControl::initialize()
{
    // Hard coded pins

    pinMode(INT_PIN, INPUT);
    pinMode(OUT_PIN, OUTPUT);

     cli();                                  // Disable interrupts for configuration
     // Configure zero cross detection
     EIMSK |= (1 << INT6);                   // Enable INT6 interrupt
     EICRB |= (1 << ISC61) | (1 << ISC60);   // Trigger on rising edge

     // Configure Timer1 to provide the delayed triac trigger
     TCCR1A = 0;
     TCCR1B = (1 << WGM12) | (1 << CS11);    // Pre-Scaler: 8, Clear Timer Compare

     OCR1A = TICKS_PER_PHASE;                // Cycle once per phase
     TIMSK1 = (1 << OCIE1A);                 // Enable Timer1

     // Enable interrupts
     sei();
}

void PhaseControl::setPhase(float phase)
{
    _phase = phase;
    // Set _phaseTicks here based on timer frequency and sine wave calculations
    // We want the area below the cut sine phase to be a fraction of the total area
    float startTime = acos(2 * phase - 1) / M_PI;
    _phaseTicks = (unsigned int)((1 - startTime) * TICKS_PER_PHASE);

    if(phase > MAX_PHASE)
    {
        _fixedState = 1;
        PORTD |= (1 << 7);          //  PIN 7 HIGH
    }
    else if (phase < MIN_PHASE)
    {
        _fixedState = -1;
        PORTD &= ~(1 << 7);         // PIN 7 LOW
    }
    else
        _fixedState = 0;
}

void PhaseControl::handleZeroCrossing()
{
    cli();                      // Disable inrterrupts for timer configuration
    TCNT1 = _phaseTicks;        // Synchronize Timer
    sei();
}

void PhaseControl::handleTimer()
{
    if(_fixedState != 0)
        return;

    // Signal the triac to fire
    PORTD |= (1 << 7);          // Signal PIN 7
    delayMicroseconds(10);
    PORTD &= ~(1 << 7);
}


ISR(INT6_vect)
{
    PhaseControl::handleZeroCrossing();
}

ISR(TIMER1_COMPA_vect)
{
    PhaseControl::handleTimer();
}
