#include "Filter.h"
#include <algorithm>

bool Filter::readAnalog(int pin, float& result) {
    int samples[SAMPLES];
    
    for (int i = 0; i < SAMPLES; i++) {
        samples[i] = analogRead(pin);
        delay(SAMPLE_DELAY);
    }
    
    std::sort(samples, samples + SAMPLES);
    long sum = 0;
    for (int i = 2; i < SAMPLES - 2; i++) {
        sum += samples[i];
    }
    
    result = static_cast<float>(sum) / (SAMPLES - 4);
    return true;
}

bool Filter::readDigital(int pin, float& result) {
    int readings[SAMPLES];
    
    for (int i = 0; i < SAMPLES; i++) {
        readings[i] = digitalRead(pin);
        delay(SAMPLE_DELAY);
    }
    
    int highCount = 0;
    int lowCount = 0;
    
    for (int i = 0; i < SAMPLES; i++) {
        if (readings[i] == HIGH) {
            highCount++;
        } else {
            lowCount++;
        }
    }
    
    result = (highCount > lowCount) ? 0.0f : 1.0f;
    return true;
}