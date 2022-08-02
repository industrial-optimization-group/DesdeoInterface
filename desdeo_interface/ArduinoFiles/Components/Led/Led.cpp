/*
    Led.cpp - Library for displaying status codes with the help of a rgb led
*/

#include "Arduino.h"
#include "Led.h"

Led::Led(uint8_t redPin, uint8_t greenPin, uint8_t bluePin)
{
    _pins[0] = redPin;
    _pins[1] = greenPin;
    _pins[2] = bluePin;
    _isOn = true;
    for (uint8_t pin: _pins)
        pinMode(pin, OUTPUT);
    off();
}

/*
 * Function: setColor
 * --------------------
 * Sets the led color
 *
 * rgb[3]: An array of red, green and blue values to set the pins
 */
void Led::setColor(uint8_t rgb[3])
{
    for (int i = 0; i < 3; ++i)
    {
        _rgb[i] = rgb[i];
        analogWrite(_pins[i], _rgb[i]);
    }
}

/*
 * Function: off
 * --------------------
 * Turns the led off
 */
void Led::off()
{
    _isOn = false;
    uint8_t rgb[3] = {0,0,0};
    setColor(rgb);
}

/*
 * Function: on
 * --------------------
 * Turns the led on
 */
void Led::on()
{
    _isOn = true;
    setColor(_rgb);
}

/*
 * Function: toggle
 * --------------------
 * Toggles the led. If on then turns off and likewise
 */
void Led::toggle()
{
    if (_isOn)
        off();
    else
        on();
}