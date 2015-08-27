#include "Button.h"
#include "Encoder.h"
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"
#include "PhaseControl.h"
#include "I2Cdev.h"
#include "DS1307.h"

#define MODE_OFF 0
#define MODE_ALARM 1
#define MODE_ALARM_LINGER 2
#define MODE_ALARM_HOLD 3
#define MODE_CLOCK 8
#define MODE_CLOCK_LINGER 9
#define MODE_CLOCK_HOLD 10

#define HOLD_DELAY 500
#define LINGER_DELAY 2000
#define TIME_UPDATE_DELAY 1000


Button _alarmButton = Button(7, true, true, 20);
Button _clockButton = Button(6, true, true, 20);
Encoder _encoder = Encoder(8, 9);

DS1307 rtc;

Adafruit_7segment display = Adafruit_7segment();

int _currentMode;
int _lingerStart;
bool _alarmActivated;

byte _alarmHour;
byte _alarmMinute;

byte _currentHour;
byte _currentMinute;
byte _currentSecond;

unsigned long _lastClockUpdate;

long _lastEncoderPosition;
long _encoderDifference;
long _currentEncoderPosition;

void readInputMode();
void handleEncoder();
void updateClock();
void updateDisplay();

void setup()
{
    PhaseControl::initialize(1);
    PhaseControl::setPhase(0.5);

    _currentMode = MODE_OFF;

    rtc.initialize();   // ToDo: go to error mode if failed
    rtc.setTime24(0, 0, 0);

    _lastEncoderPosition = encoder.read();
}

void loop()
{
    readInputMode();
    handleEncoder();
    updateClock();
    updateDisplay();

}

void readInputMode()
{
    _alarmButton.read();
    _clockButton.read();
    _currentEncoderPosition = encoder.read();
    _encoderDifference = _currentEncoderPosition - _lastEncoderPosition;
    _lastEncoderPosition = _currentEncoderPosition;

    if(_alarmButton.isPressed())
    {
        if(_alarmButton.isPressedFor(HOLD_DELAY))
            _currentMode = MODE_ALARM_HOLD;
        else
            _currentMode = MODE_ALARM;
    }
    else if(_clockButton.IsPressed())
    {
        if(_clockButton.isPressedFor(HOLD_DELAY))
            _currentMode = MODE_CLOCK_HOLD;
        else
            _currentMode = MODE_CLOCK;
    }
    else
    {
        switch(currentMode)
        {
            case MODE_ALARM:
                if(_alarmActivated)
                {
                    _alarmActivated = false;
                    _currentMode = MODE_OFF;
                }
                else
                {
                    _lingerStart = millis();
                    _currentMode = MODE_ALARM_LINGER;
                    _alarmActivated = true;
                }
                break;
            case MODE_CLOCK:
                _lingerStart = millis();
                _currentMode = MODE_CLOCK_LINGER;
                break;
            case MODE_ALARM_HOLD:
            case MODE_CLOCK_HOLD:
                _currentMode = MODE_OFF;
                break;
            case MODE_ALARM_LINGER:
            case MODE_CLOCK_LINGER:
                if (millis() - _lingerStart > LINGER_DELAY)
                    _currentMode = MODE_OFF;
                break;
        }
    }
}

void ensureValidClock(byte *hour, byte *minute)
{
    while(*minute > 59)
    {
        *minute -= 60;
        *hour++;
    }
    while(*minute < 0)
    {
        *minute += 60;
        *hour--;
    }
    while(*hour > 23)
        *hour -= 24;
    while(*hour < 0)
        *hour += 24;
}

void handleEncoder()
{
    if(_encoderDifference == 0)
        return;

    if(_currentMode == MODE_ALARM_HOLD)
    {
        _alarmMinute += _encoderDifference;
        ensureValidClock(&_alarmHour, &_alarmMinute);
    }
    else if(_currentMode == MODE_CLOCK_HOLD)
    {
        _clockMinute += _encoderDifference;
        ensureValidClock(&_clockHour, &_clockMinute);
        rtc.setTime24(_currentHour, _currentMinute, 0);
    }
    else
    {
        // Turn on light!
    }
}

void updateClock()
{
    unsigned long currentMillis = millis();
    if(_lastClockUpdate > currentMillis || _lastClockUpdate + TIME_UPDATE_DELAY < currentMillis)
    {
        rtc.getTime24(&_currentHour, &_currentMinute, &_currentSecond);
        _lastClockUpdate = currentMillis;
    }
}

void updateDisplay()
{
    switch(_currentMode)
    {
        case MODE_OFF:
            display.clear();
            display.writeDigitRaw(4, _alarmActivated ? (1 << 7) : 0);
            break;
        case MODE_ALARM:
        case MODE_ALARM_HOLD:
        case MODE_ALARM_LINGER:
            display.writeDigitNum(0, _alarmHour / 10, false);
            display.writeDigitNum(1, _alarmHour % 10, false);
            display.drawColon(true);
            display.writeDigitNum(3, _alarmMinute / 10, false);
            display.writeDigitRaw(4, _alarmMinute % 10,  _alarmActivated);
            break;
        case MODE_CLOCK:
        case MODE_CLOCK_HOLD:
        case MODE_CLOCK_LINGER:
            display.writeDigitNum(0, _clockHour / 10, false);
            display.writeDigitNum(1, _clockHour % 10, false);
            display.drawColon(true);
            display.writeDigitNum(3, _clockMinute / 10, false);
            display.writeDigitRaw(4, _clockMinute % 10,  _alarmActivated);
            break;
    }
    display.writeDisplay();
}
