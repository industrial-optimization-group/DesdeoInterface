/*
    Potentiometer.h - Library for a potentiometer component
*/

#include "Arduino.h"
#include "Potentiometer.h"
#include "Component.h"
#include <ADS1X15.h>
Potentiometer::Potentiometer(uint8_t pin,  uint8_t id): Component(id, 'P')
{
    _pin = pin;
};

/*
 * Function:  getValue 
 * --------------------
 * Get the value of the potentiometer.
 *  Is only checked every 150ms seconds as reading from the adc is a bit slow
 *  and we don't want the main loop to be stuck in getting the value as receiving data
 *  is also crucial.
 * 
 * adc: A ADS1115 instance of the ADS module the potentiometer is connected.
 *
 * returns: The current value of the potentiometer
 */
float Potentiometer::getValue(ADS1115 adc)
{
    int analogVal = _prevValue;
    long m = millis();
    if (m - _lastRead >= 250 && adc.isReady()) // Read only every 250ms
    {
        _lastRead = m;
        analogVal = adc.readADC(_pin);
        if (analogVal < 0) analogVal = 0;
    }

    //analogVal = filter(analogVal);
    _hasChanged = (abs(_prevValue-analogVal) > 100);

    if (_hasChanged)
        _prevValue = analogVal;
    return scale(analogVal, _min, _max);
};

/*
 * Function: filter 
 * --------------------
 * Filters the value of the potentiometer using EMA (exponential moving average) algorithm
 * 
 * value: The current value
 *
 * returns: The filtered value
 */
uint16_t Potentiometer::filter(uint16_t value)
{
    float a = 0.8;
    return ((a * value) + ((1-a)*_prevValue));
};

/*
 * Function: scale 
 * --------------------
 * Scales the value to given bounds
 * 
 * value: The value to be scaled
 * min: Minimum value 
 * max: Maximum value
 *
 * returns: The scaled value
 * 
 * Note: Assumes using 16 bit ADS -> max base value 2^15
 */
float Potentiometer::scale(float value, float min, float max) 
{
    if (max <= min) return value;
    return min + (value / 32768)*(max-min);
}

/*
 * Function: setBounds 
 * --------------------
 * Set bounds for the potentiometer
 * 
 * min: Minimum value 
 * max: Maximum value
 */
void Potentiometer::setBounds(float min, float max)
{
    if (max <= min) return;
    _min = min;
    _max = max;
}