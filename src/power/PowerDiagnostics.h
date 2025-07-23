#ifndef POWER_DIAGNOSTICS_H
#define POWER_DIAGNOSTICS_H

#include <Arduino.h>

class PowerDiagnostics {
public:
    static void printCurrentConsumption();
    static void analyzePeripheralPower();
    static void testSleepModes();
    static void printWakeupSources();
    static void measureBaselinePower();
    static void printGPIOStates();
    static void checkPeripheralStates();
    
private:
    static void printPeripheralState(const char* name, bool enabled, float estimated_ma);
    static float estimateCurrentConsumption();
};

#endif // POWER_DIAGNOSTICS_H
