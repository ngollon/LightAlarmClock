#include "Button.h"
#include "Encoder.h"
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"
#include "PhaseControl.h"
#include "I2Cdev.h"
#include "DS1307.h"
#include <Math.h>

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

#define ENCODER_MICROSTEPS 4


Button _alarmButton = Button(5, true, true, 20);
Button _clockButton = Button(4, true, true, 20);
Encoder _encoder = Encoder(8, 9);

DS1307 rtc;

Adafruit_7segment _display = Adafruit_7segment();

int _currentMode;
int _lingerStart;
bool _alarmActivated;

int8_t  _alarmHour;
int8_t  _alarmMinute;

int8_t  _currentHour;
int8_t  _currentMinute;
int8_t  _currentSecond;

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
    Serial.begin(9600);
    delay(50);
    Serial.println("Starting");
    PhaseControl::initialize();
    PhaseControl::setPhase(0);
    Serial.println("PhaseControl initialized.");
    _currentMode = MODE_OFF;

    //rtc.initialize();   // ToDo: go to error mode if failed
    //rtc.setTime24(0, 0, 0);
    Serial.println("RTC initialized.");

    _display.begin(0x70);
    Serial.println("Display initialized.");

    _lastEncoderPosition = _encoder.read() / ENCODER_MICROSTEPS;
    Serial.println(_lastEncoderPosition);
}

void loop()
{
    long mil = millis();

    PhaseControl::setPhase(sin(M_PI * (mil % 1000) / 1000.0 ));

    readInputMode();
    handleEncoder();
    updateClock();
    updateDisplay();

}

void readInputMode()
{
    _alarmButton.read();
    _clockButton.read();
    _currentEncoderPosition = _encoder.read() / ENCODER_MICROSTEPS;
    _encoderDifference = _currentEncoderPosition - _lastEncoderPosition;

    if(_currentEncoderPosition != _lastEncoderPosition) {
        Serial.print("Current: ");Serial.print(_currentEncoderPosition);Serial.print("   Diff: ");Serial.println(_encoderDifference);
    }

    _lastEncoderPosition = _currentEncoderPosition;

    if(_alarmButton.isPressed())
    {
        if(_alarmButton.pressedFor(HOLD_DELAY))
            _currentMode = MODE_ALARM_HOLD;
        else
            _currentMode = MODE_ALARM;
    }
    else if(_clockButton.isPressed())
    {
        if(_clockButton.pressedFor(HOLD_DELAY))
            _currentMode = MODE_CLOCK_HOLD;
        else
            _currentMode = MODE_CLOCK;
    }
    else
    {
        switch(_currentMode)
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

void ensureValidClock(int8_t  *hour, int8_t  *minute)
{
    while(*minute > 59)
    {
        *minute -= 60;
        *hour += 1;
    }
    while(*minute < 0)
    {
        *minute += 60;
        *hour -= 1;
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
        _currentMinute += _encoderDifference;
        ensureValidClock(&_currentHour, &_currentMinute);
        rtc.setTime24(_currentHour, _currentMinute, _currentSecond);
    }
    else
    {
        // Turn on light!
    }
}

void updateClock()
{
    #if 0
    unsigned long currentMillis = millis();
    if(_lastClockUpdate > currentMillis || _lastClockUpdate + TIME_UPDATE_DELAY < currentMillis)
    {
        rtc.getTime24(&_currentHour, &_currentMinute, &_currentSecond);
        _lastClockUpdate = currentMillis;
    }
    #endif
}

void updateDisplay()
{
    switch(_currentMode)
    {
        case MODE_OFF:
            _display.clear();
            _display.writeDigitRaw(4, _alarmActivated ? (1 << 7) : 0);
            break;
        case MODE_ALARM:
        case MODE_ALARM_HOLD:
        case MODE_ALARM_LINGER:
            _display.writeDigitNum(0, _alarmHour / 10, false);
            _display.writeDigitNum(1, _alarmHour % 10, false);
            _display.drawColon(true);
            _display.writeDigitNum(3, _alarmMinute / 10, false);
            _display.writeDigitNum(4, _alarmMinute % 10,  _alarmActivated);
            break;
        case MODE_CLOCK:
        case MODE_CLOCK_HOLD:
        case MODE_CLOCK_LINGER:
            _display.writeDigitNum(0, _currentHour / 10, false);
            _display.writeDigitNum(1, _currentHour % 10, false);
            _display.drawColon(true);
            _display.writeDigitNum(3, _currentMinute / 10, false);
            _display.writeDigitNum(4, _currentMinute % 10,  _alarmActivated);
            break;
    }
    _display.writeDisplay();
}
