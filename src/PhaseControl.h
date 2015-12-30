#pragma once

class PhaseControl
{
public:
    static void initialize();
    static void setPhase(float phase);

    static void handleZeroCrossing();
    static void handleTimer();
private:
    static bool _fired;
    static int _outputPin;
    static float _phase;
    static volatile unsigned int _phaseTicks;
};
